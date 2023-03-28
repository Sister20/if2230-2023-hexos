#include "../lib-header/stdtype.h"
#include "fat32.h"
#include "../lib-header/stdmem.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

static uint8_t fat32_buffer[BLOCK_SIZE];
static struct FAT32FileAllocationTable fat32_fat;
static struct FAT32DirectoryEntry fat32_root_dir;

static uint32_t fat32_current_cluster;
static uint32_t fat32_current_dir_cluster;

uint32_t cluster_to_lba(uint32_t cluster){
    return (cluster - 2) * CLUSTER_BLOCK_COUNT + 1;
}

uint32_t lba_to_cluster(uint32_t lba){
    return (lba - 1) / CLUSTER_BLOCK_COUNT + 2;
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster){
    struct FAT32DirectoryEntry *dir_entry = &dir_table->table;
    dir_entry->name[0] = 0xE5;
    dir_entry->ext[0] = 0x00;
    dir_entry->attribute = 0x10;
    dir_entry->user_attribute = 0x00;
    dir_entry->cluster_high = 0x00;
    dir_entry->cluster_low = 0x00;
    dir_entry->filesize = 0x00;
}

bool is_empty_storage(void){
    return memcmp(BOOT_SECTOR, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32(void){
    memmove(fat32_buffer, BOOT_SECTOR, BLOCK_SIZE);
    memset(fat32_buffer, 0, BLOCK_SIZE);
    memcpy(fat32_buffer, fs_signature, BLOCK_SIZE);
   // Write file system signature to boot sector
    write_block(0, fs_signature);

    // Write File Allocation Table
    write_block(1, CLUSTER_0_VALUE);  // cluster 0
    write_block(2, CLUSTER_1_VALUE);
}

void initialize_filesystem_fat32(void){
    if (is_empty_storage()){
        create_fat32();
    }
    else{
        read_block(0, fat32_buffer);
        memcpy(&fat32_fat, fat32_buffer, sizeof(struct FAT32FileAllocationTable));
        read_block(1, fat32_buffer);
        memcpy(&fat32_root_dir, fat32_buffer, sizeof(struct FAT32DirectoryEntry));
        fat32_current_cluster = 0;
        fat32_current_dir_cluster = 0;
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    uint32_t lba = cluster_to_lba(cluster_number);
    write_blocks(lba, ptr, cluster_count);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    uint32_t lba = cluster_to_lba(cluster_number);
    read_blocks(lba, ptr, cluster_count);
}

// int8_t read_directory(struct FAT32DriverRequest request){

// }

// int8_t read(struct FAT32DriverRequest request){
//     FAT32DiirectoryEntry entry;
//     while(request.name != entry.name && request.ext != entry.ext){
//         if(entry.attribute == 0x10){
//             read_clusters(&entry, entry.cluster_low, 1);
//         }
//         else{
//             read_clusters(&entry, entry.cluster_low, 1);
//         }
//     }
    
    
// }