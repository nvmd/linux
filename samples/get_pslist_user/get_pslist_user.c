#include <stdio.h>
#include <string.h>
#include <errno.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

// --------------------------------------------------------------------------------

typedef struct {
  pid_t pid;
  char name[16];
} process_t;

#define __NR_get_pslist  349

#define SYS_get_pslist __NR_get_pslist     

// --------------------------------------------------------------------------------

static process_t processes[600];

int main(int argc, char **argv)
{
	if (syscall(SYS_get_pslist, &processes, sizeof(processes)) < 0) {
		printf("Failed to get the process list: %s\n", strerror(errno));
		return 1;
	}

	process_t* p = processes;
	while (p->pid) {
		printf("Task: %.16s, pid: %d\n", p->name, p->pid);
		++p;
	}

	return 0;
}

