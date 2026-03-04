#ifndef IPC_H
#define IPC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>

#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"
#include "cli_prompt.h"

#include "woy_interpreter.h"

#define GDB_BUFFER_SIZE 4096


typedef struct IPCReader {
    union {
        strbuf_space_t(GDB_BUFFER_SIZE) _buffer;
        strbuf_t buffer;
    };
} IPCReader;


void IPCReader_init(IPCReader *reader) {
    *reader = (IPCReader) { 0 };
    STRBUF_STATIC_INIT2(GDB_BUFFER_SIZE, reader->_buffer);
}


/// @retval  0 Found new line.
/// @retval -1 Not found.
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

/// @retval  1 Found gdb prompt.
/// @retval  0 Found new line.
/// @retval -1 Not found.
int _IPCReader_find_gdb_prompt(IPCReader *reader, strbuf_t *out_line) {
    strbuf_t *src = &reader->buffer;

    strview_t split_right = strbuf_view(&src);
    strview_t split_left = strview_find_first_strview(split_right, cstr("\n(gdb) "));
    if (!strview_is_valid(split_left)) {
        return -1;
    }

    strbuf_assign(&src, split_right);
    return 0;
}

/// Goal: Look for "(gdb) " And there has to be no more buffer to read.
bool _IPCReader_is_line_gdb_prompt(strview_t line) {
    strview_t line_end = strview_split_index(&line, -6);
    return (strview_compare(line_end, cstr("(gdb) ")) == 0);
}


/// @retval  1 OK, Found GDB prompt, stop reading.
/// @retval  0 OK, Found newline, keep reading.
/// @retval -1 No more lines.
int _IPCReader_read_line(IPCReader *reader, int fd, strbuf_t *out_line) {
    strbuf_assign(&out_line, cstr(""));

    // got newline?
    if (_IPCReader_get_new_line(reader, out_line) == 0) {
        // reached EOF
        if (reader->buffer.size == 0) {
            if (_IPCReader_is_line_gdb_prompt(strbuf_view(&out_line))) {
                return 1;
            }
        }
        return 0;
    }

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
    if (_IPCReader_get_new_line(reader, out_line) == 0) {
        // reached EOF
        if (reader->buffer.size == 0) {
            if (_IPCReader_is_line_gdb_prompt(strbuf_view(&out_line))) {
                return 1;
            }
        }
        return 0;
    }
    return -1;
}


typedef struct IPCCtx {
    int child_pid;
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
    ctx->child_pid = pid;
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

enum IPC_WAIT_DO {
    IPC_WAIT_DO_NOTHING,
    IPC_WAIT_DO_HIDE,
    IPC_WAIT_DO_READ_WOY_BREAKPOINTS,
    IPC_WAIT_DO_READ_WOY_LOCALS,
    IPC_WAIT_DO_READ_WOY_QUERY,
};


/// @param symbol_id. Optional. Only if ipc_do == IPC_WAIT_DO_READ_WOY_LOCALS
///                             or ipc_do == IPC_WAIT_DO_READ_WOY_QUERY
/// BLOCKS until gdb prompt is found
/// @note Prints all output
void IPC_wait_for_prompt(
    IPCCtx *ipc_ctx,
    IPCReader* reader,
    CliPrompt *cli_prompt,
    enum IPC_WAIT_DO ipc_do,
    WuiState *wui_state,
    uint symbol_id
) {
    /// TODO: Sort arguments

    strbuf_space_t(GDB_BUFFER_SIZE) _aux_str = STRBUF_STATIC_INIT(GDB_BUFFER_SIZE);
    strbuf_t *aux_str = (strbuf_t*)(&_aux_str);


    /*TODO: if ipc_do != NOTHING or HIDE*/
    WoyInterp woy_interp = { 0 };
    WoyInterp_init(&woy_interp);
    WoyInterp_reset(&woy_interp);


    int error;
    while(true) {
        sleep_ms(5);
        error = _IPCReader_read_line(
                reader, ipc_ctx->child_to_master_pipe[0], aux_str);
        if (error == 0) {
            if (ipc_do != IPC_WAIT_DO_HIDE) {
                CliPrompt_print_line(cli_prompt, "|- %s\n", aux_str->cstr);
            }
            if (ipc_do != IPC_WAIT_DO_NOTHING) {
                WoyInterp_push_line(&woy_interp, strbuf_view(&aux_str));
            }
        }

        // finish reading
        else if (error == 1) {
            if (ipc_do == IPC_WAIT_DO_HIDE) {
                CliPrompt_print_line(cli_prompt, "|- %s\n", "(gdb)");
            }
            else {
                CliPrompt_print_line(cli_prompt, "|- %s\n", aux_str->cstr);
            }
            CliPrompt_print_line(cli_prompt, "|- %s\n", "<<<INSERTING>>>");

            if (ipc_do == IPC_WAIT_DO_READ_WOY_BREAKPOINTS) {
                WoyInterp_interpret_breakpoints(&woy_interp, wui_state);
            }
            else if (ipc_do == IPC_WAIT_DO_READ_WOY_LOCALS) {
                int err = WoyInterp_interpret_symbols(&woy_interp, wui_state, symbol_id);
                ASSERT(err == 0);
            }
            WoyInterp_reset(&woy_interp);

            return;
        }
    }
}


int IPC_write_cmd(IPCCtx *ctx, strview_t cmd) {
    long written_bytes = write(
            ctx->master_to_child_pipe[1], cmd.data, (size_t)cmd.size);
    (void)written_bytes;
    printf("\n");
    return written_bytes == cmd.size ? 0 : -1;
}

#endif // !IPC_H
