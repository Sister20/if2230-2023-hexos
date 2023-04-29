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

// int main(void) {
//     __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(0xDEADBEEF));
//     while (TRUE);
//     return 0;
// }

int main(void) {
    syscall(5, (uint32_t) "owo\n", 4, 0xF);
    char buf[16];
    while (TRUE) {
        // syscall(5, (uint32_t) "hexos\n", 5, 0xD);
        syscall(4, (uint32_t) buf, 16, 0);
        syscall(5, (uint32_t) buf, 16, 0xF);
    }

    return 0;
}
