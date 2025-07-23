#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
sieve(int pipe_fd[2]) {
    int prime, num;

    close(pipe_fd[1]);  //close write

    if (read(pipe_fd[0], &prime, sizeof(int)) != sizeof(int)) {
        fprintf(2, "Error: failed to read prime.\n");
        exit(1);
    }

    printf("prime %d\n", prime);

    // read next num
    if (read(pipe_fd[0], &num, sizeof(int)) == sizeof(int)) {
        int next_pipe[2];
        pipe(next_pipe);

        int pid = fork();
        if (pid < 0) {
            fprintf(2, "Error: fork failed.\n");
            exit(1);
        }

        if (pid > 0) {
            // father
            close(next_pipe[0]); 

            if (num % prime != 0) {
                write(next_pipe[1], &num, sizeof(int));
            }

            while (read(pipe_fd[0], &num, sizeof(int)) == sizeof(int)) {
                if (num % prime != 0) {
                    write(next_pipe[1], &num, sizeof(int));
                }
            }

            close(pipe_fd[0]);
            close(next_pipe[1]);
            wait(0);
        } else {
            // child
            sieve(next_pipe);
        }
    }

    exit(0);
}

int
main(int argc, char *argv[]) {
    int pipe_fd[2];
    pipe(pipe_fd);

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "Error: fork failed.\n");
        exit(1);
    }

    if (pid == 0) {
        // child
        sieve(pipe_fd);
    } else {
        // father
        close(pipe_fd[0]);

        for (int i = 2; i <= 35; i++) {
            if (write(pipe_fd[1], &i, sizeof(int)) != sizeof(int)) {
                fprintf(2, "Error: failed to write %d to pipe.\n", i);
                exit(1);
            }
        }

        close(pipe_fd[1]);
        wait(0);
        exit(0);
    }

    return 0;
}
