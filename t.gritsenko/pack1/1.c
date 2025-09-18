#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <ulimit.h>

int main(int argc, char *argv[]) {
	
	char options[] = "ispuU:";	
	int c;
	char *U_ptr;
	long new_fsize;

	while ((c = getopt(argc, argv, options)) != EOF) {

		switch (c) {
		case 'i':
			printf("uid: %u\neuid: %u\ngid: %u\negid: %u\n", 
				getuid(), geteuid(), getgid(), getegid());
			break;
		case 's':
			if (setpgid(0, 0) == 0)
				printf("Current process is set as a group leader.\n");
			break;
		case 'p':
			printf("pid: %d\nppid: %d\npgrp: %d\n", 
				getpid(), getppid(), getpgrp());
			break;
		case 'u':
			printf("ulimit is %ld\n", ulimit(UL_GETFSIZE));
			break;
		case 'U':
			U_ptr = optarg;
			new_fsize = atol(U_ptr);
			if (ulimit(UL_SETFSIZE, new_fsize) == new_fsize)
				printf("ulimit value has changed.\n");
			break;
		default:
			break;
		}

	}

	return 0;
}

