#include "stdio.h"
#include "raygui.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"
#include "portable_utils.h"
#include "sys/wait.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#define GDBUFFER_SIZE 4096


typedef struct IPCReader {
    union {
        strbuf_space_t(GDBUFFER_SIZE) _buffer;
        strbuf_t buffer;
    };
} IPCReader;


void IPCReader_init(IPCReader *reader) {
    *reader = (IPCReader) { 0 };
    STRBUF_STATIC_INIT2(GDBUFFER_SIZE, reader->_buffer);
}


/// @retval  0 OK, keep reading.
/// @retval -1 No more lines.
int _IPCReader_get_new_line(IPCReader *reader, strbuf_t *out_line) {
    strbuf_t *src = &reader->buffer;
    strview_t split_right = strbuf_view(&src);
    strview_t split_left = strview_split_first_delim(&split_right, "\n", false);
    if (split_left.size == 0 && split_right.size == 0) {
        return -1;
    }
    strbuf_assign(&out_line, split_left);
    strbuf_assign(&src, split_right);
    return 0;
}


/// @retval  0 OK, keep reading.
/// @retval -1 No more lines.
int IPCReader_read_line(IPCReader *reader, int fd, strbuf_t *out_line) {
    // got newline?
    if (_IPCReader_get_new_line(reader, out_line) == 0) { return 0; }

    // No new line, read until buffer full or can't read anymore.
    ssize_t total_bytes_read = 0;
    ssize_t bytes_read = 0;
    do {
        bytes_read = read(fd,
                reader->buffer.cstr + bytes_read,
                size_t_max(0, (size_t)reader->buffer.capacity -1 -(size_t)bytes_read));
        if (bytes_read > 0) { total_bytes_read += bytes_read; }
    } while (bytes_read > 0);

    reader->buffer.cstr[total_bytes_read] = '\0';
    {
        strbuf_t *tmp = &reader->buffer;
        strbuf_recalculate_size_as_cstr(&tmp);
    }

    // got newline?
    if (_IPCReader_get_new_line(reader, out_line) == 0) { return 0; }
    return -1;
}


typedef struct IPCCtx {
    int master_to_child_pipe[2];
    int child_to_master_pipe[2];
} IPCCtx;


/// @retuns error
int IPC_launch_gdb(IPCCtx *ctx) {
    *ctx = (IPCCtx) { 0 };

    if (pipe(ctx->master_to_child_pipe) == -1
        || pipe(ctx->child_to_master_pipe) == -1) {
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
        if (dup2(ctx->master_to_child_pipe[0], STDIN_FILENO) == -1)
            { perror("dup2"); _exit(1); };
        if (dup2(ctx->child_to_master_pipe[1], STDOUT_FILENO) == -1)
            { perror("dup2"); _exit(1); };

        // close original pipe ends
        close(ctx->master_to_child_pipe[1]);
        close(ctx->master_to_child_pipe[0]);
        close(ctx->child_to_master_pipe[0]);
        close(ctx->child_to_master_pipe[1]);

        // die on parent exit
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        execlp("gdb", "gdb", NULL);

        // if all fails
        perror("exec");
        exit(1);
    }

    close(ctx->master_to_child_pipe[0]);
    close(ctx->child_to_master_pipe[1]);

    // non-blocking
    int flags = fcntl(ctx->child_to_master_pipe[0], F_GETFL, 0);
    fcntl(ctx->child_to_master_pipe[0], F_SETFL, flags | O_NONBLOCK);

    return 0;
}


void IPC_process(IPCCtx *ctx) {
}


void IPC_cleanup(IPCCtx *ctx) {
    close(ctx->master_to_child_pipe[1]);
    close(ctx->child_to_master_pipe[0]);
    wait(NULL);
}


int IPC_write_cmd(IPCCtx *ctx, strview_t cmd) {
    long written_bytes = write(
            ctx->master_to_child_pipe[1], cmd.data, (size_t)cmd.size);
    (void)written_bytes;
    printf("WRITE CMD: (%s) %ld bytes were written\n",
            written_bytes == cmd.size ? "success" : "failure", written_bytes);
    return written_bytes == cmd.size ? 0 : -1;
}


int main (void) {
    printf("Hello there\n");

    IPCCtx ipc_ctx = { 0 };
    IPC_launch_gdb(&ipc_ctx);

    IPCReader reader = { 0 };
    IPCReader_init(&reader);

    strbuf_space_t(GDBUFFER_SIZE) _aux_str = STRBUF_STATIC_INIT(GDBUFFER_SIZE);
    strbuf_t *aux_str = (strbuf_t*)(&_aux_str);

    int error;
    error = IPC_launch_gdb(&ipc_ctx);
    ASSERT(error == 0);

    // Write a command
    error = IPC_write_cmd(&ipc_ctx, cstr("file ../smb-raylib/build/3djump\n"));
    ASSERT(error == 0);

    while(true) {
        sleep_ms(50);

        error = IPCReader_read_line(
                &reader, ipc_ctx.child_to_master_pipe[0], aux_str);

        if (error == 0) {
            printf("--|%s\n", aux_str->cstr);
        } else {
            printf("\n");
            printf("................\n");
            printf("No output yet...\n");
            printf("................\n");
            printf("\n");

            error = IPC_write_cmd(&ipc_ctx, cstr("b main\n"));
            ASSERT(error == 0);
        }
    }

    // Cleanup

    /*Fork_cleanup(&fork_ctx);*/

    return 0;
}
