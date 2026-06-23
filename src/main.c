#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "raylib.h"
#include "raygui.h"
#include "events.h"
#include "strbuf.h"
#include "strview.h"
#include "portable_utils.h"
#include "cli_prompt.h"
#include "better_mouse_input.h"

#include "gui.h"
#include "gdb_woy_api.h"
#include "wui_state.h"
#include "ipc.h"
#include "main_context.h"

/* IMPLEMENTATIONS */
#define IPC_H_IMPLEMENTATION
#include "ipc.h"


void glfw_mouse_callback(GLFWwindow* w, int button, int action, int mods) {
    BetterMouse_glfw_mouse_button_callback(w, button, action, mods);
}


void glfw_scroll_callback(GLFWwindow* w, double xoffset, double yoffset) {
    BetterMouse_glfw_scroll_callback(w, xoffset, yoffset);
}


int hook_glfw_callbacks(Ctx *ctx) {
    GLFWwindow* window = (GLFWwindow*)GetWindowHandle();
    glfwSetWindowUserPointer(window, ctx);
    if (glfwSetMouseButtonCallback(window, glfw_mouse_callback) == NULL) { return 1; }
    if (glfwSetScrollCallback(window, glfw_scroll_callback) == NULL) { return 1; }
    /*if (glfwSetKeyCallback(window, glfw_kb_callback) == NULL) { return 1; }*/
    /*if (glfwSetCharCallback(window, glfw_char_callback) == NULL) { return 1; }*/
    /*if (glfwSetWindowFocusCallback(window, glfw_focus_callback) == NULL) { return 1; }*/
    return 0;
}


int main (void) {
    printf("Hello there\n");
    printf("PYTHON_CODE(len %zu), [%.*s]\n", PYTHON_CODE_len, 100, PYTHON_CODE);

    strview_t python_code_view = {
        .data = PYTHON_CODE,
        .size = (int)PYTHON_CODE_len
    };

    Ctx ctx = { 0 };
    ctx_init(&ctx);

    WuiState *wui_state = &ctx.wui_state;
    IPCCtx *ipc_ctx = &ctx.ipc_ctx;
    IPCReader *reader = &ctx.reader;
    CliPrompt *cli_prompt = &ctx.cli_prompt;
    Textedit *textedit = &ctx.textedit;


    strbuf_space_t(GDB_BUFFER_SIZE) _aux_str = STRBUF_STATIC_INIT(GDB_BUFFER_SIZE);
    strbuf_t *aux_str = (strbuf_t*)(&_aux_str);
    int error;

    CliPrompt_setup(cli_prompt);
    error = IPC_launch_gdb(ipc_ctx);
    ASSERT(error == 0);

    // Insert custom script.

    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
    error = IPC_write_cmd(ipc_ctx, cstr("python\n"));
    error = IPC_write_cmd(ipc_ctx, python_code_view);
    error = IPC_write_cmd(ipc_ctx, cstr("end\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_HIDE, NULL, 0);

    error = IPC_write_cmd(ipc_ctx, cstr("file ../smb-raylib/build/3djump\n"));
    ASSERT(error == 0);
    ASSERT(0 == IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0));

    /*error = IPC_write_cmd(ipc_ctx, cstr("b main\n"));*/
    /*ASSERT(error == 0);*/
    /*IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);*/

    error = IPC_write_cmd(ipc_ctx, cstr("b GameState_load_world\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);

    error = IPC_write_cmd(ipc_ctx, cstr("run\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);


    error = IPC_write_cmd(ipc_ctx, cstr("py woy_get_breakpoints()\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_READ_WOY_BREAKPOINTS, wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);


    error = IPC_write_cmd(ipc_ctx, cstr("py woy_locals()\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_READ_WOY_LOCALS, wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);

    error = IPC_write_cmd(ipc_ctx, cstr("py woy_get_file_and_line()\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_READ_WOY_FILE_LINE, wui_state, 0);
    // After calling a 'WOY API' command, clear the GDB previous command
    error = IPC_write_cmd(ipc_ctx, cstr("echo\n"));
    ASSERT(error == 0);
    IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);


    // Raylib stuff
    const int screenWidth = 600;
    const int screenHeight = 700;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "WUI");
    SetTargetFPS(30);

    hook_glfw_callbacks(&ctx);
    GUI_init_global_context();

    textedit_set_draw_opts(textedit, (Textedit_Drawopts) {
        .font_width = GUI.font_width,
        .font_spacing = GUI.font_spacing,
        .font_size = GUI.font_size,
        .line_spacing = 2,
    });
    textedit_highlight_line(textedit, true, 7);
    textedit_toggle_line_numbers(textedit, true);
    textedit_set_editable(textedit, false);
    textedit_set_buffer(textedit, cstr(
        /*"#include \"stdio.h\"\n"*/
        /*"#include \"stdlibbbb.h\"\n"*/
        "[123.123.123.123.123.123.123.123.123.123.123.123.123.123.123.123.]\n"
        "1�1\n"
        "latin[ñ,ü,Ā,č,ω,Ω,Ж]\n"
        "symbols[←,✓,☃,€]\n"
        "CJK[世,界]\n"
        "Emoji[😀,🚀,🦀,🧠]\n"
        "[😀,😀,😃,😄,😁,😆,🥹,😅,😂,🤣,🥲,😊,😇,🙂,🙃,😉,😌,😍,🥰,😘,😗,😙,😚,😋,😛,😝,😜,🤪,🤨,🧐,🤓,😎,🥸,🤩,🥳,😏,😒,😞,😔,😟,😕,🙁,😣,😖,😫,😩,☺️,☹️,🙂‍↔️,🙂‍↕️]"
        "\n"
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

        GUI_draw_all(&ctx, wui_state);

    /*while(true) {*/
        /*sleep_ms(50);*/
        /*CliPrompt_print_line(cli_prompt, "i\n");*/
        // Logic loop

        while (CliPrompt_handle_prompt(cli_prompt)) {

            {
                strbuf_t *tmp = &cli_prompt->input;
                strbuf_cat(&aux_str, strbuf_view(&tmp), cstr("\n"));
            }
            error = IPC_write_cmd(ipc_ctx, strbuf_view(&aux_str));
            ASSERT(error == 0);
            CliPrompt_clear(cli_prompt);
            IPC_wait_for_prompt(ipc_ctx, reader, cli_prompt, IPC_WAIT_DO_NOTHING, NULL, 0);
        }

        /* EndDrawing() sleeps and during this time we can catch inputs. */
        WuiState_process_events(&ctx);
        BetterMouse_consume_all();
        EndDrawing();

        if (IsKeyPressed(KEY_H)) {
            textedit_reveal_line(textedit, 15);
        }
        if (IsKeyPressed(KEY_J)) {
            update_file_view_from_file_line_query(&ctx);
        }
    }

    // Cleanup

    CloseWindow();
    GUI_cleanup();
    ctx_free(&ctx);


    return 0;
}
