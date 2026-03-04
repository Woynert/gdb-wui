#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "raygui.h"
#include "strbuf.h"
#include "strview.h"
#include "portable_utils.h"
#include "cli_prompt.h"

#include "gui.h"
#include "gdb_woy_api.h"
#include "wui_state.h"
#include "ipc.h"


int main (void) {
    printf("Hello there\n");
    printf("PYTHON_CODE(len %zu), [%.*s]\n", PYTHON_CODE_len, 100, PYTHON_CODE);

    strview_t python_code_view = {
        .data = PYTHON_CODE,
        .size = (int)PYTHON_CODE_len
    };

    WuiState wui_state = { 0 };
    WuiState_init(&wui_state);

    IPCCtx ipc_ctx = { 0 };
    IPC_launch_gdb(&ipc_ctx);

    IPCReader reader = { 0 };
    IPCReader_init(&reader);

    CliPrompt cli_prompt = { 0 };
    CliPrompt_setup(&cli_prompt);

    strbuf_space_t(GDB_BUFFER_SIZE) _aux_str = STRBUF_STATIC_INIT(GDB_BUFFER_SIZE);
    strbuf_t *aux_str = (strbuf_t*)(&_aux_str);

    int error;
    error = IPC_launch_gdb(&ipc_ctx);
    ASSERT(error == 0);

    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
    error = IPC_write_cmd(&ipc_ctx, cstr("python\n"));
    error = IPC_write_cmd(&ipc_ctx, python_code_view);
    error = IPC_write_cmd(&ipc_ctx, cstr("end\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_HIDE, NULL, 0);

    error = IPC_write_cmd(&ipc_ctx, cstr("file ../smb-raylib/build/3djump\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);

    error = IPC_write_cmd(&ipc_ctx, cstr("b main\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);

    error = IPC_write_cmd(&ipc_ctx, cstr("b Server_physic_step\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);

    error = IPC_write_cmd(&ipc_ctx, cstr("run\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);


    error = IPC_write_cmd(&ipc_ctx, cstr("py woy_get_breakpoints()\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_READ_WOY_BREAKPOINTS, &wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(&ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);


    error = IPC_write_cmd(&ipc_ctx, cstr("py woy_locals()\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_READ_WOY_LOCALS, &wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(&ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);

    // find a symbol of type struct
    /*for (size_t i = 0; i < wui_state.symbol_tree.nodes.size; ++i) {*/
        /*WuiSymbol *symbol = &wui_state.symbol_tree.nodes.items[i].item;*/
        /*if (symbol->basic_type == 3) {*/
            /*uint node_id = wui_state.symbol_tree.nodes.items[i].id;*/

            /*printf("Found struct [%s]\n", symbol->symbol_name.cstr);*/

            /*{*/
                /*strbuf_t *tmp = &symbol->symbol_name;*/
                /*strbuf_cat(*/
                    /*&aux_str,*/
                    /*cstr("py woy_query_symbol(\""),*/
                    /*strbuf_view(&tmp),*/
                    /*cstr("\")\n")*/
                /*);*/
            /*}*/

            /*printf("QUERY string [%s]\n", aux_str->cstr);*/

/*[>error = IPC_write_cmd(&ipc_ctx, cstr("py woy_query_symbol(\"client1_gs.ray_trails\")\n"));<]*/
/*error = IPC_write_cmd(&ipc_ctx, strbuf_view(&aux_str));*/
/*ASSERT(error == 0);*/
/*IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_READ_WOY_LOCALS, &wui_state, node_id);*/
/*// After calling a 'WOY API' command, clear the GDB previous command*/
/*error = IPC_write_cmd(&ipc_ctx, cstr("echo\n"));*/
/*ASSERT(error == 0);*/
/*IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);*/

            /*break;*/
        /*}*/
    /*}*/

    /*for (size_t i = 0; i < wui_state.symbol_tree.nodes.size; ++i) {*/
        /*WuiSymbol *symbol = &wui_state.symbol_tree.nodes.items[i].item;*/
        /*if (wui_state.symbol_tree.nodes.items[i].id == 7) {*/
            /*uint node_id = wui_state.symbol_tree.nodes.items[i].id;*/

            /*printf("Found struct [%s]\n", symbol->symbol_name.cstr);*/

            /*{*/
                /*strbuf_t *tmp = &symbol->symbol_name;*/
                /*strbuf_cat(*/
                    /*&aux_str,*/
                    /*cstr("py woy_query_symbol(\""),*/
                    /*strbuf_view(&tmp),*/
                    /*cstr("\")\n")*/
                /*);*/
            /*}*/

            /*printf("QUERY string [%s]\n", aux_str->cstr);*/

/*[>error = IPC_write_cmd(&ipc_ctx, cstr("py woy_query_symbol(\"client1_gs.ray_trails\")\n"));<]*/
/*error = IPC_write_cmd(&ipc_ctx, strbuf_view(&aux_str));*/
/*ASSERT(error == 0);*/
/*IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_READ_WOY_LOCALS, &wui_state, node_id);*/
/*// After calling a 'WOY API' command, clear the GDB previous command*/
/*error = IPC_write_cmd(&ipc_ctx, cstr("echo\n"));*/
/*ASSERT(error == 0);*/
/*IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);*/

            /*break;*/
        /*}*/
    /*}*/


    /*error = IPC_write_cmd(&ipc_ctx, cstr("py woy_query_symbol(\"client1_gs.ray_trails\")\n"));*/
    /*ASSERT(error == 0);*/
    /*IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_READ_WOY_QUERY, &wui_state, 1);*/
    /*// After calling a 'WOY API' command, clear the GDB previous command*/
    /*error = IPC_write_cmd(&ipc_ctx, cstr("echo\n"));*/
    /*ASSERT(error == 0);*/
    /*IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);*/


    // Raylib stuff
    const int screenWidth = 400;
    const int screenHeight = 400;
    InitWindow(screenWidth, screenHeight, "WUI");
    SetTargetFPS(60);
    GUI_init_global_context();

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {

        GUI_draw_all(&wui_state);

    /*while(true) {*/
        /*sleep_ms(50);*/
        /*CliPrompt_print_line(&cli_prompt, "i\n");*/
        // Logic loop

        while (CliPrompt_handle_prompt(&cli_prompt)) {

            {
                strbuf_t *tmp = &cli_prompt.input;
                strbuf_cat(&aux_str, strbuf_view(&tmp), cstr("\n"));
            }
            error = IPC_write_cmd(&ipc_ctx, strbuf_view(&aux_str));
            ASSERT(error == 0);
            CliPrompt_clear(&cli_prompt);
            IPC_wait_for_prompt(&ipc_ctx, &reader, &cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
        }
    }

    // Cleanup

    /*kill(ipc_ctx.child_pid, SIGINT);*/
    /*Fork_cleanup(&fork_ctx);*/

    return 0;
}
