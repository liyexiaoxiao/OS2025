#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pipefd[2];  // pipe for the two processes,pipefd[0]-read，pipefd[1]-write

    if (pipe(pipefd) < 0) {
        fprintf(2, "Failed to create pipe.\n");
        exit(1);
    }

    int pid = fork();

    if (pid < 0) {
        fprintf(2, "Fork failed.\n");
        exit(1);
    }

    if (pid == 0) {
        // child process:waiting for recieving news and reply
        char msg[1];
        read(pipefd[0], msg, 1);         
        close(pipefd[0]);                
        fprintf(1, "%d: received ping\n", getpid());

        write(pipefd[1], msg, 1);        
        close(pipefd[1]);               
    } else {
        // father process:send news and waiting for reply
        char msg[1] = {'x'};
        write(pipefd[1], msg, 1);        
        close(pipefd[1]);                

        read(pipefd[0], msg, 1);       
        fprintf(1, "%d: received pong\n", getpid());
        close(pipefd[0]);               
    }

    exit(0);
}

