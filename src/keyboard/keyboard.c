#include "keyboard.h"
#include "../lib-header/portio.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"


const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

static struct KeyboardDriverState keyboard_state;

void keyboard_state_activate(){
  keyboard_state.keyboard_input_on = TRUE;
}

void keyboard_state_deactivate() {
  keyboard_state.keyboard_input_on = FALSE;
}

void get_keyboard_buffer(char *buf) {
  memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
}

bool is_keyboard_blocking() {
  return keyboard_state.keyboard_input_on;
}


void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on) {
        keyboard_state.buffer_index = 0;
    } 
    else {
        uint8_t scancode = in(KEYBOARD_DATA_PORT);
        char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        uint8_t cursor_x, cursor_y;
        static bool last_key_pressed = FALSE;
        static bool make_code = FALSE;
        // char* framebuffer = (char*) 0xB8000;

        if (!is_keyboard_blocking()) {
            keyboard_state.buffer_index = 0;
        }
        else {
            if (keyboard_state.buffer_index == 0) {
                memset(keyboard_state.keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
            }
            if (mapped_char != 0) {
                last_key_pressed = mapped_char;
                make_code = TRUE;
            }
            else if (make_code) {
                switch (last_key_pressed) {
                    case '\n':
                        framebuffer_get_cursor(&cursor_x, &cursor_y);
                        keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
                        keyboard_state.buffer_index = 0;
                        framebuffer_set_cursor(cursor_x + 1, 0);
                        keyboard_state_deactivate();
                        break;
                    case '\b':
                        framebuffer_get_cursor(&cursor_x, &cursor_y);
                        if (keyboard_state.buffer_index > 0) {
                            keyboard_state.buffer_index--;
                            if (cursor_y == 0) {
                                if (cursor_x > 0) {
                                    framebuffer_set_cursor(cursor_x - 1, 79);
                                    framebuffer_write(cursor_x - 1, 79, ' ', 0xF, 0);
                                } 
                            } else {
                                framebuffer_set_cursor(cursor_x, cursor_y - 1);
                                framebuffer_write(cursor_x, cursor_y - 1, ' ', 0xF, 0);
                                // for (int i = cursor_y; i < 254; i++) {
                                //     char c = *(framebuffer + ((cursor_x * 80) + i) * 2);
                                //     framebuffer_write(cursor_x, i - 1, c, 0xF, 0);
                                //     framebuffer_write(cursor_x, i, 0, 0xF, 0);
                                // }
                            }
                        }
                        break;    
                    default:
                        keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = last_key_pressed;
                        framebuffer_get_cursor(&cursor_x, &cursor_y);
                        if (cursor_y == 79) {
                            // if (keyboard_state.keyboard_buffer[keyboard_state.buffer_index - 1] == '\n') {
                            //     for (int i = cursor_y - 1; i >= 0; i--) {
                            //         char temp_char = *(framebuffer + ((cursor_x * 80) + i) * 2);
                            //         framebuffer_write(cursor_x, i + 1, temp_char, 0x0F, 0x00);
                            //     }
                            // }
                            framebuffer_write(cursor_x, cursor_y, last_key_pressed, 0x0F, 0x00);
                            framebuffer_set_cursor(cursor_x + 1, 0);
                            cursor_x++;
                            cursor_y = 0;
                        } else {
                            // if (cursor_y < keyboard_state.buffer_index) {
                            //     for (int i = 254; i > cursor_y; i--) {
                            //         char temp_char = *(framebuffer + ((cursor_x * 80) + i) * 2);
                            //         framebuffer_write(cursor_x, i + 1, temp_char, 0x0F, 0x00);
                            //     }
                            // }
                            framebuffer_write(cursor_x, cursor_y, last_key_pressed, 0x0F, 0x00);
                            framebuffer_set_cursor(cursor_x, cursor_y + 1);
                            cursor_y++;
                        }
                        break;
                }
                make_code = FALSE;
            }
            switch (scancode) {
                case EXT_SCANCODE_RIGHT:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    uint8_t currentBaris = cursor_x;
                    uint8_t nBaris = keyboard_state.buffer_index / 80;
                    uint8_t baris = currentBaris - nBaris;
                    uint8_t kolom = keyboard_state.buffer_index % 80;
                    if (keyboard_state.buffer_index > 79) {
                        if (cursor_x <= currentBaris && cursor_x >= baris) {
                            if (cursor_y == 79) {
                                if (cursor_x < 24) {
                                    framebuffer_set_cursor(cursor_x + 1, 0);
                                }
                            }
                            else if (cursor_y < 79) {
                                framebuffer_set_cursor(cursor_x, cursor_y + 1);
                            }
                        }
                        else if (cursor_x == currentBaris - 1) {
                            if (cursor_y < kolom) {
                                framebuffer_set_cursor(cursor_x, cursor_y + 1);
                            } else {
                                framebuffer_set_cursor(cursor_x, cursor_y);
                            }
                        }
                    }
                    else {
                        if (cursor_y < keyboard_state.buffer_index) {
                            framebuffer_set_cursor(cursor_x, cursor_y + 1);
                        } else {
                            framebuffer_set_cursor(cursor_x, cursor_y);
                        }
                    }
                    break;
                case EXT_SCANCODE_LEFT:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    uint8_t barisL = cursor_x - (keyboard_state.buffer_index / 80);
                    if (keyboard_state.buffer_index > 80) {
                        if (cursor_x != barisL) {
                            if (cursor_y == 0) {
                                if (cursor_x > 0) {
                                    framebuffer_set_cursor(cursor_x - 1, 79);
                                }
                            } else {
                                framebuffer_set_cursor(cursor_x, cursor_y - 1);
                            }
                        }
                    } else {
                        if (cursor_y > 0) {
                            framebuffer_set_cursor(cursor_x, cursor_y - 1);
                        } else {
                            framebuffer_set_cursor(cursor_x, cursor_y);
                        }
                    }
                    break;
                case EXT_SCANCODE_UP:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    framebuffer_set_cursor(cursor_x, cursor_y);
                    break;
                case EXT_SCANCODE_DOWN:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    framebuffer_set_cursor(cursor_x, cursor_y);
                    break;
                default:
                    break;
            }
        }
    }
    pic_ack(IRQ_KEYBOARD);
}