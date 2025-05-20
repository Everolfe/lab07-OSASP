#include "func.h"
int fd = -1;
struct flock fl;
void handle_sigint(int sig);

int main(int argc, char **argv) {
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal error");
        exit(EXIT_FAILURE);
    }
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDWR | O_CREAT, 0666); // Открываем или создаём
    if (fd == -1) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (st.st_size == 0) {  
        printf("File is empty. Creating default records...\n");
        fill_file(fd);  
    }

    menu();
    close(fd);
    return 0;
}

void handle_sigint(int sig) {
    (void)sig;
    printf("\nReceived SIGINT (Ctrl+C). Exiting...\n");
    if (fd != -1) {
        close(fd);
    }
    exit(EXIT_SUCCESS);
}
