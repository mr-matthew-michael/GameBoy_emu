#include "cpu.h"
#include "mmu.h"

int timer_clocksum = 0;
int div_clocksum = 0;

void interrupts(MMU *mmu, Register *reg) 
{
    if (read_byte(mmu, 0xffff) & read_byte(mmu, 0xff0f) && reg->cc.halt)
        reg->cc.halt = 0;
    if(reg->int_enable)
    {
        if(read_byte(mmu, 0xffff) & read_byte(mmu, 0xff0f))
        {
            if((read_byte(mmu, 0xffff) & 1) & (read_byte(mmu, 0xff0f) & 1))
            {
                reg->sp--;
                write_byte(mmu, reg->sp, reg->pc >> 8);
                reg->sp--;
                write_byte(mmu, reg->sp, reg->pc & 0xff);
                reg->pc = 0x40;
                write_byte(mmu, 0xff0f, mmu_read_addr16(mmu, 0xff0f) & ~1);
            }
            if((read_byte(mmu, 0xffff) & 2) & (read_byte(mmu, 0xff0f) & 2))
            {
                reg->sp--;
                write_byte(mmu, reg->sp, reg->pc >> 8);
                reg->sp--;
                write_byte(mmu, reg->sp, reg->pc & 0xff);
                reg->pc = 0x48;
                write_byte(mmu, 0xff0f, read_byte(mmu, 0xff0f) & ~2);
            }
            else if((read_byte(mmu, 0xffff) & 4) & (read_byte(mmu, 0xff0f) & 4))
            {
                reg->sp--;
                write_byte(mmu, reg->sp, reg->pc >> 8);
                reg->sp--;
                write_byte(mmu, reg->sp, reg->pc & 0xff);
                reg->pc = 0x50;
                write_byte(mmu, 0xff0f, read_byte(mmu, 0xff0f) & ~4);
            }
            reg->int_enable = 0;
        }
    }
}

void clock(int cycle, MMU *mmu) 
{
    div_clocksum += (cycle/4);
    if (div_clocksum >= 256)
    {
        div_clocksum -= 256;
        aluToMem(mmu, 0xff04, 1);
    }
    if ((read_byte(mmu, 0xff07) >> 2) & 0x1)
    {
        timer_clocksum += cycle;
        int freq = 4096;
        if ((read_byte(mmu, 0xff07)& 3) ==1)
            freq = 262144;
        else if ((read_byte(mmu, 0xff07)& 3) == 2)
            freq = 65536;
        else if ((read_byte(mmu, 0xff07)& 3) == 3)
            freq = 16384;
        
        while (timer_clocksum >= (4194304 / freq))
        {
            aluToMem(mmu, 0xff05, 1);

            if ((mmu_read_addr16(mmu, 0xff05)) == 0x00)
            {
                write_byte(mmu, 0xff0f, read_byte(mmu,0xff0f)|4);
                write_byte(mmu, 0xff05, read_byte(mmu,0xff06));
            }
            timer_clocksum -= (4194304 / freq);
        }
    }
}
/*
int main()
{
    MMU *mmu = init();
    Register *reg = (Register*) malloc(sizeof(Register));
    Cart *cart = load_rom("/Users/Matt1/Gameboy_emu/gb-test-roms-master/cpu_instrs/individual/01-special.gb");
    load_rom_to_mmu(mmu, cart);
    mmu_load_bios(mmu);
    init_cpu(reg, mmu);
    
    
    uint8_t unpaused = 1;
    int sum = 0;
    int cyc = 0;
    while (1)
    {
                
        if (unpaused)
        {
            if(!reg->cc.halt) 
            {
                cyc = emulate_instrucions(reg, mmu);
                if (mmu->addr[0xff02] == 0x81) {
                char c = mmu->addr[0xff01];
                printf("%c", c);
                mmu->addr[0xff02] = 0x0;
            }
            }

            else
                cyc = 1;
            
            clock(cyc, mmu);  
            interrupts(mmu, reg); 
        }  
    }
    */
    //write_hl_16bit(reg, 0xa21b);
    //printf("reg h: [%0x], reg l: [%0x]\n", 0x0001+ 1,reg->l );
    //load_rom("Tetris.gb");
    //write_byte(mmu, 0xb101, 0x12);
    
//}