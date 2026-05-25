#ifndef BETTER_MOUSE_INPUT_H
#define BETTER_MOUSE_INPUT_H

#include "GLFW/glfw3.h"
#include "raylib.h"
#include "stdbool.h"
#include "assert.h"
#include "stdio.h"

/*
    USAGE: Modify your main loop to include this functions:

        InitWindow(...);
        BetterMouse_hook_events(); <-- Call after InitWindow.

        while (!WindowShouldClose()) {
            ... Drawing ...
            BetterMouse_consume_all(); <-- Call it just before EndDrawing.
            EndDrawing();
        }

    Then use API like BetterMouse_is_pressed(MOUSE_BUTTON_LEFT).
    Currently it supports LEFT, RIGHT, and MIDDLE button from raylib:
    typedef enum {
        MOUSE_BUTTON_LEFT    = 0,
        MOUSE_BUTTON_RIGHT   = 1,
        MOUSE_BUTTON_MIDDLE  = 2,
        ...
    } MouseButton;
 */

/* 0 pressed, 1 held, 2 released */
static bool BetterMouse__input [MOUSE_BUTTON_MIDDLE+1][3] = { 0 };

void BetterMouse__mouse_additive_cb(GLFWwindow* w, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            BetterMouse__input[MOUSE_BUTTON_LEFT][0] = true;
            BetterMouse__input[MOUSE_BUTTON_LEFT][1] = true;
        } else if (action == GLFW_RELEASE) {
            BetterMouse__input[MOUSE_BUTTON_LEFT][2] = true;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            BetterMouse__input[MOUSE_BUTTON_RIGHT][0] = true;
            BetterMouse__input[MOUSE_BUTTON_RIGHT][1] = true;
        } else if (action == GLFW_RELEASE) {
            BetterMouse__input[MOUSE_BUTTON_RIGHT][2] = true;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            BetterMouse__input[MOUSE_BUTTON_MIDDLE][0] = true;
            BetterMouse__input[MOUSE_BUTTON_MIDDLE][1] = true;
        } else if (action == GLFW_RELEASE) {
            BetterMouse__input[MOUSE_BUTTON_MIDDLE][2] = true;
        }
    }
}

/* Call after InitWindow. */
void BetterMouse_hook_events(void) {
    GLFWwindow* w = (GLFWwindow*)GetWindowHandle();
    GLFWmousebuttonfun callback = glfwSetMouseButtonCallback(w, BetterMouse__mouse_additive_cb);
    assert(callback != NULL);
    int error_code = glfwGetError(NULL);
    if (error_code != GLFW_NO_ERROR) {
        printf("Failed to set mouse callback. Error code: %d\n", error_code);
    }
}

/* Call at frame end. */
void BetterMouse_consume_all(void) {
    for (int i = MOUSE_BUTTON_LEFT; i <= MOUSE_BUTTON_MIDDLE; ++i) {
        BetterMouse__input[i][0] = false;
        if (BetterMouse__input[i][2]) {
            /* Held is reset only when Released was triggered. */
            BetterMouse__input[i][1] = false;
        }
        BetterMouse__input[i][2] = false;
    }
}

void BetterMouse_consume(MouseButton button, bool trigger_release) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    BetterMouse__input[button][0] = false;
    BetterMouse__input[button][1] = false;
    BetterMouse__input[button][2] = trigger_release;
}

bool BetterMouse_is_pressed(MouseButton button) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    return BetterMouse__input[button][0];
}

bool BetterMouse_is_held(MouseButton button) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    return BetterMouse__input[button][1];
}

bool BetterMouse_is_released(MouseButton button) {
    assert(button <= MOUSE_BUTTON_MIDDLE);
    return BetterMouse__input[button][2];
}

#endif // !BETTER_MOUSE_INPUT_H
