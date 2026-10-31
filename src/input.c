#include "terminal.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void input_init(void) {
    /* Terminal input initialized via terminal_init */
}

bool input_poll(input_event_t *ev) {
    if (!ev) return false;
    memset(ev, 0, sizeof(*ev));

    unsigned char buf[64];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (n <= 0) return false;
    buf[n] = '\0';

    /* Parse ANSI / SGR escape sequences */
    if (buf[0] == '\033') {
        if (n >= 3 && buf[1] == '[' && buf[2] == '<') {
            /* SGR Mouse report: \033[<b;x;yM or \033[<b;x;ym */
            int btn = 0, px = 0, py = 0;
            char action_char = 'M';
            if (sscanf((char *)buf + 3, "%d;%d;%d%c", &btn, &px, &py, &action_char) >= 3) {
                if (action_char == 'M' && (btn == 0 || btn == 32 || btn == 64)) {
                    ev->action = INPUT_MOUSE_PAINT;
                    ev->target_x = px - 1;
                    /* Map terminal row to world vertical cell coordinate (2 cells per row) */
                    ev->target_y = (py - 1) * 2;
                    ev->is_dragging = (btn == 32);
                    return true;
                }
            }
        }
        return false;
    }

    /* Single key strokes */
    char c = buf[0];
    switch (c) {
        case 'q':
        case 'Q':
        case 3: /* Ctrl+C */
            ev->action = INPUT_QUIT;
            return true;
        case ' ':
            ev->action = INPUT_PAUSE;
            return true;
        case 'c':
        case 'C':
            ev->action = INPUT_CLEAR;
            return true;
        case 'n':
        case 's':
            ev->action = INPUT_STEP;
            return true;
        case '1':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_SAND;
            return true;
        case '2':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_WATER;
            return true;
        case '3':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_LAVA;
            return true;
        case '4':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_ACID;
            return true;
        case '5':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_GUNPOWDER;
            return true;
        case '6':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_FIRE;
            return true;
        case '7':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_STONE;
            return true;
        case '8':
        case '0':
            ev->action = INPUT_SELECT_ELEMENT;
            ev->selected_element = TYPE_EMPTY;
            return true;
        case '+':
        case '=':
            ev->action = INPUT_BRUSH_INC;
            return true;
        case '-':
        case '_':
            ev->action = INPUT_BRUSH_DEC;
            return true;
        default:
            break;
    }

    return false;
}
