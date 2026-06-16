#ifndef IPC_H
#define IPC_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "wui_state.h"
#include "strbuf.h"
#include "strview.h"
#include "cli_prompt.h"

#define GDB_BUFFER_SIZE 4096

enum IPC_WAIT_DO {
    IPC_WAIT_DO_NOTHING,
    IPC_WAIT_DO_HIDE,
    IPC_WAIT_DO_READ_WOY_BREAKPOINTS,
    IPC_WAIT_DO_READ_WOY_LOCALS,
    IPC_WAIT_DO_READ_WOY_QUERY,
    IPC_WAIT_DO_READ_WOY_FILE_LINE,
};

typedef struct IPCReader {
    strbuf_t *buffer;
} IPCReader;

typedef struct IPCCtx {
    int child_pid;
    int master_to_child_pipe[2];
    int child_to_master_pipe[2];
} IPCCtx;

void IPCReader_init(IPCReader *reader);
void IPCReader_free(IPCReader *reader);
bool IPCReader_is_line_gdb_prompt(strview_t line);
int IPCReader_get_curr_line(const IPCReader *reader, strview_t *line_out);
void IPCReader_read_lines(IPCReader *reader, int fd);
void IPCReader_consume_line(IPCReader *reader);
int IPC_launch_gdb(IPCCtx *ctx);
void IPC_cleanup(IPCCtx *ctx);
int IPC_wait_for_prompt(
    IPCCtx *ipc_ctx,
    IPCReader* reader,
    CliPrompt *cli_prompt,
    enum IPC_WAIT_DO ipc_do,
    WuiState *wui_state,
    uint symbol_id
);
int IPC_write_cmd(IPCCtx *ctx, strview_t cmd);

#endif // !IPC_H

#include "have_lsp.h"
#if (defined IPC_H_IMPLEMENTATION || defined HAVE_LSP) && !defined IPC_H_DONE
#define IPC_H_DONE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "strbuf_extra.h"
#include "woy_interpreter.h"


void IPCReader_init(IPCReader *reader) {
    *reader = (IPCReader) { 0 };
    reader->buffer = strbuf_create(0, NULL);
}


void IPCReader_free(IPCReader *reader) {
    strbuf_destroy(&reader->buffer);
    *reader = (IPCReader) { 0 };
}


/// Try Look for "(gdb) " and there has to be no more buffer to read.
bool IPCReader_is_line_gdb_prompt(strview_t line) {
    strview_t line_end = strview_split_index(&line, -6);
    return (strview_compare(line_end, cstr("(gdb) ")) == 0);
}


int IPCReader_get_curr_line(const IPCReader *reader, strview_t *line_out) {
    if (reader->buffer->size <= 0) { return -1; }

    strview_t right = strbuf_view2(reader->buffer);
    strview_t left = strview_split_first_delim(&right, "\n", false);
    if (strview_is_valid(left)) {
        *line_out = left;
        return 0;
    }

    *line_out = strbuf_view2(reader->buffer);
    return 0;
}


void IPCReader_read_lines(IPCReader *reader, int fd) {
    if (reader->buffer->size > 0) { return; }

    /* @note: Potential to grow indefinitely. Consider upper limit. */
    char buf[256];
    for(;;) {
        int bytes_read = (int)read(fd, buf, sizeof(buf) / sizeof(buf[0]));
        if (bytes_read <= 0) { break; }
        strview_t view = { .data = buf, .size = bytes_read };
        strbuf_append(&reader->buffer, view);
    }
}


/* Advaces buffer until after \n. */
void IPCReader_consume_line(IPCReader *reader) {
    if (reader->buffer->size <= 0) { return; }

    strview_t right = strbuf_view(&reader->buffer);
    strview_t left = strview_split_first_delim(&right, "\n", false);

    /* Couldn't split, that means this is the last line. */
    if (!strview_is_valid(left)) {
        strbuf_empty(&reader->buffer);
        return;
    }

    strbuf_assign(&reader->buffer, right);
}


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


void IPC_cleanup(IPCCtx *ctx) {
    close(ctx->master_to_child_pipe[1]);
    close(ctx->child_to_master_pipe[0]);
    wait(NULL);
}


/// @param symbol_id [optional] Only if ipc_do == IPC_WAIT_DO_READ_WOY_LOCALS
///                             or ipc_do == IPC_WAIT_DO_READ_WOY_QUERY
/// @note BLOCKS until gdb prompt is found or Timeout.
/// @note Prints all output
/// @returns Error.
int IPC_wait_for_prompt(
    IPCCtx *ipc_ctx,
    IPCReader* reader,
    CliPrompt *cli_prompt,
    enum IPC_WAIT_DO ipc_do,
    WuiState *wui_state,
    uint symbol_id
) {
    /// TODO: Sort arguments

    int return_err = 0;

    /*TODO: if ipc_do != NOTHING or HIDE*/
    WoyInterp woy_interp = { 0 };
    WoyInterp_init(&woy_interp);
    WoyInterp_reset(&woy_interp);

    const long TIMEOUT_MS = 5000;
    long start = get_system_ms();

    for(;;) {

        strview_t line = STRVIEW_INVALID;
        IPCReader_read_lines(reader, ipc_ctx->child_to_master_pipe[0]);
        int err = IPCReader_get_curr_line(reader, &line);

        /* No more lines. */
        if (err == -1 || !strview_is_valid(line)) {
            if (get_system_ms() > start + TIMEOUT_MS) {
                printfd("Timeout: reading from gdb output.");
                return_err = -1;
                break;
            }
            sleep_ms(5);
        }

        /* Found gdb prompt. */
        if (IPCReader_is_line_gdb_prompt(line)) {
            if (ipc_do == IPC_WAIT_DO_HIDE) {
                CliPrompt_print_line(cli_prompt, "|- %s\n", "(gdb)");
            } else {
                CliPrompt_print_line(cli_prompt, "|- %"PRIstr"\n", PRIstrarg(line));
            }
            CliPrompt_print_line(cli_prompt, "|- %s\n", "<<<INSERTING>>>");

            if (ipc_do == IPC_WAIT_DO_READ_WOY_BREAKPOINTS) {
                WoyInterp_interpret_breakpoints(&woy_interp, wui_state);
            } else if (ipc_do == IPC_WAIT_DO_READ_WOY_LOCALS) {
                return_err = WoyInterp_interpret_symbols(&woy_interp, wui_state, symbol_id);
            } else if (ipc_do == IPC_WAIT_DO_READ_WOY_FILE_LINE) {
                return_err = WoyInterp_interpret_file_line(&woy_interp, wui_state);
            }

            WoyInterp_reset(&woy_interp);
            IPCReader_consume_line(reader);
            break;
        }

        /* Found newline. */
        else if (line.size > 0) {
            if (ipc_do != IPC_WAIT_DO_HIDE) {
                CliPrompt_print_line(cli_prompt, "|- %"PRIstr"\n", PRIstrarg(line));
            }
            if (ipc_do != IPC_WAIT_DO_NOTHING) {
                WoyInterp_push_line(&woy_interp, line);
            }
        }

        IPCReader_consume_line(reader);
    }

    return return_err;
}


int IPC_write_cmd(IPCCtx *ctx, strview_t cmd) {
    long written_bytes = write(
            ctx->master_to_child_pipe[1], cmd.data, (size_t)cmd.size);
    (void)written_bytes;
    printf("\n");
    return written_bytes == cmd.size ? 0 : -1;
}

#endif // !IPC_H_IMPLEMENTATION
