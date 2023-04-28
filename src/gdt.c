#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"
#include "interrupt/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // Implement: Null Descriptor
            .segment_low        = 0,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0,
            .non_system         = 0,
            .dpl                = 0,
            .present            = 0,
            .segment_high       = 0,
            .reserved           = 0,
            .long_mode          = 0,
            .operation_size     = 0,
            .granularity        = 0,
            .base_high          = 0
        },
        {
            // Implement: Kernel Code Segment
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0xA,
            .non_system         = 1,
            .dpl                = 0,
            .present            = 1,
            .segment_high       = 0,
            .reserved           = 0,
            .long_mode          = 0,
            .operation_size     = 1,
            .granularity        = 1,
            .base_high          = 0
        },
        {
            // Implement: Kernel Data Segment
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0x2,
            .non_system         = 1,
            .dpl                = 0,
            .present            = 1,
            .segment_high       = 0xF,
            .reserved           = 0,
            .long_mode          = 0,
            .operation_size     = 1,
            .granularity        = 1,
            .base_high          = 0
        },
        {
            // Implement: User Code Segment
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0xA,
            .non_system         = 1,
            .dpl                = 3,
            .present            = 1,
            // .segment_high       = 0b1111,
            .segment_high       = 0,
            .reserved           = 0,
            .long_mode          = 0,
            .operation_size     = 1,
            .granularity        = 1,
            .base_high          = 0
        },
        {
            // Implement: User Data Segment
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0x2,
            .non_system         = 1,
            .dpl                = 3,
            .present            = 1,
            // .segment_high       = 0b1111,
            .segment_high       = 0xF,
            .reserved           = 0,
            .long_mode          = 0,
            .operation_size     = 1,
            .granularity        = 1,
            .base_high          = 0
        },
        {
            // Implement: TSS Segment
            .segment_low        = sizeof(struct TSSEntry),
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0x9,
            .non_system         = 0,
            .dpl                = 0,
            .present            = 1,
            .segment_high       = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .reserved           = 1,
            .long_mode          = 0,
            .operation_size     = 1,
            .granularity        = 0,
            .base_high          = 0
        }
        // {0}
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
     // TODO : Implement, this GDTR will point to global_descriptor_table.
     // Use sizeof operator
    .size = sizeof(struct GlobalDescriptorTable) - 1,
    .address = &global_descriptor_table    
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}
