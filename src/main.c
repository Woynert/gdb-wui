#include "stdio.h"
#include "raygui.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "sys/wait.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

int main (void) {
    printf("Hello there\n");

    int master_to_child_pipe[2];
    int child_to_master_pipe[2];

    if (pipe(master_to_child_pipe) == -1 || pipe(child_to_master_pipe) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {

        // redirect stdin/stdout to pipe
        if (dup2(master_to_child_pipe[0], STDIN_FILENO) == -1)
            { perror("dup2"); _exit(1); };
        if (dup2(child_to_master_pipe[1], STDOUT_FILENO) == -1)
            { perror("dup2"); _exit(1); };

        // close original pipe ends
        close(master_to_child_pipe[1]);
        close(master_to_child_pipe[0]);
        close(child_to_master_pipe[0]);
        close(child_to_master_pipe[1]);

        // die on parent exit
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        // Example: exec grep that waits for input
        /*execlp("cat", "cat", NULL);*/
        execlp("gdb", "gdb", NULL);

        // If exec fails
        perror("exec");
        exit(1);
    }

    // Parent process
    close(master_to_child_pipe[0]);  // Parent doesn't read commands
    close(child_to_master_pipe[1]); // Parent doesn't write results

    // Set child→parent read end to non-blocking
    int flags = fcntl(child_to_master_pipe[0], F_GETFL, 0);
    fcntl(child_to_master_pipe[0], F_SETFL, flags | O_NONBLOCK);

    // Write a command
    /*const char *cmd = "foo123 test\n";*/
    const char *cmd = "file ../smb-raylib/build/3djump\n";
    long written_bytes = write(master_to_child_pipe[1], cmd, strlen(cmd));
    (void)written_bytes;
    printf("%ld bytes were written\n", written_bytes);

    for (int i = 15; i > 1; --i) {
        sleep(1); // simulate doing other work

        // Try reading (non-blocking)
        char buffer[256];
        ssize_t n = read(child_to_master_pipe[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            /*printf("Got output: %s", buffer);*/
            printf("%s", buffer);
        } else {
            printf("\n");
            printf("\n");
            printf("................\n");
            printf("No output yet...\n");
            printf("................\n");
            printf("\n");
            printf("\n");

            const char *cmd_run = "b main\n";
            written_bytes = write(master_to_child_pipe[1], cmd_run, strlen(cmd_run));
            (void)written_bytes;
            printf("%ld bytes were written\n", written_bytes);
        }
    }

    // Cleanup
    close(master_to_child_pipe[1]);
    close(child_to_master_pipe[0]);
    wait(NULL);
    return 0;
}
