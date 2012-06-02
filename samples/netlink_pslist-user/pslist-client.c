#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <sys/types.h>

// pslist procotol number
#define NETLINK_PSLIST 31

struct pslist_req_hdr {
	int type;
	pid_t pid;
};

#define PSLIST_REQ_TYPE_PROC_LIST 1
#define PSLIST_REQ_TYPE_GROUP_THREADS_LIST 2
#define PSLIST_REQ_TYPE_PROC_COMM 3

#define TASK_COMM_LEN 16

ssize_t send_request(int sock, struct sockaddr_nl *sockaddr, 
			struct pslist_req_hdr *req)
{
	struct msghdr message;
	struct iovec iov;
	int result;
	
	iov.iov_base = (void*)req;
	iov.iov_len = sizeof(message);
	
	memset(&message, 0, sizeof(message));
	message.msg_name = (void*)sockaddr;
	message.msg_namelen = sizeof(*sockaddr);
	message.msg_iov = &iov;
	message.msg_iovlen = 1;
	
	result = sendmsg(sock, &message, 0);
	if (result == -1) {
		perror("sendmsg");
	}
	return result;
}

int receive_pid_response(int sock)
{
	pid_t pid;
	int result;

	while ((result = read(sock, &pid, sizeof(pid_t))) > 0   
		&& pid != -1) {
		printf("%d\n", pid);
	}

	if (result < 0) {
		perror("read");
	}

	return result;
}

int request_pids(int sock, struct sockaddr_nl *sockaddr,
		int req_type, pid_t pid)
{
	struct pslist_req_hdr req;
	int result;

	req.pid = pid;
	req.type = req_type;
	result = send_request(sock, sockaddr, &req);
	if (result < 0) {
		return result;
	}

	result = receive_pid_response(sock);

	return result;
}

int request_proc_list(int sock, struct sockaddr_nl *sockaddr,
			pid_t pid)
{
	return request_pids(sock, sockaddr, PSLIST_REQ_TYPE_PROC_LIST, pid);
}

int request_group_threads_list(int sock, struct sockaddr_nl *sockaddr, 
				pid_t pid)
{
	return request_pids(sock, sockaddr, PSLIST_REQ_TYPE_GROUP_THREADS_LIST, pid);
}

int request_proc_comm(int sock, struct sockaddr_nl *sockaddr,
			pid_t pid)
{
	struct pslist_req_hdr req;
	char comm[TASK_COMM_LEN];
	int result;

	req.pid = pid;
	req.type = PSLIST_REQ_TYPE_PROC_COMM;
	result = send_request(sock, sockaddr, &req);
	if (result < 0) {
		return result;
	}

	result = read(sock, &pid, TASK_COMM_LEN);
	if (result < 0) {
		return result;
	}
	printf("%s\n", comm);

	return result;
}

void init_sockaddr(struct sockaddr_nl *sockaddr)
{
	memset(&sockaddr, 0, sizeof(*sockaddr));
	sockaddr->nl_family = AF_NETLINK;
	sockaddr->nl_pad = 0;
	sockaddr->nl_pid = 0;
	sockaddr->nl_groups = 0;
}

void print_usage(const char *command)
{
	printf("Usage: %s <0|1|2> [tgid for 1|pid for 2]\n", command);
}

int main(int argc, char **argv)
{
	struct sockaddr_nl sockaddr;
	int sock;
	int user_req;
	int (*user_req_table[])(int,struct sockaddr_nl*,pid_t) = {
						request_proc_list,
						request_group_threads_list,
						request_proc_comm};
	pid_t pid;
	int result;
	
	result = 0;

	if (argc > 3 || argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	user_req = atoi(argv[1]);
	if (user_req < 0 || user_req > 2) {
		print_usage(argv[0]);
		return -1;
	}

	if ((user_req > 0 && argc != 3) || (user_req == 0 && argc != 2)) {
		print_usage(argv[0]);
		return -1;
	}

	if (user_req == 0) {
		pid = -1;
	} else {
		pid = atoi(argv[1]);
	}
	
	sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_PSLIST);
	if (sock < 0) {
		perror("socket");
		close(sock);
		return -1;
	}
	
	init_sockaddr(&sockaddr);

	result = connect(sock, (const struct sockaddr*)&sockaddr, sizeof(sockaddr));
	if (result < 0) {
		perror("connect");
		close(sock);
		return result;
	}

	result = user_req_table[user_req](sock, &sockaddr, pid);
	if (result < 0) {
		printf("Can't complete user request\n");
		return result;
	}
	close(sock);

	return result;
}

