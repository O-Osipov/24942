#include <unistd.h>
#include <stdio.h>
#include <sys/types.h> 

void printUIDs() {
    
    uid_t real_uid = getuid();       // функции getuid() - Real User ID; geteuid() - Effective User ID
    uid_t effective_uid = geteuid();

    printf("uid: %d, euid: %d\n", real_uid, effective_uid);     
}

void tryToOpen(char *filename) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("Failed to open the file");
        return;
    }

    fclose(file);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Filename is not specified\n");
        return 1;
    }

    printUIDs();
    tryToOpen(argv[1]);

    setuid(getuid()); // устанавливает оба UID в Real UID

    printUIDs();
    tryToOpen(argv[1]);

    return 0;
}
