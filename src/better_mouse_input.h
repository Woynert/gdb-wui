#ifndef BETTER_MOUSE_INPUT_H
#define BETTER_MOUSE_INPUT_H

#include "GLFW/glfw3.h"
#include "raylib.h"
#include "stdbool.h"
#include "assert.h"

/*
    USAGE:

    Step 1: Hook GLFW events from your application:

        void glfw_mouse_callback(GLFWwindow* w, int button, int action, int mods) {
            BetterMouse_glfw_mouse_button_callback(w, button, action, mods);
        }

        void glfw_scroll_callback(GLFWwindow* w, double xoffset, double yoffset) {
            BetterMouse_glfw_scroll_callback(w, xoffset, yoffset);
        }

        void hook_glfw_callbacks(void) {
            assert(glfwSetMouseButtonCallback(window, glfw_mouse_callback));
            assert(glfwSetScrollCallback(window, glfw_scroll_callback));
            return 0;
        }

        main (...) {
            ...
            InitWindow(...);
            hook_glfw_callbacks(); <-- Call after InitWindow.
            ...
        }

    Step 2: Modify your main loop to include this functions:

        while (!WindowShouldClose()) {
            ... Drawing ...
            BetterMouse_consume_all(); <-- Call it just before EndDrawing.
            EndDrawing();
        }

    API:

        bool BetterMouse_is_pressed(MouseButton);
        bool BetterMouse_is_held(MouseButton);
        bool BetterMouse_is_released(MouseButton);
        float BetterMouse_mouse_wheel();
        float BetterMouse_mouse_wheel_h();

    Currently it supports LEFT, RIGHT, and MIDDLE button from raylib:

        MOUSE_BUTTON_LEFT    = 0,
        MOUSE_BUTTON_RIGHT   = 1,
        MOUSE_BUTTON_MIDDLE  = 2,
 */


static struct {
    bool button [MOUSE_BUTTON_MIDDLE+1][3]; /* 0 pressed, 1 held, 2 released */
    float wheel_x;
    float wheel_y;
} BetterMouse__state = { 0 };


void BetterMouse_glfw_mouse_button_callback(GLFWwindow* w, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            BetterMouse__state.button[MOUSE_BUTTON_LEFT][0] = true;
            BetterMouse__state.button[MOUSE_BUTTON_LEFT][1] = true;
        } else if (action == GLFW_RELEASE) {
            BetterMouse__state.button[MOUSE_BUTTON_LEFT][2] = true;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            BetterMouse__state.button[MOUSE_BUTTON_RIGHT][0] = true;
            BetterMouse__state.button[MOUSE_BUTTON_RIGHT][1] = true;
        } else if (action == GLFW_RELEASE) {
            BetterMouse__state.button[MOUSE_BUTTON_RIGHT][2] = true;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            BetterMouse__state.button[MOUSE_BUTTON_MIDDLE][0] = true;
            BetterMouse__state.button[MOUSE_BUTTON_MIDDLE][1] = true;
        } else if (action == GLFW_RELEASE) {
            BetterMouse__state.button[MOUSE_BUTTON_MIDDLE][2] = true;
        }
    }
}


void BetterMouse_glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    BetterMouse__state.wheel_y = (float)yoffset;
    BetterMouse__state.wheel_x = (float)xoffset;
}


/* Call at frame end. */
void BetterMouse_consume_all(void) {
    for (int i = MOUSE_BUTTON_LEFT; i <= MOUSE_BUTTON_MIDDLE; ++i) {
        BetterMouse__state.button[i][0] = false;
        if (BetterMouse__state.button[i][2]) {
            /* Held is reset only when Released was triggered. */
            BetterMouse__state.button[i][1] = false;
        }
        BetterMouse__state.button[i][2] = false;
    }

    BetterMouse__state.wheel_x = 0;
    BetterMouse__state.wheel_y = 0;
}

void BetterMouse_consume(MouseButton button, bool trigger_release) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    BetterMouse__state.button[button][0] = false;
    BetterMouse__state.button[button][1] = false;
    BetterMouse__state.button[button][2] = trigger_release;
}

bool BetterMouse_is_pressed(MouseButton button) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    return BetterMouse__state.button[button][0];
}

bool BetterMouse_is_held(MouseButton button) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    return BetterMouse__state.button[button][1];
}

bool BetterMouse_is_released(MouseButton button) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    return BetterMouse__state.button[button][2];
}

float BetterMouse_mouse_wheel(void) {
    return BetterMouse__state.wheel_y;
}

float BetterMouse_mouse_wheel_h(void) {
    return BetterMouse__state.wheel_x;
}

#endif // !BETTER_MOUSE_INPUT_H
