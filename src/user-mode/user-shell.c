#include "../lib-header/stdtype.h"
#include "../filesystem/fat32.h"
// #include "../lib-header/stdmem.h"
#include "../interrupt/interrupt.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *buf1 = (const uint8_t*) s1;
    const uint8_t *buf2 = (const uint8_t*) s2;
    for (size_t i = 0; i < n; i++) {
        if (buf1[i] < buf2[i])
            return -1;
        else if (buf1[i] > buf2[i])
            return 1;
    }

    return 0;
}

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++)
        dstbuf[i] = srcbuf[i];
    return dstbuf;
}

void read_buffer(char* buf) {

    if (memcmp(buf, "ls", 3) == 0) {
        syscall(9, 0, 0, 0);
        // puts("ls za", 5, 0xF);
    }

    if (memcmp(buf, "mkdir", 5) == 0){
        char *name = buf + 6;
        struct FAT32DriverRequest request = {
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x0,
    };
        memcpy(request.name,name,8);
        // puts("mkdir za", 8, 0xF);
        syscall(10, (uint32_t) &request, 0, 0);
    }
    
    // if (memcmp((char*)buf, "rm", 2) == 0){
    //     char *name = buf + 3;
    //     // puts("yey", 8, 0xF);

    //     struct FAT32DriverRequest request = {
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 0x0,
    // };
    //     memcpy(request.name,name,8);
    //     syscall(11, (uint32_t) &request, 0, 0);
    // }

    // if (memcmp(buf, "cat ", 4) == 0){
    //     char *name = buf + 4;
    //     struct FAT32DriverRequest request = {
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 0x0,
    // };
    //     memcpy(request.name,name,8);
    //     syscall(10, (uint32_t) &request, 0, 0);
    // }

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
        read_buffer(buf);
    }

    return 0;
}
