#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

int main() {
	//ðâûîðàîûôðàîëûâ
	pid_t pid = fork();
	switch (pid = fork()) {
	case -1:
		return 1;
		break;
	case 0:
		execlp("cat", "cat", "../task1.c", NULL);
		return 1;
		break;
	defolt:
		if (wait(NULL) != -1) {
			printf("\nChild process (pid: %d) finished\n", pid);
		}
	}
	return 0;
}