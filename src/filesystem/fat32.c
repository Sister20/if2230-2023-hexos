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
    read_clusters(fat_state.cluster_buf.buf, (uint32_t) fs_signature, 1);
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


// CRUD ===================================
// int8_t read_directory(struct FAT32DriverRequest request) {
//     struct FAT32DirectoryTable* buffer = (struct FAT32DirectoryTable*) request.buf;
//     const char* target_name = request.name;
//     uint32_t target_parent_cluster = request.parent_cluster_number;
    
//     // Check if the parent cluster is actually a folder
//     struct FAT32DirectoryEntry* parent_entry = (struct FAT32DirectoryEntry*) buffer->table;
//     if ((parent_entry->attribute & 0x10) != 0x10) {
//         return 1; // Not a folder
//     }
    
//     // Search for the target directory entry within the parent directory
//     for (int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
//         struct FAT32DirectoryEntry* entry = &buffer->table[i];
//         if (entry->name[0] == 0x00) {
//             // End of directory table reached without finding the target entry
//             return 2; // Not found
//         }
//         if (entry->name[0] == 0xE5) {
//             // Entry deleted, skip
//             continue;
//         }
//         if ((entry->attribute & 0x10) == 0x10) {
//             // Subdirectory, check if this is the target entry
//             if (entry->name == (char) target_name && entry->cluster_low == (uint16_t) target_parent_cluster) {
//                 memcpy(buffer->table, (void*) entry->cluster_low, sizeof(struct FAT32DirectoryTable));
//                 return 0; // Success
//             }
//         }
//     }
    
//     // End of directory table reached without finding the target entry
//     return 2; // Not found
// }


// int8_t read(struct FAT32DriverRequest request){
//     static struct FAT32DirectoryEntry entry;
//     static struct FAT32DirectoryTable dir_table;
// // int i = 0;
//     int8_t error = read_directory((struct FAT32DriverRequest) {
//         .buf = &dir_table,
//         .name = request.name,
//         .ext = request.ext,
//         .parent_cluster_number = request.parent_cluster_number,
//         .buffer_size = sizeof(struct FAT32DirectoryTable),
//     });

//     if (error != 0) {
//         return error;
//     }
//     bool flag = FALSE;
//     for (int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
//         entry = dir_table.table[i];
//         if (memcmp(entry.name, request.name, 8) == 0 && memcmp(entry.ext, request.ext, 3) == 0) {
//             flag = TRUE;
//             break;
//         }
//         if (entry.attribute & (1 << 4)) {
//             // Subdirectory flag is set, this is not a file
//             return 1;
//         }
//      }
//     uint32_t cluster_number = (entry.cluster_high << 16) | entry.cluster_low;
    
//     read_clusters(&dir_table, entry.cluster_low, 1);
//     memcpy(request.buf, &entry, entry.filesize);
//     return 0;
//     } 


uint32_t findEmptyCluster(void){
    for (int i = 2; i < CLUSTER_MAP_SIZE; i++){
        if (fat_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY){
            return i;
        }
    }
    return -1;
}

int8_t isDirectory(struct FAT32DriverRequest request){
    read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
    if (fat_state.dir_table_buf.table[0].user_attribute != UATTR_NOT_EMPTY || fat_state.dir_table_buf.table[0].filesize != 0){
        return 0;
    }
    else {
        return 1;
    }
}

int8_t isExist(struct FAT32DriverRequest request){
    read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
    for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
        // if (fat_state.dir_table_buf.table[i].name[0] == 0xE5) {
        //     continue;
        // }
        if (request.buf == 0) {
            if (memcmp(fat_state.dir_table_buf.table[i].name, request.name, 8) == 0) {
                return 1;
            }
            else {
                if (memcmp(fat_state.dir_table_buf.table[i].name, request.name, 8) == 0 && memcmp(fat_state.dir_table_buf.table[i].ext, request.ext, 3) == 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}
int8_t write(struct FAT32DriverRequest request){
    // static struct FAT32DirectoryEntry entry;
    // static struct FAT32DirectoryTable dir_table;
    // bool flag = FALSE;
    // for (int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
    // if (dir_table.table[i].attribute == 0x00) {
    //     flag = TRUE;
    //     if (request.parent_cluster_number != 0 && memcmp(entry.name, request.name, 8) == 0 && memcmp(entry.ext, request.ext, 3) == 0) {

    //     }
    // }
    // }
    uint32_t cluster_number = findEmptyCluster();
    // write folder
    if (request.buffer_size == 0){
        uint8_t i;
        read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1); 
        // find empty entry in parent directory table
        for (i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){ 
            if (fat_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
                memcpy(fat_state.dir_table_buf.table[i].name, request.name, 8);
                fat_state.dir_table_buf.table[i].attribute = ATTR_SUBDIRECTORY;
                fat_state.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY;
                fat_state.dir_table_buf.table[i].cluster_low = (uint16_t) (cluster_number >> 16);
                fat_state.dir_table_buf.table[i].cluster_high = (uint16_t) (cluster_number & 0xFFFF);
                fat_state.dir_table_buf.table[i].filesize = 0;
                break;
            }
        }

        //validasi parent cluster
        if (request.parent_cluster_number < 2 || request.parent_cluster_number > CLUSTER_MAP_SIZE || isDirectory(request) == 0){
            return 2;
        }

        //if full
        if (i == CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)){
            return -1;
        }

        // already exist
        if (isExist(request) == 1){
            return 1;
        }

        write_clusters(fat_state.dir_table_buf.table, request.parent_cluster_number, 1);
        read_clusters(&fat_state.dir_table_buf, cluster_number, 1);
        init_directory_table(&fat_state.dir_table_buf, request.name, request.parent_cluster_number);
        write_clusters(fat_state.dir_table_buf.table, cluster_number, 1);

        fat_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_END_OF_FILE;
        write_clusters(fat_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);

    }
    // WRITE FILE
    else{
        uint8_t i;
        read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1); 
        // find empty entry in parent directory table
        for (i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){ 
            if (fat_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
                memcpy(fat_state.dir_table_buf.table[i].name, request.name, 8);
                memcpy(fat_state.dir_table_buf.table[i].ext, request.ext, 3);
                fat_state.dir_table_buf.table[i].attribute = 0;
                fat_state.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY;
                fat_state.dir_table_buf.table[i].cluster_low = (uint16_t) (cluster_number >> 16);
                fat_state.dir_table_buf.table[i].cluster_high = (uint16_t) (cluster_number & 0xFFFF);
                fat_state.dir_table_buf.table[i].filesize = request.buffer_size;
                break;
            }
        }

        //validasi parent cluster
        if (request.parent_cluster_number < 2 || request.parent_cluster_number > CLUSTER_MAP_SIZE || isDirectory(request) == 0){
            return 2;
        }

        //if full
        if (i == CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)){
            return -1;
        }

        // already exist
        if (isExist(request) == 1){
            return 1;
        }
        

        write_clusters(fat_state.dir_table_buf.table, request.parent_cluster_number, 1);

        read_clusters(&fat_state.dir_table_buf, cluster_number, 1);
        memcpy(fat_state.dir_table_buf.table, request.buf, request.buffer_size);
        write_clusters(fat_state.dir_table_buf.table, cluster_number, 1);
    }
    return 0;
}
