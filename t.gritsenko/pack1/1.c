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
			unsigned int uid = getuid();
			unsigned int euid = geteuid();
			unsigned int gid = getgid();
			unsigned int egid = getegid();
			printf("uid: %u\neuid: %u\ngid: %u\negid: %u\n", 
				uid, euid, gid, egid);
			break;
		case 's':
			if (setpgid(0, 0) == 0)
				printf("Current process is set as a group leader.\n");
		case 'p':
			int pid = getpid();
			int ppid = getppid();
			int pgrp = getpgrp();
			printf("pid: %d\nppid: %d\npgrp: %d\n", 
				pid, ppid, pgrp);
		default:
			break;
		}

	}

	return 0;
}

