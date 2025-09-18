#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <ulimit.h>

int main(int argc, char *argv[]) {
	
	char options[] = "ispu2";	
	int c;
	char *s_ptr;

	while ((c = getopt(argc, argv, options)) != EOF) {

		switch (c) {
		case 'i':
			printf("uid: %u\neuid: %u\ngid: %u\negid: %u\n", 
				getuid(), geteuid(), getgid(), getegid());
			break;
		case 's':
			if (setpgid(0, 0) == 0)
				printf("Current process is set as a group leader.\n");
		case 'p':
			printf("pid: %d\nppid: %d\npgrp: %d\n", 
				getpid(), getppid(), getpgrp());
		case 'u':
			printf("ulimit is %ld", ulimit(UL_GETFSIZE));
		default:
			break;
		}

	}

	return 0;
}

