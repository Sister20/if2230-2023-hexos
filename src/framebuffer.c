#include "lib-header/framebuffer.h"
#include "portio.c"
#include "stdmem.c"
/**
 * Terminal framebuffer
 * Resolution: 80x25
 * Starting at MEMORY_FRAMEBUFFER,
 * - Even number memory: Character, 8-bit
 * - Odd number memory:  Character color lower 4-bit, Background color upper 4-bit
*/

/**
 * Set framebuffer character and color with corresponding parameter values.
 * More details: https://en.wikipedia.org/wiki/BIOS_color_attributes
 *
 * @param row Vertical location (index start 0)
 * @param col Horizontal location (index start 0)
 * @param c   Character
 * @param fg  Foreground / Character color
 * @param bg  Background color
 */
void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg){
    // uint16_t *mem = (uint16_t *)MEMORY_FRAMEBUFFER;
    // memset(mem, 0, row * 25 + col);
    // uint16_t idx = row * 80 + col;
    // mem[idx] = c;
    // mem[idx + 1] = ((fg & 0x0F) << 4) | (bg & 0x0F);
    // out(mem[idx], 0x00E);
    uint16_t offset = 2 * (row * 80 + col);
    memset(MEMORY_FRAMEBUFFER + offset, c, 1);
    memset(MEMORY_FRAMEBUFFER + offset + 1, bg << 4 | fg, 1);
}

/**
 * Set cursor to specified location. Row and column starts from 0
 * 
 * @param r row
 * @param c column
*/
void framebuffer_set_cursor(uint8_t r, uint8_t c){
    uint16_t pos = r * 80 + c;

    // set cursor position
    out(CURSOR_PORT_CMD, 0x00F);
    out(CURSOR_PORT_DATA, pos & 0x00FF);
    out(CURSOR_PORT_CMD, 0x00E);
    out(CURSOR_PORT_DATA, (pos >> 8) & 0x00FF);

}

/** 
 * Set all cell in framebuffer character to 0x00 (empty character)
 * and color to 0x07 (gray character & black background)
 * 
 */
void framebuffer_clear(void){
    memset(MEMORY_FRAMEBUFFER, 0x00, 80 * 25 * 2);
    for (uint8_t row = 0; row < 25; row++) {
        for (uint8_t col = 0; col < 80; col++) {
            framebuffer_write(row, col, ' ', 0x07, 0x00);
        }
    }
}