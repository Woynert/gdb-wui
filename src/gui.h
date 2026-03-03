#ifndef GUI

#include "stdio.h"

#include "raygui.h"
#include "raylib.h"
#include "strbuf.h"
#include "strview.h"
#include "strbuf_extra.h"

typedef struct GuiBreakpointView {
    //union
    //strbuf_t 
    int i;
} GuiBreakpointView;

void GUI_draw(void) {

    Rectangle btn = { 20, 20, 300, 20 };
    if (GuiButton(btn, "Press Me!")) {
        printf("Thank you\n");
    }
    btn.y += btn.height;
    if (GuiButton(btn, "Press Me Too!")) {
        printf("Thank you twise\n");
    }

}

#endif // !GUI
