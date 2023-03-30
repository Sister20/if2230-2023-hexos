#include "fat32.h"
#include "disk.h"
#include "../lib-header/stdtype.h"
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

static struct FAT32DriverState fat_state;

// static uint8_t fat32_buffer[BLOCK_SIZE];
// static struct FAT32FileAllocationTable fat32_fat;
// static struct FAT32DirectoryEntry fat32_root_dir;

// static uint32_t fat32_current_cluster;
// static uint32_t fat32_current_dir_cluster;

uint32_t cluster_to_lba(uint32_t cluster){
    return (cluster) * CLUSTER_BLOCK_COUNT;
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster){
    memcpy(dir_table->table[0].name, name, 8);
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].cluster_low = (uint16_t) (parent_dir_cluster >> 16);
    dir_table->table[0].cluster_high = (uint16_t) (parent_dir_cluster & 0xFFFF);
    dir_table->table[0].filesize = 0;
}

bool is_empty_storage(void){
    read_clusters(fat_state.cluster_buf.buf, (uint32_t)fs_signature, 1);
    return memcmp(fat_state.cluster_buf.buf, BOOT_SECTOR, BLOCK_SIZE) == 1;
}

void create_fat32(void){
    // Copy signature to boot sector
    memcpy(fat_state.cluster_buf.buf, fs_signature, BLOCK_SIZE);

    // Initialize empty FAT
    fat_state.fat_table.cluster_map[BOOT_SECTOR] = CLUSTER_0_VALUE;
    fat_state.fat_table.cluster_map[FAT_CLUSTER_NUMBER] = CLUSTER_1_VALUE;
    fat_state.fat_table.cluster_map[ROOT_CLUSTER_NUMBER] = FAT32_FAT_END_OF_FILE;

    // Initialize root directory
    init_directory_table(&fat_state.dir_table_buf, "root", ROOT_CLUSTER_NUMBER); 

    // Write reserved clusters
    write_clusters(fat_state.cluster_buf.buf, BOOT_SECTOR, 1);
    write_clusters(fat_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    write_clusters(fat_state.dir_table_buf.table, ROOT_CLUSTER_NUMBER, 1);   
}

void initialize_filesystem_fat32(void){
    if (is_empty_storage()){
        create_fat32();
    }
    else{
        read_clusters(fat_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
        read_clusters(fat_state.dir_table_buf.table, ROOT_CLUSTER_NUMBER, 1);
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    uint32_t lba = cluster_to_lba(cluster_number);
    write_blocks(ptr, lba, cluster_count);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    uint32_t lba = cluster_to_lba(cluster_number);
    read_blocks(ptr, lba, cluster_count);
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