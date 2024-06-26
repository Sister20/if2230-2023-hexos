#include "interrupt.h"
#include "idt.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/portio.h"
#include "../keyboard/keyboard.h"
#include "../lib-header/framebuffer.h"
#include "../filesystem/fat32.h"

static struct FAT32DriverState fat_state;

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);

    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = in(PIC1_DATA); 
    a2 = in(PIC2_DATA);

    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);      // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);      // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    out(PIC1_DATA, a1);
    out(PIC2_DATA, a2);
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

// USER MODE
void puts(char* str, uint32_t len, uint32_t color, uint8_t row, uint8_t col) {
    uint8_t i;
    for (i = 0; i < len; i++) {
        framebuffer_write(row, col + i, str[i], color, 0);
    }
    framebuffer_set_cursor(row, col + i);
}

void cmd_ls(struct FAT32DriverRequest request, uint8_t row, uint8_t col){
    read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
    for(int i =0;i<64; i++){
        if (fat_state.dir_table_buf.table[i].name[0] != 0x00) {
            puts(fat_state.dir_table_buf.table[i].name, 8, 0xF, row, col);
        }
    }   
}

void cmd_mkdir(struct FAT32DriverRequest request, struct CPURegister cpu, uint8_t row, uint8_t col){
    *((int8_t*) cpu.ecx) = write(request);
    // puts("yayyy", 5, 0xF);
    if (cpu.ecx == (int8_t) 0){
        puts("Folder has been created", 24, 0xF, row, col);
    }
    else{
        puts("Error", 5, 0xF, row, col);
    }
}

// void cmd_rm(struct FAT32DriverRequest request, struct CPURegister cpu){
//     *((int8_t*) cpu.ecx) = delete(request);
//     // puts("yyy", 5, 0xF);
//     if (cpu.ecx == (int8_t) 0){
//         puts("Folder has been removed", 24, 0xF);
//     }
//     else{
//         puts("Error", 5, 0xF);
//     }
// }

// void cmd_cat(struct FAT32DriverRequest request){
//     read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
//     for(int i =0;i<64; i++){
//         if (fat_state.dir_table_buf.table[i].name[0] != 0x00) {
//             puts(fat_state.dir_table_buf.table[i].name, 8, 0xF);
//         }
//     }   
// }

void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
    uint8_t row, col;
    framebuffer_get_cursor(&row, &col);
    if (cpu.eax == 0) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read(request);
    } else if (cpu.eax == 4) {
        keyboard_state_activate();
        __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
        while (is_keyboard_blocking());
        char buf[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buf);
        memcpy((char *) cpu.ebx, buf, cpu.ecx);
    } else if (cpu.eax == 5) {
        puts((char *) cpu.ebx, cpu.ecx, cpu.edx, row, col);
        framebuffer_set_cursor(row + 1, 0);
    } 
    else if (cpu.eax == 6) {
        puts((char *) cpu.ebx, cpu.ecx, cpu.edx, row, col);
    } 
    else if (cpu.eax == 9) {
        struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
        cmd_ls(request, row, col);
    }
    else if(cpu.eax == 10){
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        cmd_mkdir(request, cpu, row, col);
    }
    
    // else if (cpu.eax == 11){
    //     struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
    //     cmd_rm(request, cpu); 
    // }
    // else if (cpu.eax == 10){
    //     struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
    //     cmd_cat(request);
    // }
    
}

void main_interrupt_handler(
    __attribute__((unused)) struct CPURegister cpu,
    uint32_t int_number,
    __attribute__((unused)) struct InterruptStack info
) {
    switch (int_number) {
        case (PIC1_OFFSET + IRQ_KEYBOARD):
            keyboard_isr();
            break;
        case 0x30:
            syscall(cpu, info);
            break;
        default:
            break;
    }
}