#include "fat32.h"
#include "disk.h"
#include "../lib-header/stdtype.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/framebuffer.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

// Static Driver State
static struct FAT32DriverState fat_state;
static struct FAT32DirectoryTable test;

// Convert cluster number to LBA
uint32_t cluster_to_lba(uint32_t cluster){
    return (cluster) * CLUSTER_BLOCK_COUNT;
}

/* === INITIALIZING FILESYSTEM ================================================ */
// Initialize directory table
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster){
    memcpy(dir_table->table[0].name, name, 8);
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].cluster_low = (uint16_t) (parent_dir_cluster >> 16);
    dir_table->table[0].cluster_high = (uint16_t) (parent_dir_cluster & 0xFFFF);
    dir_table->table[0].filesize = 0;
}

// Checking if storage is empty
bool is_empty_storage(void){
    uint8_t buf[BLOCK_SIZE];
    read_blocks(buf, BOOT_SECTOR, 1);
    return memcmp(buf, fs_signature, BLOCK_SIZE);
}

// Create FAT32 filesystem
void create_fat32(void){
    // Copy signature to boot sector
    memcpy(fat_state.cluster_buf.buf, fs_signature, BLOCK_SIZE);

    // Initialize empty FAT
    fat_state.fat_table.cluster_map[BOOT_SECTOR] = CLUSTER_0_VALUE;
    fat_state.fat_table.cluster_map[FAT_CLUSTER_NUMBER] = CLUSTER_1_VALUE;
    fat_state.fat_table.cluster_map[ROOT_CLUSTER_NUMBER] = FAT32_FAT_END_OF_FILE;

    // Initialize root directory
    // read_clusters(&fat_state.dir_table_buf, ROOT_CLUSTER_NUMBER, 1);
    init_directory_table(&fat_state.dir_table_buf, "root\0\0\0", ROOT_CLUSTER_NUMBER); 

    // Write reserved clusters
    write_clusters(fat_state.cluster_buf.buf, BOOT_SECTOR, 1);
    write_clusters(fat_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    write_clusters(fat_state.dir_table_buf.table, ROOT_CLUSTER_NUMBER, 1);   
}

// Initialize filesystem
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
    write_blocks(ptr, lba, cluster_count * 4);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    uint32_t lba = cluster_to_lba(cluster_number);
    read_blocks(ptr, lba, cluster_count * 4);
}



/* === CRUD ===================================================== */
int8_t read_directory(struct FAT32DriverRequest request) {
    uint8_t i;
    read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
    uint8_t isDir =  isDirectory(fat_state.dir_table_buf.table);
    
    if (request.parent_cluster_number < 2 || request.parent_cluster_number > CLUSTER_MAP_SIZE || isDir == 0){
       return -1;
    }
    for (i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
        struct FAT32DirectoryEntry* entry = &fat_state.dir_table_buf.table[i];
        // uint32_t cluster_number = ((uint32_t) entry->cluster_low << 16) | entry->cluster_high;
        if (memcmp(entry->name, request.name, 8) == 0){
            if (request.buffer_size < entry->filesize){
                return 2;
            }
            break;            
        }
    }
    if (i==CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)){
        return 3;
    }
    struct FAT32DirectoryEntry* entry = &fat_state.dir_table_buf.table[i];
    read_clusters(&fat_state.cluster_buf, entry->cluster_low, 1);
    memcpy(request.buf, fat_state.cluster_buf.buf, entry->filesize);
    return 0;
}


int8_t read(struct FAT32DriverRequest request){
    uint8_t i;
    read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
    uint8_t isDir =  isDirectory(fat_state.dir_table_buf.table);
    
    if (request.parent_cluster_number < 2 || request.parent_cluster_number > CLUSTER_MAP_SIZE || isDir == 0){
       return -1;
    }
    for (i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
        struct FAT32DirectoryEntry* entry = &fat_state.dir_table_buf.table[i];
        // uint32_t cluster_number = ((uint32_t) entry->cluster_low << 16) | entry->cluster_high;
        if (memcmp(entry->name, request.name, 8) == 0 && memcmp(entry->ext, request.ext, 3) == 0){
            if (request.buffer_size < entry->filesize){
                return 2;
            }
            break;            
        }
    }
    if (i==CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)){
        return 3;
    }
    struct FAT32DirectoryEntry* entry = &fat_state.dir_table_buf.table[i];
    read_clusters(&fat_state.cluster_buf, entry->cluster_low, 1);
    memcpy(request.buf, fat_state.cluster_buf.buf, BLOCK_SIZE);
    return 0;
}

/* === DIRECTORY OPERATIONS ================================================== */
// find empty cluster to write in fat table
uint32_t findEmptyCluster(void){
    for (int i = 2; i < CLUSTER_MAP_SIZE; i++){
        if (fat_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY){
            return i;
        }
    }
    return 0;
}

// find next empty cluster to write in fat table through i
uint32_t findNextEmptyCluster(uint32_t i){
    if (i >= CLUSTER_MAP_SIZE || i < 2){
        return 0;
    }

    for (uint32_t j = i+1; j < CLUSTER_MAP_SIZE; j++){
        if (fat_state.fat_table.cluster_map[j] == FAT32_FAT_EMPTY_ENTRY){
            return j;
        }
    }
    return 0;
}

// Find empty entry in parent directory table
uint8_t findEmptyEntry(struct FAT32DirectoryEntry *entry){
    for (uint8_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
        if (entry[i].user_attribute != UATTR_NOT_EMPTY){
            return i;
        }
    }
    return 0;
}


// is parent directory is a directory?
uint8_t isDirectory(struct FAT32DirectoryEntry *entry){
    if (entry[0].user_attribute != UATTR_NOT_EMPTY || entry[0].filesize != 0){
        return 0;
    }
    else {
        return 1;
    }
}


// is there an exist folder or file in parent directory?
uint8_t isExist(struct FAT32DriverRequest request, struct FAT32DirectoryEntry *entry){
    for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
        if (request.buffer_size == 0) {
            if (memcmp(entry[i].name, request.name, 8) == 0) {
                return i;
            }
        }
        else {
            if (memcmp(entry[i].name, request.name, 8) == 0 && memcmp(entry[i].ext, request.ext, 3) == 0) {
                return i;
            }
        }
    }
    return 0;
}

// Validate write request
uint8_t validateWrite(struct FAT32DriverRequest request, struct FAT32DirectoryEntry *entry){
    
    // validate parent cluster number
    if (request.parent_cluster_number < 2 || 
        request.parent_cluster_number > CLUSTER_MAP_SIZE || 
        isDirectory(entry) == 0) {
        return 2;
    }

    // already exist
    if (isExist(request, entry) > 0){
        return 1;
    }

    return 0;
}

// Write entry to parent directory table
void writeEntry(struct FAT32DriverRequest request, struct FAT32DirectoryEntry *entry, uint32_t cluster_number, uint8_t entry_number){
    memcpy(entry[entry_number].name, request.name, 8);
    entry[entry_number].user_attribute  = UATTR_NOT_EMPTY;
    entry[entry_number].cluster_low     = (uint16_t) (cluster_number & 0xFFFF);
    entry[entry_number].cluster_high    = (uint16_t) (cluster_number >> 16);
    entry[entry_number].filesize        = request.buffer_size;

    if (request.buffer_size == 0){
        entry[entry_number].attribute   = ATTR_SUBDIRECTORY;
    }
    else {
        entry[entry_number].attribute   = 0;
        memcpy(entry[entry_number].ext, request.ext, 3);
    }
}

int8_t write(struct FAT32DriverRequest request){
    read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
    uint8_t val = validateWrite(request, fat_state.dir_table_buf.table);
    isDirectory(fat_state.dir_table_buf.table);

    if (val != 0){
        return val;
    }

    // FIND EMPTY CLUSTER AND ENTRY
    uint32_t cluster_number = findEmptyCluster();
    uint8_t entry_number = findEmptyEntry(fat_state.dir_table_buf.table);
    if (cluster_number == 0 || entry_number == 0){
        return -1;
    }

    // WRITE ENTRY IN PARENT DIRECTORY
    writeEntry(request, fat_state.dir_table_buf.table, cluster_number, entry_number);
    write_clusters(fat_state.dir_table_buf.table, request.parent_cluster_number, 1);
    
    // WRITE FOLDER
    if (request.buffer_size == 0){
        read_clusters(&fat_state.dir_table_buf, cluster_number, 1);
        init_directory_table(&fat_state.dir_table_buf, request.name, request.parent_cluster_number);
        write_clusters(fat_state.dir_table_buf.table, cluster_number, 1);

        fat_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_END_OF_FILE;
        write_clusters(fat_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    }

    // WRITE FILE
    else{
        uint32_t remaining_bytes = request.buffer_size;
        uint32_t temp_cluster;
        uint32_t n = 0;
        
        while (remaining_bytes > 0){
            temp_cluster = cluster_number;
            
            uint32_t offset = n * CLUSTER_SIZE;

            // Write cluster buffer to disk
            read_clusters(&fat_state.cluster_buf, temp_cluster, 1);
            memcpy(fat_state.cluster_buf.buf, request.buf + offset, CLUSTER_SIZE);
            write_clusters(fat_state.cluster_buf.buf, temp_cluster, 1);

            // Update FAT table
            read_clusters(&fat_state.fat_table, FAT_CLUSTER_NUMBER, 1);
            if (remaining_bytes <= CLUSTER_SIZE){
                fat_state.fat_table.cluster_map[temp_cluster] = FAT32_FAT_END_OF_FILE;
                break;
            }
            else {
                cluster_number = findNextEmptyCluster(cluster_number);
                if (cluster_number == 0){
                    return -1;
                }
                fat_state.fat_table.cluster_map[temp_cluster] = cluster_number;
            }
            write_clusters(fat_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
            
            remaining_bytes -= CLUSTER_SIZE;
            n++;
        }
        write_clusters(fat_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    }
    return 0;
}

int8_t delete(struct FAT32DriverRequest request){
    read_clusters(&fat_state.dir_table_buf, request.parent_cluster_number, 1);
    // read_clusters(&fat_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    uint8_t i;
    uint32_t cluster_number;
    uint8_t isDir = isDirectory(fat_state.dir_table_buf.table);
    uint32_t temp;
    
    // uint8_t isEx = isExist(request, fat_state.dir_table_buf.table);
    if (request.parent_cluster_number < 2 || 
        request.parent_cluster_number > CLUSTER_MAP_SIZE ||
        isDir == 0) {
        return -1;
    }
     
    for (i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++){

        if ((memcmp(fat_state.dir_table_buf.table[i].name, request.name,8) == 0)){
    
            if(fat_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY ){
                cluster_number = fat_state.dir_table_buf.table[i].cluster_high << 16 | fat_state.dir_table_buf.table[i].cluster_low;
                read_clusters(&test, cluster_number, 1);
        
                for (uint8_t j = 1; j < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); j++){
                    if (test.table[j].user_attribute != 0){
                        // read_clusters(&test, request.parent_cluster_number, 1);
                        // struct FAT32DirectoryEntry empty_entry = {0};
                        // fat_state.dir_table_buf.table[i] = empty_entry;
                        // write_clusters(&test, cluster_number, 1);
                

                        return 2;
                    }

                }
            
            }
            else{
                for (uint8_t k = 1; k < (request.buffer_size/CLUSTER_SIZE); k++){
            

                    temp = fat_state.fat_table.cluster_map[k];
                    fat_state.fat_table.cluster_map[k] = 0;
                    write_clusters(&temp, request.parent_cluster_number, 1);
                    }

                   
                }
                continue;
            }
                cluster_number = fat_state.dir_table_buf.table[i].cluster_high << 16 | fat_state.dir_table_buf.table[i].cluster_low;
        
                fat_state.dir_table_buf.table[i].user_attribute = 0;
                struct FAT32DirectoryEntry empty_entry = {0};
                fat_state.dir_table_buf.table[i] = empty_entry;
        
                fat_state.fat_table.cluster_map[cluster_number] = 0x00000000;
                write_clusters(&fat_state.fat_table, FAT_CLUSTER_NUMBER, 1);
                write_clusters(fat_state.dir_table_buf.table, request.parent_cluster_number, 1);

                break;
        }
        
    if (i == (CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry))){
        return 1;
    }
    return 0;
}
