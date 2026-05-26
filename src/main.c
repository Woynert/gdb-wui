#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "raylib.h"
#include "raygui.h"
#include "strbuf.h"
#include "strview.h"
#include "portable_utils.h"
#include "cli_prompt.h"
#include "better_mouse_input.h"

#include "gui.h"
#include "gdb_woy_api.h"
#include "wui_state.h"
#include "ipc.h"

/*void away(int *p) {*/
    /**p = 9;*/
/*}*/


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
    const int screenWidth = 600;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "WUI");
    SetTargetFPS(30);

    BetterMouse_hook_events();
    GUI_init_global_context();

    textedit_toggle_line_numbers(&GUI.textedit, true);
    textedit_set_editable(&GUI.textedit, true);
    textedit_enable(&GUI.textedit, cstr(
        /*"#include \"stdio.h\"\n"*/
        /*"#include \"stdlibbbb.h\"\n"*/
        "[123.123.123.123.123.123.123.123.123.123.123.123.123.123.123.123.]\n"
        "1�1\n"
        "latin[ñ,ü,Ā,č,ω,Ω,Ж]\n"
        "symbols[←,✓,☃,€]\n"
        "CJK[世,界]\n"
        "Emoji[😀,🚀,🦀,🧠]\n"
        "#include \"math.hhhhh\""
        "#include \"math.h\".123.123.123.123.123.123.123.123\n"
        "#include \"math.hhhhh1\"\n"
        "#include \"math.hhhh2\"\n"
        "\n"
        "\n"
        "#include \"math.hhhh3\"\n"
        "#include \"math.hhhh4\"\n"
        "#include \"math.hhhh5\"\n"
        "[GUI_TextEdit__find_curs\nor_visual_line(textedit);"
        "\nGUI_TextEdit__find_cursor_\nvisual_line(textedit);"
        "G\nUI_TextEdit__find_cursor_vi\nsual_line(textedit);"
        "GU\nI_TextEdit__find_cursor_visu\nal_line(textedit);"
        "GUI\n_TextEdit__find_cursor_visual\n_line(textedit);"
        "GUI\n_TextEdit__find_cursor_visual_li\nne(textedit);"
        "GU\nI_TextEdit__find_cursor_visual_l\nine(textedit);"
        "GUI\n_TextEdit__find_cursor_visual_lin\ne(textedit);"
        "GUI_\nTextEdit__find_cursor_visual_l\nine(textedit);]\n"
        "#include \"math.hhhh6\"\n"
        /*"#include \"hello.world.hello.world.hello.world.hello.world\"\n"*/
        "\n"
        "int main(void)\n"
        "{\n"
        "    printf(\"Hello world\\n\");\n"
        "}\n"
        "\n"
        "\n"
        "\n"
        "EOF below\n"
        "EOF"
    ));

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

        /* ↓ EndDrawing() sleeps and during this time we can catch inputs. */
        BetterMouse_consume_all();
        EndDrawing();
    }

    // Cleanup

    CloseWindow();
    GUI_cleanup();
    WuiState_free(&wui_state);

    /*kill(ipc_ctx.child_pid, SIGINT);*/
    /*Fork_cleanup(&fork_ctx);*/

    // Buffer overflow
    /*int *list = (int*) malloc(sizeof(int) * 7);*/
    /*if (0) {*/
        /*list[8] = 888;*/
        /*free(list);*/
    /*}*/

    /*// Use after free*/
    /*int *p[10] = { 0 };*/
    /*for (int i = 0; i < 10; ++i) {*/
        /*p[0] = (int*) malloc(sizeof(int));*/
    /*}*/
    /**p[0] = 40;*/
    /*if (1) {*/
        /*free(p[1]);*/
        /**p[1] = 10;*/
    /*}*/

    /*// Signed overflow*/
    /*int x = INT_MAX;*/
    /*++x;*/

    /*// Invalid shift*/
    /*int y = 1 << (*p[0]);*/

    return 0;
}
