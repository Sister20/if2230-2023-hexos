#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/framebuffer.h"

#include "interrupt/idt.h"
#include "interrupt/interrupt.h"

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);

    /* Milestone 1 Testing */
    // framebuffer_clear();
    // framebuffer_write(3, 8,  'H', 0, 0xF);
    // framebuffer_write(3, 9,  'e', 0, 0xF);
    // framebuffer_write(3, 10, 'x', 0, 0xF);
    // framebuffer_write(3, 11, 'O', 0, 0xF);
    // framebuffer_write(3, 12, 'S', 0, 0xF);
    
    // framebuffer_write(5, 8,  'H', 0, 0xF);
    // framebuffer_write(5, 9,  'e', 0, 0xF);
    // framebuffer_write(5, 10, 'l', 0, 0xF);
    // framebuffer_write(5, 11, 'l', 0, 0xF);
    // framebuffer_write(5, 12, 'o', 0, 0xF);
    // framebuffer_write(5, 13, ',', 0, 0xF);
    // framebuffer_write(5, 14, ' ', 0, 0xF);
    // framebuffer_write(5, 15, 'W', 0, 0xF);
    // framebuffer_write(5, 16, 'o', 0, 0xF);
    // framebuffer_write(5, 17, 'r', 0, 0xF);
    // framebuffer_write(5, 18, 'l', 0, 0xF);
    // framebuffer_write(5, 19, 'd', 0, 0xF);
    // framebuffer_write(5, 20, '!', 0, 0xF);

    /* Milestone 2 Testing */
    /* Load IDT & Testing Interrupt*/
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    __asm__("int $0x4");

    // framebuffer_set_cursor(5, 19);
    while (TRUE);
}
