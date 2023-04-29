#include "../lib-header/stdtype.h"
#include "../filesystem/fat32.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int main(void) {
    char buf[16];
    while (TRUE) {
        syscall(6, (uint32_t) ("hexOS@OS-IF2230"), 15, 0x2);
        syscall(6, (uint32_t) ":", 1, 0x7);
        syscall(6, (uint32_t) "root", 4, 0x9);
        // syscall(6, (uint32_t) print_cwd())
        syscall(6, (uint32_t) "$ ", 2, 0x7);
        // syscall(5, (uint32_t) ":", 7, 0xF);
        syscall(4, (uint32_t) buf, 16, 0);
        syscall(5, (uint32_t) buf, 16, 0xE);
    }

    return 0;
}
