#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ulimit.h>

#define PATH_MAX_SIZE 1024

int main(int argc, char *argv[]) {
	
	char options[] = "ispuU:cC:d";	
	int c;
	char *U_ptr, *C_ptr;
	char cwd[PATH_MAX_SIZE];
	struct rlimit rlp;
	

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
			getrlimit(RLIMIT_FSIZE, &rlp);
			printf("ulimit is %lu or %ld\n", rlp.rlim_max, ulimit(UL_GETFSIZE));
			break;
		case 'U':
			U_ptr = optarg;
			rlp.rlim_cur = atol(U_ptr);
			rlp.rlim_max = rlp.rlim_cur;
			if (setrlimit(RLIMIT_FSIZE, &rlp) == 0)
				printf("ulimit value has changed.\n");
			break;
		case 'c':
			getrlimit(RLIMIT_CORE, &rlp);
			printf("max core-file size is %lu\n", rlp.rlim_max);
		case 'C': // not working idk
			C_ptr = optarg;
			getrlimit(RLIMIT_CORE, &rlp);
			rlp.rlim_cur = atol(C_ptr);
			rlp.rlim_max = rlp.rlim_cur;
			if (setrlimit(RLIMIT_CORE, &rlp) == 0) 
				printf("max core-file size has changed.\n");
		case 'd':
			if (getcwd(cwd, sizoef(cwd))) {
				printf("%s", cwd);
			}
		default:
			break;
		}

	}

	return 0;
}

