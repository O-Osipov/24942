#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <string.h>

#define PATH_MAX_SIZE 1024

extern char **environ;

int main(int argc, char *argv[]) {
	
	char options[] = "ispuU:cC:dvV:";	
	int c;
	char *U_ptr, *C_ptr, *V_ptr, *env_name, *env_val;
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
			if (getcwd(cwd, PATH_MAX_SIZE)) {
				printf("%s", cwd);
			}
		case 'v':
			for (char **env_var = environ; *env_var != NULL; env_var++) {
        		printf("%s\n", *env_var);
    		}
		case 'V':
			V_ptr = optarg;
			env_name = strtok(V_ptr, "=");
			env_val = strtok(V_ptr, "=");
			if (setenv(env_name, env_val, 1) == 0) {
				printf("variable %s is set to $s", env_name, env_val);
			}
		default:
			break;
		}

	}

	return 0;
}

