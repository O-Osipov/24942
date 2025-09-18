#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <ulimit.h>

int main(int argc, char *argv[]) {
	
	char options[] = "is:p";	
	int c;
	char *s_ptr;

	while ((c = getopt(argc, argv, options)) != EOF) {

		switch (c) {
		case 'i':
			uid_t uid = getuid();
			uid_t euid = geteuid();
			gid_t gid = getgid();
			gid_t egid = getegid();
			printf("uid: %u\neuid: %u\ngid: %u\negid: %u\n", 
				uid, euid, gid, egid);
			break;
		case 's':
			if (setpgid(0, 0) == 0)
				printf("Current process is set as a group leader.\n");
		case 'p':
			pid_t pid = getpid();
			pid_t ppid = getppid();
			pid_t pgrp = getpgrp();
			printf("pid: %d\nppid: %d\npgrp: %d", 
				pid, ppid, pgrp);
		default:
			break;
		}

	}

	return 0;
}

