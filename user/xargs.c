#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"


#define DEBUG 0
#define debug(code) if (DEBUG) { code; }


void run_with_args(char *program, char **args);

void
xargs(char **initial_args, int initial_argc, char *program_name) {
    char buf[1024];               
    char *args[MAXARG];           
    int buf_index = 0;            

    debug({
        for (int i = 0; i < initial_argc; i++) {
            printf("initial_args[%d] = %s\n", i, initial_args[i]);
        }
    })

    // read the char
    while (read(0, buf + buf_index, 1) == 1) {
        if (buf_index >= sizeof(buf)) {
            fprintf(2, "xargs: input too long.\n");
            exit(1);
        }

        // end with '\n' 
        if (buf[buf_index] == '\n') {
            buf[buf_index] = '\0';

            debug(printf("Read line: %s\n", buf);)

            memmove(args, initial_args, sizeof(char *) * initial_argc);

            int arg_index = initial_argc;

            // default
            if (arg_index == 0) {
                args[arg_index++] = program_name;
            }

            // add the action
            args[arg_index] = malloc(buf_index + 1);
            memmove(args[arg_index], buf, buf_index + 1);
            arg_index++;

            args[arg_index] = 0;  

            debug({
                for (int i = 0; args[i] != 0; i++) {
                    printf("args[%d] = \"%s\"\n", i, args[i]);
                }
            })

            // run
            run_with_args(program_name, args);

            free(args[arg_index - 1]);  // free malloc space
            buf_index = 0;              
        } else {
            buf_index++;
        }
    }
}

// child run
void
run_with_args(char *program, char **args) {
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "xargs: fork failed.\n");
        exit(1);
    }

    if (pid == 0) {
        // child
        debug({
            printf("Child process executing:\n");
            printf("  Program: %s\n", program);
            for (int i = 0; args[i] != 0; i++) {
                printf("  args[%d]: %s\n", i, args[i]);
            }
        })

        if (exec(program, args) == -1) {
            fprintf(2, "xargs: failed to exec %s\n", program);
        }

        debug(printf("Child process exiting.\n");)
        exit(1);
    } else {
        wait(0);
    }
}

int
main(int argc, char *argv[]) {
    debug(printf("xargs: main function start\n");)

    char *program = "echo";  

    if (argc >= 2) {
        program = argv[1];   
        debug({
            printf("argc >= 2\n");
            printf("Using program: %s\n", program);
        })
    } else {
        debug(printf("Using default program: echo\n");)
    }

    xargs(argv + 1, argc - 1, program);

    exit(0);
}
