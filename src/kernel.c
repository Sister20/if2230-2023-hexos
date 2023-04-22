#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/framebuffer.h"
#include "keyboard/keyboard.h"

#include "interrupt/idt.h"
#include "interrupt/interrupt.h"
#include "filesystem/fat32.h"
#include "filesystem/disk.h"

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
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    // __asm__("int $0x4");                                 // INTERRUPT
    initialize_filesystem_fat32();                          // FILESYSTEM
    keyboard_state_activate();                              // KEYBOARD DRIVER

    
    

    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = i + 'a';

    struct FAT32DriverRequest request = {
        .buf                   = cbuf,
        .name                  = "ikanaide",
        .ext                   = "uwu",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;

    write(request);  // Create folder "ikanaide"
    memcpy(request.name, "kano1\0\0\0", 8);
    write(request);  // Create folder "kano1"
    // memcpy(request.name, "ikanaide", 8);
    // memcpy(request.ext, "\0\0\0", 3);
    // delete(request); // Delete first folder, thus creating hole in FS

    memcpy(request.name, "daijoubu", 8);
    request.buffer_size = 5*CLUSTER_SIZE;
    write(request);  // Create fragmented file "daijoubu"

    // struct ClusterBuffer readcbuf;
    // read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
    // If read properly, readcbuf should filled with 'a'

    // request.buffer_size = CLUSTER_SIZE;
    // // read(request);   // Failed read due not enough buffer size
    // request.buffer_size = 5*CLUSTER_SIZE;
    // // read(request);   // Success read on file "daijoubu"

    while (TRUE);
}

