#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/netlink.h>
#include <net/net_namespace.h>
#include <linux/skbuff.h>
#include <linux/mutex.h>

#include <linux/sched.h>
#include <linux/types.h>

// pslist procotol number
#define NETLINK_PSLIST 31

struct pslist_req_hdr {
	int type;
	pid_t pid;
};

#define PSLIST_REQ_TYPE_PROC_LIST 1
#define PSLIST_REQ_TYPE_GROUP_THREADS_LIST 2
#define PSLIST_REQ_TYPE_PROC_COMM 3

static struct sock *netlink_sock;
static DEFINE_MUTEX(netlink_pslist_mtx);

static struct sk_buff *init_response(unsigned int size)
{
	struct sk_buff *response;

	response = alloc_skb(size, GFP_KERNEL);
	NETLINK_CB(response).pid = 0;
	NETLINK_CB(response).dst_group = 0;
	
	return response;
}

static int respond_with_pid(pid_t pid, __u32 recipient)
{
	struct sk_buff *response;
	pid_t *response_pid;
	int result;

	response = init_response(sizeof(pid_t));
	response_pid = (pid_t *)skb_put(response, sizeof(pid_t));
	*response_pid = pid;
	
	result = netlink_unicast(netlink_sock, response, recipient, 0);
	if (result) {
		printk(KERN_ERR "Can't send pid response\n");
	}

	return result;
}

static int respond_with_comm(char *comm, __u32 recipient)
{
	struct sk_buff *response;
	char *response_comm;
	int result;

	response = init_response(TASK_COMM_LEN);
	response_comm = (char *)skb_put(response, TASK_COMM_LEN);
	strncpy(response_comm, comm, TASK_COMM_LEN);
	
	result = netlink_unicast(netlink_sock, response, recipient, 0);
	if (result) {
		printk(KERN_ERR "Can't send group comm response\n");
	}

	return result;
}

static void receive_netlink_data(struct sk_buff *skb)
{
	struct pslist_req_hdr *req;
	struct sk_buff *response;

	struct task_struct *task;
	struct task_struct *thread;

	req = (struct pslist_req_hdr *)skb->data;
	switch (req->type) {
	case PSLIST_REQ_TYPE_PROC_LIST:
		for_each_process(task) {
			respond_with_pid(task->pid, NETLINK_CB(skb).pid);
		}
		respond_with_pid(-1, NETLINK_CB(skb).pid);
		break;
	case PSLIST_REQ_TYPE_GROUP_THREADS_LIST:
		do_each_thread(task, thread) {
			if (thread->tgid == req->pid) {
				respond_with_pid(thread->pid, NETLINK_CB(skb).pid);
			}
		} while_each_thread(task, thread);
		respond_with_pid(-1, NETLINK_CB(skb).pid);
		break;
	case PSLIST_REQ_TYPE_PROC_COMM:
		response = NULL;
		for_each_process(task) {
			if (task->pid == req->pid) {
				respond_with_comm(task->comm, NETLINK_CB(skb).pid);
				break;
			}
		}
		if (!response) {
			respond_with_comm("", NETLINK_CB(skb).pid);
		}
		break;
	default:
		printk(KERN_ERR "Unknown request type\n");
		break;
	}
}

static int __init init_pslist(void)
{
	netlink_sock = netlink_kernel_create(&init_net, NETLINK_PSLIST,
					0,
					receive_netlink_data,
					&netlink_pslist_mtx,
					THIS_MODULE);
	if (!netlink_sock) {
		printk(KERN_ERR "Can't create netlink socket\n");
		return 1;
	}
					
	return 0;
}

static void __exit cleanup_pslist(void)
{
	netlink_kernel_release(netlink_sock);
	printk(KERN_INFO "PSList module unloaded\n");
}

module_init(init_pslist);
module_exit(cleanup_pslist);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergey Kazenyuk <kazenyuk@gmail.com>");
MODULE_DESCRIPTION("Exporting process information through Netlink");
MODULE_VERSION("1.0");

