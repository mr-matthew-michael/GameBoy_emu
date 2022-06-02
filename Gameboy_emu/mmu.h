#ifndef _MMU_H_
#define _MMU_H_
#include <stdlib.h>
#include "rom.h"
#include "mmu_constants.h"

typedef struct _MMU 
{
    uint8_t addr[0xFFFF + 0x0001];
    /*
    uint8_t	bios[0x100];
    struct
    {
        union
        {
            uint8_t addr[0xFFFF + 0x0001];
            struct
            {
                uint8_t rom[2][0x4000];
                uint8_t video_ram[0X2000];
                uint8_t work_ram[0X2000];
                uint8_t work_ram_shadow[0x1E00];
                uint8_t extra_ram[0X2000];
                uint8_t oam[0xA0];
                uint8_t empty[0x40];
                uint8_t io[0x40];
                uint8_t ppu[0x40];
                uint8_t zram[0x80];
                uint8_t inaccessible; 
            };
        };
    };
    */
    uint8_t* joyflags;
    uint8_t* intflags;
    uint8_t* bios_done;
    
}MMU;

MMU* init();
void aluToMem(MMU *mmu, uint16_t address, uint16_t data);
uint8_t read_byte(MMU *mmu, uint16_t val);
uint8_t write_byte(MMU *mmu, uint16_t address, uint8_t data);
void load_rom_to_mmu(MMU *mmu, Cart *cart, char *file_name);
void mmu_load_bios(MMU *mmu);
uint16_t read_word(MMU *mmu, uint16_t address);
uint16_t mmu_read_addr16(MMU *mmu, uint16_t address);
uint8_t read_8bit_addr(MMU *mmu, uint16_t address);
uint8_t write_word(MMU *mmu, uint16_t address, uint16_t data);
void write_8bit_addr(MMU *mmu, uint16_t address, uint8_t data);
void write_16bit_addr(MMU *mmu, uint16_t address, uint16_t data);

#endif