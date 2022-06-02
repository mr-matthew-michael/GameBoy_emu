#include "mmu.h"
#include "rom.h"
#include <string.h>
#include <stdio.h>

MMU* init() 
{
    MMU *mmu = (MMU*)malloc(sizeof(MMU));
    mmu->joyflags = mmu->addr + 0xFF00;
    mmu->intflags = mmu->addr + 0xFF0F;
    mmu->bios_done = mmu->addr+0xFF50;
    return mmu;
}

void load_rom_to_mmu(MMU *mmu, Cart *cart, char *file_name)
{
    FILE *file = NULL;

    file = fopen(file_name, "rb");

    int pos = 0;
    while (fread(&mmu->addr[pos], 1, 1, file)) 
    {
        pos++;
    }
 
   // memcpy((void*)mmu->addr, (const void*)cart->rom, cart->size);
}

void mmu_load_bios(MMU *mmu)
{
    memcpy((void*)mmu->addr, (const void*)BIOS, sizeof(BIOS));
    (*mmu->bios_done) = 0;
    /*
    memcpy((void*)mmu->bios, (const void*)BIOS, sizeof(BIOS));
    (*mmu->bios_done) = 0;
    */
}

uint8_t read_byte(MMU *mmu, uint16_t val)
{
    return mmu->addr[val];
    /*
    if (!(*mmu->bios_done) && val >= 0x00 && val <= 0xFF)
        return mmu->bios[val];

    switch ((val & 0xf000)>>12)
    {
    case 0x0:
    
    //ROM BANK 1
    case 0x1:
    case 0x2:
    case 0x3:
        return mmu->rom[0][val];
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        return mmu->rom[1][val- 0x4000];
    case 0x8:
    case 0x9:
        return mmu->video_ram[val - 0x8000];
    case 0xa:
    case 0xb:
        return mmu->extra_ram[val - 0xA000];
    case 0xc:
    case 0xd:
        return mmu->work_ram[val - 0xC000];
    case 0xe:
        return mmu->work_ram[val - 0xC000];
    case 0xf:
        switch (val & 0x0f00)
        {
            case 0x000: case 0x100: case 0x200: case 0x300:
            case 0x400: case 0x500: case 0x600: case 0x700: 
            case 0x800: case 0x900: case 0xa00: case 0xb00:
            case 0xc00: case 0xd00:
                return mmu->work_ram[val & 0x1FFF];
            
            case 0xe00:
            // graphics 
                if (val < 0xfea0) 
                    return mmu->oam[val&0xff];
                else 
                    return 0;
        case 0xf00:
            if (val == 0xffff)
                    return mmu->inaccessible;
                else 
                {
                    switch (val & 0x00f0)
                    {
                        case 0x00:
                            return mmu->io[val&0xff];

                        case 0x40: case 0x50: case 0x60: case 0x70:
                            return mmu->ppu[val-0xff40];

                        case 0x80: case 0x90: case 0xa0: case 0xb0:
                        case 0xc0: case 0xd0: case 0xe0: case 0xf0:
                            return mmu->zram[val & 0x7f];
                    
                    }
                    
                   return 0;
                }
        }
    }
    */
}

uint16_t read_word(MMU *mmu, uint16_t address)
{
    return (read_byte(mmu,address) | (read_byte(mmu, address+1)<<8));
}

uint16_t mmu_read_addr16(MMU *mmu, uint16_t address)
{
    return *((uint16_t*)(mmu->addr + address));
}

uint8_t read_8bit_addr(MMU *mmu, uint16_t address)
{
 //   if (!(*mmu->bios_done) && address >= 0x00 && address <= 0xff) 
   //     return mmu->bios[address];
        
    return *(mmu->addr + address);
}

uint8_t write_byte(MMU *mmu, uint16_t address, uint8_t data)
{
    mmu->addr[address] = data;
    return 0;
    /*
    switch (address & 0xf000)
    {
    case 0x0000:
    {
        if(address >= 0x00 && address <= 0xFF)
            return 0;
        
    }
    //ROM BANK 1
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
    case 0x8000:
    case 0x9000:
        mmu->video_ram[address  & 0x1FFF] = data;
        return 1;
    case 0xa000:
    case 0xb000:
        mmu->extra_ram[address - 0Xa000] = data;
        return 2;
    case 0xc000:
    case 0xd000:
        mmu->work_ram[address  & 0x1FFF] = data;
        return 3;
    case 0xe000:
        mmu->work_ram[address & 0x1FFF] = data;
        return 4;
    case 0xf000:
        switch (address & 0x0f00)
        {
            case 0x000: case 0x100: case 0x200: case 0x300:
            case 0x400: case 0x500: case 0x600: case 0x700: 
            case 0x800: case 0x900: case 0xa00: case 0xb00:
            case 0xc00: case 0xd00:
                printf("address %0x\n", address - 0x1FFF);
                mmu->work_ram[address - 0x1FFF] = data;
                return 4;
            case 0xe00:
            // graphics 
                if (address < 0xfea0) 
                {
                    mmu->oam[address&0xff] = data;
                    return 6;
                }
                else 
                    return 0;
        case 0xf00:
            if (address == 0xffff)
            {
                mmu->inaccessible = data;
                return 0xa;
            }
            else 
            {
                switch (address & 0x00f0)
                {
                    case 0x00:
                        mmu->io[address&0xff] = data;
                        return 6;
                    case 0x40: case 0x50: case 0x60: case 0x70:
                        mmu->ppu[address-0xff40] = data;
                        return 7;
                    case 0x80: case 0x90: case 0xa0: case 0xb0:
                    case 0xc0: case 0xd0: case 0xe0: case 0xf0:
                        mmu->zram[address & 0x7f] = data;
                        //printf("working %0x\n", address);
                        return 8;
                }
            }
        }
    }
    return 0;
    */
}

uint8_t write_word(MMU *mmu, uint16_t address, uint16_t data)
{
    uint8_t new_data = write_byte(mmu, address, data&0xff) & write_byte(mmu, address+1, data>>8);
    return new_data;
}

void write_8bit_addr(MMU *mmu, uint16_t address, uint8_t data)
{
    mmu->addr[address] = data;
}

void write_16bit_addr(MMU *mmu, uint16_t address, uint16_t data)
{
    uint16_t *position = ((uint16_t*)(mmu->addr+address));
    *position = data;
}

void aluToMem(MMU *mmu, uint16_t address, uint16_t data)
{
    mmu->addr[address] += data;
}

/*
int main()
{
    MMU *mmu = init();
    Cart *cart = load_rom("/Users/Matt1/Gameboy_emu/gb-test-roms-master/cpu_instrs/individual/01-special.gb");
    mmu_load_bios(mmu);
    load_rom_to_mmu(mmu, cart);
    
    //load_rom("Tetris.gb");
    //write_byte(mmu, 0xb101, 0x12);
    printf("%0x\n", read_byte(mmu, mmu->addr[0xff01]));
}
*/

