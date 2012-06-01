#include <asm/page.h>

#include <linux/sched.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

static ssize_t process_name_show(struct kobject *kobj, 
				struct kobj_attribute *attr, 
				char *buf)
{
	strncpy(buf, container_of(kobj, struct task_struct, pslist_link)->comm,
		TASK_COMM_LEN);

	//snprintf(buf, PAGE_SIZE, "%." stringify(TASK_COMM_SIZE) "s",   
	return TASK_COMM_LEN;
}

static struct kobj_attribute process_name_attr = __ATTR_RO(process_name);

static struct attribute* process_attrs[] = {
	&process_name_attr.attr,
	NULL
};

static struct kobj_type process_kobj_type = {
	.release	= NULL,
	.sysfs_ops	= &kobj_sysfs_ops,
	.default_attrs	= process_attrs,
	.child_ns_type	= NULL,
	.namespace	= NULL
};

static struct kobj_type pslist_root_kobj_type = {
	.release	= NULL,
	.sysfs_ops	= &kobj_sysfs_ops,
	.default_attrs	= NULL,
	.child_ns_type	= NULL,
	.namespace	= NULL
};

static struct kobject pslist_root_kobj;

int pslist_task_init(struct task_struct *child)
{
	int result;
	
	memset(&child->pslist_link, 0, sizeof(struct kobject));

	kobject_init(&child->pslist_link, &process_kobj_type);
	result = kobject_add(&child->pslist_link, &pslist_root_kobj, "%d", child->pid);
	if (result) {
		printk(KERN_ERR "Can't add child process to the pslist tree\n");
		kobject_put(&child->pslist_link);
		return result;
	}

	return 0;
} 

int pslist_task_link(struct task_struct *parent, struct task_struct *child)
{
	int result;
	if (kernel_kobj->state_initialized) {
		result = sysfs_create_link(&parent->pslist_link, 
					&child->pslist_link, 
					kobject_name(&child->pslist_link));
		if (result) {
			printk(KERN_ERR "Can't create sysfs link between child and parent processes");
			return result;
		}
	} else {
		// TODO: save link information somewhere, or decide to recreate this information
		// in pslist_ksysfs_init
	}

	return 0;
}

void pslist_task_release(struct task_struct *tsk)
{
	kobject_del(&tsk->pslist_link);
	kobject_put(&tsk->pslist_link);
}

/**
 *	pslist_ksysfs_init - to be called right after ksysfs initialization
 */
int pslist_ksysfs_init(void)
{
	int result;

	result = kobject_add(&pslist_root_kobj, kernel_kobj, "pslist");
	if (result) {
		printk(KERN_ERR "Can't add PSList root KObject to KSysFS\n");
		kobject_put(&pslist_root_kobj);
		return result;
	}

	// TODO: create sysfs symlinks between parent-child pslist nodes

	return 0;
}

static __init int pslist_init(void)
{
	kobject_init(&pslist_root_kobj, &pslist_root_kobj_type);

	return 0;
}

core_initcall(pslist_init);

