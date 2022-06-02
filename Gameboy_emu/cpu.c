#include "cpu.h"
#include "mmu.h"
#include "rom.h"
#include "gbDis.h"
#include "debug.h"

int div_clocksum = 0;
int timer_clocksum = 0;

unsigned char cycles[] = {
	4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4, //0x00..0x0f
	4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4, //0x10..0x1f
	12, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4, //etc
	12, 12, 8, 8, 12, 12, 12, 4, 12, 8, 8, 8, 4, 4, 8, 4,
	
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, //0x40..0x4f
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4,
	
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, //0x80..8x4f
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	
	20, 12, 16, 16, 24, 16, 8, 16, 20, 16, 16, 4, 24, 24, 8, 16, //0xc0..0xcf
	20, 12, 16, 0, 24, 16, 8, 16, 20, 16, 16, 0, 24, 0, 8, 16,
	12, 12, 8, 0, 0, 16, 8, 16, 16, 4, 16, 0, 24, 0, 8, 16,
	12, 12, 8, 4, 0, 16, 8, 16, 12, 8, 16, 4, 24, 0, 8, 16, 
};
unsigned char cb_cycles[] = {
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x00..0x0f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x10..0x1f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //etc
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
	
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x00..0x0f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x10..0x1f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //etc
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
	
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x00..0x0f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x10..0x1f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //etc
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
	
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x00..0x0f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //0x10..0x1f
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, //etc
	8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
};

void z_flag(Register *reg)
{
    uint8_t flag_reg = reg->f;
    if (reg->cc.z)
        reg->f = (flag_reg | 0x80); 
    else
        reg->f = ((flag_reg ) & ~(0x80));
}

void n_flag(Register *reg)
{
    uint8_t flag_reg = reg->f;
    if (reg->cc.n)
        reg->f =  (flag_reg | (0x80 >> 1));  
    else 
        reg->f = ((flag_reg ) & ~(0x80 >> 1));    
}

void h_flag(Register *reg)
{
    uint8_t flag_reg = reg->f;
    if (reg->cc.h)
       reg->f = (flag_reg | (0x80 >> 2));   
    else 
       reg->f = ((flag_reg ) & ~(0x80 >> 2));     
}

void c_flag(Register *reg)
{
    uint8_t flag_reg = reg->f;
    if (reg->cc.c)
        reg->f = (flag_reg | (0x80 >> 3)); 
    else 
        reg->f =((flag_reg ) & ~(0x80 >> 3));      
}

static void unimplimented_instruction(Register *reg)
{
    printf("Error: Unkown Instruction\n");
    reg->pc--;
    printf("\n");
    exit(1);
}
//write functions 
void write_bc_16bit(Register *reg, uint16_t data)
{
    uint8_t hi = (data>>8);
    uint8_t lo = (data) & 0xff;
    reg->b = hi;
    reg->c = lo;
}

void write_de_16bit(Register *reg, uint16_t data)
{
    uint8_t hi = (data>>8);
    uint8_t lo = (data) & 0xff;
    reg->d = hi;
    reg->e = lo;
}

void write_hl_16bit(Register *reg, uint16_t data)
{
    uint8_t hi = (data>>8);
    uint8_t lo = (data) & 0xff;
    reg->h = hi;
    reg->l = lo;
}

void write_af_16bit(Register *reg, uint16_t data)
{
    uint8_t hi = (data>>8);
    uint8_t lo = (data) & 0xff;
    reg->a = hi;
    reg->f = lo;
}
static void pop(MMU *mmu, Register *reg, uint8_t *high, uint8_t *low)
{
    *low = read_byte(mmu, reg->sp);
    *high = read_byte(mmu, reg->sp+1);
    reg->sp += 2;    
    //printf ("%04x %04x pop\n", reg->pc, reg->sp);
}

static void push(MMU *mmu, Register* reg, uint8_t high, uint8_t low)
{
    write_byte(mmu, reg->sp-1, high);
    write_byte(mmu, reg->sp-2, low);
    reg->sp = reg->sp - 2;    
    //    printf ("%04x %04x\n", state->pc, state->sp);
}

//read
uint16_t read_bc_16bit(Register *reg)
{
    uint16_t bc_reg = (reg->b << 8) | reg->c;
    return bc_reg;
}

uint16_t read_hl_16bit(Register *reg)
{
    uint16_t hl_reg = (reg->h << 8) | reg->l;
    return hl_reg;
}

uint16_t read_de_16bit(Register *reg)
{
    uint16_t de_reg = (reg->d << 8) | reg->e;
    return de_reg;
}

//Initialize 
void init_cpu(Register *reg, MMU *mmu)
{
    *mmu->bios_done = 1;
    reg->cc.z =1;
    reg->pc = 0x0100;
    reg->sp = 0xFFFE;
    
    write_af_16bit(reg, 0x1180);
    write_bc_16bit(reg, 0x0000);
    write_de_16bit(reg, 0xff56);
    write_hl_16bit(reg, 0x000d);
    
}

//8bit load instructions
void LD_b_b_function(Register *reg) 
{
    reg->b = reg->b;
}

void LD_b_c_function(Register *reg) 
{
    reg->b = reg->c;
}

void LD_b_d_function(Register *reg) 
{
    reg->b = reg->d;
}

void LD_b_e_function(Register *reg) 
{
    reg->b = reg->e;
}

void LD_b_h_function(Register *reg) 
{
    reg->b = reg->h;
}

void LD_b_l_function(Register *reg) 
{
    reg->b = reg->l;
}

void LD_b_hl_function(Register *reg, MMU *mmu) 
{
    uint16_t offset = (reg->h << 8) | reg->l;
    uint8_t data = read_byte(mmu, offset);
    reg->b = data;
}

int emulate_instrucions(Register *reg, MMU *mmu)
{
    
    unsigned char *opcode =  &mmu->addr[reg->pc];
    disassemble_gb(mmu,reg,reg->pc);
    uint16_t pc_print = reg->pc;
    reg->pc+=1;
    int opbytes = 0;
   
	switch (read_byte(mmu, reg->pc-1))
	{
        case 0x00: break; //NOP
        case 0x01:              //LD BC, D16
        {
            reg->c = read_byte(mmu, reg->pc);
            reg->b = read_byte(mmu, reg->pc+1);
            reg->pc+=2;
            reg->clock.m = 12;
        }
        break;
        case 0x02:              //LD BC, A
        {
            uint16_t addr = read_bc_16bit(reg);
            write_byte(mmu, addr, reg->a);
            reg->clock.m = 8;
        } break;
        case 0x03:              //INC BC
        {
            uint16_t addr = read_bc_16bit(reg);
            addr+=1;
            write_bc_16bit(reg, addr);
            reg->clock.m = 8;
        }break;
        case 0x04:              //INC B
        {
            reg->cc.n = 0;
            reg->cc.h = ((reg->b & 0xf) == 0xf); 
            reg->b += 1;
            reg->cc.z = (reg->b == 0);
            reg->clock.m = 4;
        }
            break;
        case 0x05:              //DEC B
        {
            reg->b -= 1;
            reg->cc.n = 1;
            reg->cc.h = ((reg->b & 0x0f) == 0x0f);    
            reg->cc.z = (reg->b ==0);
            reg->clock.m = 4;
        }
            break;
        case 0x06:             //LD B, d8
            reg->b = read_byte(mmu, reg->pc);
            reg->clock.m = 8;
            reg->pc++;  
            break;
        case 0x07:             //RLCA 
        {
            uint8_t x = reg->a;
            reg->a = ((x&0x80) >> 7) | (x << 1);
            reg->cc.c = (1 == (x&1));
            reg->clock.m = 4;
        }
            break;
        case 0x08:             // LD (a16), SP
        {
            uint16_t addr = read_word(mmu, reg->pc);
            write_word(mmu, addr, reg->sp);
            reg->clock.m = 20;
            reg->pc+=2;
        }
            break;
        case 0x09:             // ADD HL, BC
        {
            uint16_t reg_hl = read_hl_16bit(reg);
            uint16_t reg_bc = read_bc_16bit(reg);
            reg->cc.n = 0;
            uint16_t ans = reg_hl + reg_bc;
            if(ans == 0) reg->cc.z = 0;
            reg->cc.c = (ans > 0xffff);
            reg->cc.h = (((reg_hl&0x0fff) + (reg_bc&0x0fff)) > 0x0fff);
            write_hl_16bit(reg,ans);
            reg->clock.m = 8;
        }
            break;
        case 0x0a:            //LD A, (BC)
        {
            uint16_t addr = read_bc_16bit(reg);
            reg->a  = read_byte(mmu, addr);
            reg->clock.m = 8;
        }
            break;
        case 0x0b:            //DEC BC
        {
            uint16_t addr = read_bc_16bit(reg);
            addr-=1;
            write_bc_16bit(reg, addr);
            reg->clock.m = 8;
        }
            break;
        case 0x0c:            //INC C
        {
            reg->cc.n = 0;
            reg->cc.h = ((reg->c & 0xf) == 0xf); 
            reg->c += 1;
            reg->cc.z = (reg->c == 0);
            reg->clock.m = 4;
        }
            break;
        case 0x0d:           //DEC C
        {
            reg->c -= 1;
            reg->cc.n = 1;
            reg->cc.h = ((reg->c & 0x0f) == 0x0f);    
            reg->cc.z = (reg->c ==0);
            reg->clock.m = 4;
        }
            break;
        case 0x0e:          //LD C, d8
            reg->c = read_byte(mmu, reg->pc);
            reg->clock.m = 8;
            reg->pc++; 
            break;
        case 0x0f:          //RRCA
        {
            uint8_t x = reg->a;
            reg->a = ((x&0x80) << 7) | (x >> 1);
            reg->cc.c = (1 == (x&1));
            reg->clock.m = 4;
        }
            break;
        case 0x10:          //STOP 0
            reg->pc++;
            reg->clock.m = 4;
            break;
        case 0x11:          //LD DE,d16
        {
            reg->e = read_byte(mmu, reg->pc);
            reg->d = read_byte(mmu, reg->pc+1);
            reg->pc+=2;
            reg->clock.m = 12;
        } 
            break;
        case 0x12:          //LD (DE),A
        {
            uint16_t addr = read_de_16bit(reg);
            write_byte(mmu, addr, reg->a);
            reg->clock.m = 8;
        } 
            break;
        case 0x13:          //INC DE
        {
            uint16_t addr = read_de_16bit(reg);
            addr+=1;
            write_de_16bit(reg, addr);
            reg->clock.m = 8;
        }
            break;
        case 0x14:          //INC D
        {
            reg->cc.n = 0;
            reg->cc.h = ((reg->d & 0xf) == 0xf); 
            reg->d += 1;
            reg->cc.z = (reg->d == 0);
            reg->clock.m = 4;
        }
            break;
        case 0x15:          //DEC D
        {
            reg->d -= 1;
            reg->cc.n = 1;
            reg->cc.h = ((reg->d & 0x0f) == 0x0f);    
            reg->cc.z = (reg->d ==0);
            reg->clock.m = 4;
        }
            break;
        case 0x16:          //LD D,d8 ***
        {
            reg->d = read_byte(mmu, reg->pc);
            reg->clock.m = 8;
            reg->pc++; 
        }
            break;
        case 0x17:          //RLA
            {
                uint8_t carry = reg->cc.c;
                reg->cc.c = (0x80 == (reg->a&0x80));
                uint8_t ans = (reg->a << 1) | reg->cc.c;
                reg->cc.z = (ans == 0);
                reg->a = ans;
                reg->cc.n = 0;
                reg->cc.h = 0;
                reg->clock.m = 4;
            }
                break;
        case 0x18:          //JR r8
            {
                int8_t r8_int = (int8_t)read_byte(mmu, reg->pc);
                reg->pc += r8_int;
                reg->clock.m = 12;
                reg->pc++;
            }
                break;
        case 0x19:          //ADD HL,DE
            {
                uint16_t reg_hl = read_hl_16bit(reg);
                uint16_t reg_de = read_de_16bit(reg);
                reg->cc.n = 0;;

                uint32_t ans = reg_hl + reg_de;
                if(ans == 0) reg->cc.z = 0;
                reg->cc.c = ((reg_hl + reg_de) > 0xbadb);
                reg->cc.h = (((reg_hl&0x0fff) + (reg_de&0x0fff)) > 0x0fff);
                write_hl_16bit(reg,ans);
                reg->clock.m = 8;
            }
            break;
        case 0x1a:          //LD A,(DE)
        {
            uint16_t addr = read_de_16bit(reg);
            uint16_t data = read_byte(mmu, addr);
            reg->a = data;
            reg->clock.m = 8;
        } 
            break;
        case 0x1b:          //DEC DE
        {
            uint16_t addr = read_de_16bit(reg);
            addr-=1;
            write_de_16bit(reg, addr);
            reg->clock.m = 8;
        }
            break;
        case 0x1c:          //INC E
        {
            reg->cc.n = 0;
            reg->cc.h = ((reg->e & 0xf) == 0xf); 
            reg->e += 1;
            reg->cc.z = (reg->e == 0);
            reg->clock.m = 4;
        }
            break;
        case 0x1d:          //DEC E
        {
            reg->e -= 1;
            reg->cc.n = 1;
            reg->cc.h = ((reg->e & 0x0f) == 0x0f);    
            reg->cc.z = (reg->e ==0);
            reg->clock.m = 4;
        }
            break;
        case 0x1e:          //LD E,d8
        {
            reg->e= read_byte(mmu, reg->pc);
            reg->clock.m = 8;
            reg->pc++; 
        }break;
        case 0x1f:          //RRA
        {
            uint8_t carry = reg->cc.c;
            reg->cc.c = (0x80 == (reg->a&0x80));
            uint8_t ans = (reg->a >> 1) | (reg->cc.c << 7);
            reg->cc.z = (ans == 0);
            reg->a = ans;
            reg->cc.n = 0;
            reg->cc.h = 0;
            reg->clock.m = 4;
        }break;

        case 0x20:          //JR NZ, R8
        {
            int8_t offset = (int8_t)read_byte(mmu, reg->pc);
            if (reg->cc.z==0)
            {
                reg->pc += offset;
                reg->clock.m = 12;
            } else
            {
                reg->clock.m = 8;
            }
            reg->pc += 1;
        }
            break;
        case 0x21:          //LD HL,d16
        {
            reg->l = read_byte(mmu, reg->pc);
            reg->h = read_byte(mmu, reg->pc+1);
            reg->pc+=2;
            reg->clock.m = 12;
        } 
            break;
        case 0x22:          //LD (HL+), A
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->a);
            addr++;
            write_hl_16bit(reg, addr);
            reg->clock.m = 8;
        } 
            break;
        case 0x23:          //INC HL
        {
            uint16_t addr = read_hl_16bit(reg);
            addr+=1;
            write_hl_16bit(reg, addr);
            reg->clock.m = 8;
        }
            break;
        case 0x24:          //INC H
        {
            reg->cc.n = 0;
            reg->cc.h = ((reg->h & 0xf) == 0xf); 
            reg->h += 1;
            reg->cc.z = (reg->h == 0);
            reg->clock.m = 4;
        }
            break;
        case 0x25:          //DEC H
        {
            reg->h -= 1;
            reg->cc.n = 1;
            reg->cc.h = ((reg->h & 0x0f) == 0x0f);    
            reg->cc.z = (reg->h ==0);
            reg->clock.m = 4;
        }
            break;
        case 0x26:          //LD H,d8 ***
        {
            reg->h = read_byte(mmu, reg->pc);
            reg->clock.m = 8;
            reg->pc++; 
        }
            break;
        case 0x27:          //DAA ****
        {
            uint8_t a = reg->a;

            if(!reg->cc.n)
            {
                if(reg->cc.c || a > 0x99)
                {
                    a+= 0x60;
                    reg->cc.c = 1;
                }
                if(reg->cc.h || (reg->a & 0x0f) > 0x09)
                {
                    a += 0x06;
                }
            }
            else
            {
                if(reg->cc.c)
                    a -= 0x60;
                if(reg->cc.h)
                    a -= 0x60;
            }
            reg->cc.z = (a == 0);
            reg->cc.h = 0;
            reg->a = a;
            reg->clock.m = 4;
        }
            break;
        case 0x28:          //JR Z, r8
        {
            if (reg->cc.z == 1)
            {
                int8_t r8_int = (int8_t)read_byte(mmu, reg->pc);
                reg->pc += r8_int;
                reg->clock.m = 12;
            } else 
            {
                reg->clock.m = 8;
            }
            reg->pc++;
        }
            break;
        case 0x29:          //ADD HL,HL
        {
            uint16_t reg_hl = read_hl_16bit(reg);
            reg->cc.n = 0;
            uint16_t ans = reg_hl + reg_hl;
            if(ans == 0) reg->cc.z = 0;
            reg->cc.c = (ans > 0xffff);
            reg->cc.h = (((reg_hl&0x0fff) + (reg_hl&0x0fff)) > 0x0fff);
            write_hl_16bit(reg,ans);
            reg->clock.m = 8;
        }
            break;
        case 0x2a:          //LD A, (HL+)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint16_t data = read_byte(mmu, addr);
            reg->a = data;
            addr++;
            write_hl_16bit(reg, addr);
            reg->clock.m = 8;
        } 
            break;
        case 0x2b:          //DEC HL
        {
            uint16_t addr = read_hl_16bit(reg);
            addr-=1;
            write_hl_16bit(reg, addr);
            reg->clock.m = 8;
        }
            break;
        case 0x2c:          //INC L
        {
            reg->cc.n = 0;
            reg->cc.h = ((reg->l & 0xf) == 0xf); 
            reg->l += 1;
            reg->cc.z = (reg->l == 0);
            reg->clock.m = 4;
        }
            break;
        case 0x2d:          //DEC L
        {
            reg->l -= 1;
            reg->cc.n = 1;
            reg->cc.h = ((reg->l & 0x0f) == 0x0f);    
            reg->cc.z = (reg->l ==0);
            reg->clock.m = 4;
        }
            break;
        case 0x2e:          //LD L,d8
        {
            reg->l = read_byte(mmu, reg->pc);
            reg->clock.m = 8;
            reg->pc++; 
        }
            break;
        case 0x2f:          //CPL
        {
            uint8_t reg_a = reg->a;
            reg_a = ~reg->a;
            reg->a = reg_a;
            reg->cc.n = 1; 
            reg->cc.h = 1;
            reg->clock.m = 4;
        }  
            break;
            
/*****************************************************************/

        case 0x30:          //JR NC, R8
        {
            if (!reg->cc.c)
            {
                int8_t r8_int = (int8_t)read_byte(mmu, reg->pc);
                reg->pc += r8_int;
                reg->clock.m = 12;
            } else 
            {
                reg->clock.m = 8;
            }
            reg->pc++;
        }
            break;
        case 0x31:          //LD SP,d16 ***check
        {
            uint16_t immediate = read_word(mmu, reg->pc);
            reg->sp = immediate;
            reg->clock.m = 12;
            reg->pc+=2;
        } 
            break;
        case 0x32:          //LD (HL-),A
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->a);
            addr--;
            write_hl_16bit(reg, addr);
            reg->clock.m = 8;
        } 
            break;
        case 0x33:          //INC SP
        {
            uint16_t addr = reg->sp;
            addr+=1;
            reg->sp = addr;
            reg->clock.m = 8;
        }
            break;
        case 0x34:          //INC (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->cc.n = 0;
            reg->cc.h = ((data&0xf) == 0xf);
            data++;
            reg->cc.z = (data == 0);
            write_byte(mmu, addr, data);
            reg->clock.m = 12;
        }
            break;
        case 0x35:          //DEC (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            data--;
            reg->cc.n = 1;
            reg->cc.h = ((data&0xf) == 0xf);
            reg->cc.z = (data == 0);
            write_byte(mmu, addr, data);
            reg->clock.m = 12;
        }
            break;
        case 0x36:          //LD (HL),d8 ***
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, reg->pc);
            write_byte(mmu, addr, data);
            reg->pc++;
            reg->clock.m = 12;
        }
            break;
        case 0x37:          //SCF ****
            reg->cc.n = 0;
            reg->cc.h = 0;
            reg->cc.c = 1;
            reg->clock.m = 4;
            break;
        case 0x38:          //JR C, r8
        {
            if (reg->cc.c)
            {
                int8_t r8_int = (int8_t)read_byte(mmu, reg->pc);
                reg->pc += r8_int;
                reg->clock.m = 12;
            } else 
            {
                reg->clock.m = 8;
            }
            reg->pc++;
        }
            break;
        case 0x39:          //ADD HL,SP
        {
            uint16_t reg_hl = read_hl_16bit(reg);
            uint16_t reg_sp = reg->sp;
            reg->cc.n = 0;
            uint16_t ans = reg_hl + reg_sp;
            if(ans == 0) reg->cc.z = 0;
            reg->cc.c = (ans > 0xffff);
            reg->cc.h = (((reg_hl&0x0fff) + (reg_sp&0x0fff)) > 0x0fff);
            write_hl_16bit(reg,ans);
            reg->clock.m = 8;
        } 
            break;
        case 0x3a:          //LD A, HL-
        {
            uint16_t addr = read_hl_16bit(reg);
            uint16_t data = read_byte(mmu, addr);
            write_byte(mmu, reg->a, data);
            addr--;
            write_hl_16bit(reg, addr);
            reg->clock.m = 8;
        } 
            break;
        case 0x3b:          //DEC SP
        {
            uint16_t addr = reg->sp;
            addr-=1;
            reg->sp = addr;
            reg->clock.m = 8;
        }
            break;
        case 0x3c:          //INC A
        {
            reg->cc.n = 0;
            reg->cc.h = ((reg->a & 0xf) == 0xf); 
            reg->a += 1;
            reg->cc.z = (reg->a == 0);
            reg->clock.m = 4;
        }
            break;
        case 0x3d:          //DEC A
        {
            reg->a -= 1;
            reg->cc.n = 1;
            reg->cc.h = ((reg->a & 0x0f) == 0x0f);    
            reg->cc.z = (reg->a ==0);
            reg->clock.m = 4;
        }
            break;
        case 0x3e:          //LD A,d8
        {
            uint8_t byte = read_byte(mmu, reg->pc);
            reg->a = byte;
            reg->clock.m = 8;
            reg->pc++; 
        }
            break;
        case 0x3f:          //CCF
        {
            reg->cc.n = 0; //***** check
            reg->cc.h = 0;
            reg->cc.c = ~reg->cc.c & 1;
            reg->clock.m = 4;
        }  
            break;
        
/*****************************************************************/

        case 0x40:          //LD B, B
            reg->b = reg->b; 
            reg->clock.m = 4;
            break;
        case 0x41:          //LD B, C
            reg->b = reg->c; 
            reg->clock.m = 4;
            break;
        case 0x42:          //LD B, D
            reg->b = reg->d; 
            reg->clock.m = 4;
            break;
        case 0x43:          //LD B, E
            reg->b = reg->e;
            reg->clock.m = 4;
            break;
        case 0x44:          //LD B, H
            reg->b = reg->h; 
            reg->clock.m = 4;
            break;
        case 0x45:          //LD B, L
            reg->b = reg->l; 
            reg->clock.m = 4;
            break;
        case 0x46:          //LD B, (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->b = data; 
            reg->clock.m = 8;
        }
            break;
        case 0x47:          //LD B, A
            reg->b = reg->a; 
            reg->clock.m = 4;
            break;
        case 0x48:          //LD C, B
            reg->c = reg->b; 
            reg->clock.m = 4;
            break;
        case 0x49:          //LD C, C
            reg->c = reg->c;
            reg->clock.m = 4;
            break;
        case 0x4a:          //LD C, D
            reg->c = reg->d; 
            reg->clock.m = 4;
            break;
        case 0x4b:          //LD C, E
            reg->c = reg->e; 
            reg->clock.m = 4;
            break;
        case 0x4c:          //LD C, H
            reg->c = reg->h; 
            reg->clock.m = 4;
            break;
        case 0x4d:          //LD C, L
            reg->c = reg->l; 
            reg->clock.m = 4;
            break;
        case 0x4e:          //LD C, (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->c = data; 
            reg->clock.m = 8;
        }
            break;
        case 0x4f:          //LD C, A
            reg->c = reg->a; 
            reg->clock.m = 4;
            break;

        /*****************************************************************/

        case 0x50:          //LD D, B
            reg->d = reg->b; 
            reg->clock.m = 4;
            break;
        case 0x51:          //LD D, C
            reg->d = reg->c; 
            reg->clock.m = 4;
            break;
        case 0x52:          //LD D, D
            reg->d = reg->d; 
            reg->clock.m = 4;
            break;
        case 0x53:          //LD D, E
            reg->d = reg->e; 
            reg->clock.m = 4;
            break;
        case 0x54:          //LD D, H
            reg->d = reg->h; 
            reg->clock.m = 4;
            break;
        case 0x55:          //LD D, L
            reg->d = reg->l; 
            reg->clock.m = 4;
            break;
        case 0x56:          //LD D, (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->d = data; 
            reg->clock.m = 8;
        }
            break;
        case 0x57:          //LD D, A
            reg->d = reg->a; 
            reg->clock.m = 4;
            break;
        case 0x58:          //LD E, B
            reg->e = reg->b; 
            reg->clock.m = 4;
            break;
        case 0x59:          //LD E, C
            reg->e = reg->c; 
            reg->clock.m = 4;
            break;
        case 0x5a:          //LD E, D
            reg->e = reg->d; 
            reg->clock.m = 4;
            break;
        case 0x5b:          //LD E, E
            reg->e = reg->e; 
            reg->clock.m = 4;
            break;
        case 0x5c:          //LD E, H
            reg->e = reg->h; 
            reg->clock.m = 4;
            break;
        case 0x5d:          //LD E, L
            reg->e = reg->l; 
            reg->clock.m = 4;
            break;
        case 0x5e:          //LD E, (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->e = data; 
            reg->clock.m = 8;
        }
            break;
        case 0x5f:          //LD E, A
            reg->e = reg->a; 
            reg->clock.m = 4;
            break;
        
        /*****************************************************************/

        case 0x60:          //LD H, B
            reg->h = reg->b; 
            reg->clock.m = 4;
            break;
        case 0x61:          //LD H, C
            reg->h = reg->c; 
            reg->clock.m = 4;
            break;
        case 0x62:          //LD H, D
            reg->h = reg->d; 
            reg->clock.m = 4;
            break;
        case 0x63:          //LD H, E
            reg->h = reg->e; 
            reg->clock.m = 4;
            break;
        case 0x64:          //LD H, H
            reg->h = reg->h; 
            reg->clock.m = 4;
            break;
        case 0x65:          //LD H, L
            reg->h = reg->l; 
            reg->clock.m = 4;
            break;
        case 0x66:          //LD H, (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->h = data; 
            reg->clock.m = 8;
        }
            break;
        case 0x67:          //LD H, A
            reg->h = reg->a; 
            reg->clock.m = 4;
            break;
        case 0x68:          //LD L, B
            reg->l = reg->b; 
            reg->clock.m = 4;
            break;
        case 0x69:          //LD L, C
            reg->l = reg->c; 
            reg->clock.m = 4;
            break;
        case 0x6a:          //LD L, D
            reg->l = reg->d; 
            reg->clock.m = 4;
            break;
        case 0x6b:          //LD L, E
            reg->l = reg->e; 
            reg->clock.m = 4;
            break;
        case 0x6c:          //LD L, H
            reg->l = reg->h; 
            reg->clock.m = 4;
            break;
        case 0x6d:          //LD L, L
            reg->l = reg->l; 
            reg->clock.m = 4;
            break;
        case 0x6e:          //LD L, (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->l = data; 
            reg->clock.m = 8;
        }
            break;
        case 0x6f:          //LD L, A
            reg->l = reg->a; 
            reg->clock.m = 4;
            break;
        
        /*****************************************************************/

        case 0x70:          //LD (HL), B
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->b);
            reg->clock.m = 8;
        }
            break;
        case 0x71:          //LD (HL), C
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->c);
            reg->clock.m = 8;
        }
            break;
        case 0x72:          //LD (HL), D
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->d);
            reg->clock.m = 8;
        }
            break;
        case 0x73:          //LD (HL), E
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->e);
            reg->clock.m = 8;
        }
            break;
        case 0x74:          //LD (HL), H
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->h);
            reg->clock.m = 8;
        }
            break;
        case 0x75:          //LD (HL), L
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->l);
            reg->clock.m = 8;
        }
            break;
        case 0x76:          //HALT
            reg->cc.halt = 1;
            reg->clock.m = 4;
            break;
        case 0x77:          //LD (HL), A
        {
            uint16_t addr = read_hl_16bit(reg);
            write_byte(mmu, addr, reg->a);
            reg->clock.m = 8;
        }
            break;
        case 0x78:          //LD A, B
            reg->a = reg->b; 
            reg->clock.m = 4;
            break;
        case 0x79:          //LD A, C
            reg->a = reg->c; 
            reg->clock.m = 4;
            break;
        case 0x7a:          //LD A, D
            reg->a = reg->d; 
            reg->clock.m = 4;
            break;
        case 0x7b:          //LD A, E
            reg->a = reg->e; 
            reg->clock.m = 4;
            break;
        case 0x7c:          //LD A, H
            reg->a = reg->h; 
            reg->clock.m = 4;
            break;
        case 0x7d:          //LD A, L
            reg->a = reg->l; 
            reg->clock.m = 4;
            break;
        case 0x7e:          //LD A, (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t data = read_byte(mmu, addr);
            reg->a = data; 
            reg->clock.m = 8;
        }
            break;
        case 0x7f:          //LD A, A
            reg->a = reg->a; 
            reg->clock.m = 4;
            break;

        /*****************************************************************/

        case 0x80:          //ADD A,B
        {
            uint8_t a = reg->a;
            uint8_t b = reg->b;
            reg->cc.n = 0;
            uint8_t ans = a + b;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + b) > 0xff);
            reg->cc.h = ((a&0xf)+(b&0xf) > 0xf);     
            reg->clock.m = 4;       
        }
            break;
        case 0x81:          //ADD A,C
        {
            uint8_t a = reg->a;
            uint8_t c = reg->b;
            reg->cc.n = 0;
            uint8_t ans = a + c;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + c) > 0xff);
            reg->cc.h = ((a&0xf)+(c&0xf) > 0xf);
            reg->clock.m = 4;            
        }
            break;
        case 0x82:          //ADD A,D
        {
            uint8_t a = reg->a;
            uint8_t d = reg->d;
            reg->cc.n = 0;
            uint8_t ans = a + d;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + d) > 0xff);
            reg->cc.h = ((a&0xf)+(d&0xf) > 0xf);    
            reg->clock.m = 4;        
        }
            break;
        case 0x83:          //ADD A,E
        {
            uint8_t a = reg->a;
            uint8_t val = reg->e;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);    
            reg->clock.m = 4;        
        }
            break;
        case 0x84:          //ADD A,H
        {
            uint8_t a = reg->a;
            uint8_t val = reg->h;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);  
            reg->clock.m = 4;          
        }
            break;
        case 0x85:          //ADD A,L
        {
            uint8_t a = reg->a;
            uint8_t val = reg->l;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);      
            reg->clock.m = 4;      
        }
            break;
        case 0x86:          //ADD A, (HL)
        {
            uint8_t a = reg->a;
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr);
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);      
            reg->clock.m = 8;      
        }
            break;
        case 0x87:          //ADD A, A
        {
            uint8_t a = reg->a;
            uint8_t val = reg->a + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);      
            reg->clock.m = 4;      
        }
            break;
        case 0x88:          //ADC A, B
        {
            uint8_t a = reg->a;
            uint8_t val = reg->b + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);     
            reg->clock.m = 4;       
        }
            break;
        case 0x89:          //ADC A, C
        {
            uint8_t a = reg->a;
            uint8_t val = reg->c + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);       
            reg->clock.m = 4;     
        }
            break;
        case 0x8a:          //ADC A, D
        {
            uint8_t a = reg->a;
            uint8_t val = reg->d + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);    
            reg->clock.m = 4;        
        }
            break;
        case 0x8b:          //ADC A, E
        {
            uint8_t a = reg->a;
            uint8_t val = reg->e + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);      
            reg->clock.m = 4;      
        }
            break;
        case 0x8c:          //ADC A, H
        {
            uint8_t a = reg->a;
            uint8_t val = reg->h + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);    
            reg->clock.m = 4;        
        }
            break;
        case 0x8d:          //ADC A, L
        {
            uint8_t a = reg->a;
            uint8_t val = reg->l + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);      
            reg->clock.m = 4;      
        }
            break;
        case 0x8e:          //ADC A, (HL)
        {
            uint8_t a = reg->a;
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr) + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);      
            reg->clock.m = 8;      
        }
            break;
        case 0x8f:          //ADC A, A
        {
            uint8_t a = reg->a;
            uint8_t val = reg->a + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf); 
            reg->clock.m = 4;           
        }
            break;

            /*****************************************************************/

        case 0x90:          //SUB A,B
        {
            reg->cc.n = 1;
            uint8_t val = reg->b;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;          
        }
            break;
        case 0x91:          //SUB A,C
        {
            reg->cc.n = 1;
            uint8_t val = reg->c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;         
        }
            break;
        case 0x92:          //SUB A,D
        {
            reg->cc.n = 1;
            uint8_t val = reg->d;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;         
        }
            break;
        case 0x93:          //SUB A,E
        {
            reg->cc.n = 1;
            uint8_t val = reg->e;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;         
        }
            break;
        case 0x94:          //SUB A,H
        {
            reg->cc.n = 1;
            uint8_t val = reg->h;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;         
        }
            break;
        case 0x95:          //SUB A,L
        {
            reg->cc.n = 1;
            uint8_t val = reg->l;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;         
        }
            break;
        case 0x96:          //SUB A, (HL)
        {
            reg->cc.n = 1;
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr);
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 8;  
        }
            break;
        case 0x97:          //SUB A, A
        {
            reg->cc.n = 1;
            uint8_t val = reg->a;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;         
        }
            break;
        case 0x98:          //SBC A, B
        {
            reg->cc.n = 1;
            uint8_t val = reg->b + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;
        }
            break;
        case 0x99:          //SBC A, C
        {
            reg->cc.n = 1;
            uint8_t val = reg->c + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;          
        }
            break;
        case 0x9a:          //SBC A, D
        {
            reg->cc.n = 1;
            uint8_t val = reg->d + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;         
        }
            break;
        case 0x9b:          //SBC A, E
        {
            reg->cc.n = 1;
            uint8_t val = reg->e + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;          
        }
            break;
        case 0x9c:          //SBC A, H
        {
            reg->cc.n = 1;
            uint8_t val = reg->h + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;          
        }
            break;
        case 0x9d:          //SBC A, L
        {
            reg->cc.n = 1;
            uint8_t val = reg->l + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;  
        }
            break;
        case 0x9e:          //SBC A, (HL)
        {
            reg->cc.n = 1;
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr) + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 8;
        }
            break;
        case 0x9f:          //SBC A, A
        {
            reg->cc.n = 1;
            uint8_t val = reg->a + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 4;  
        }
            break;

        /********************************************************/
        
        case 0xa0:          //AND B
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= reg->b;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa1:          //AND C
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= reg->c;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa2:          //AND D
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= reg->d;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa3:          //AND E
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= reg->e;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa4:          //AND H
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= reg->h;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa5:          //AND L
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= reg->l;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa6:          //AND (HL)
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr);
            reg->a &= val;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 8;
        }
            break;
        case 0xa7:          //AND A
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= reg->a;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa8:          //XOR B
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= reg->b;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xa9:          //XOR C
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= reg->c;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xaa:          //XOR D
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= reg->d;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xab:          //XOR E
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= reg->e;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xac:          //XOR H
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= reg->h;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xad:          //XOR L
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= reg->l;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xae:          //XOR (HL)
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr);
            reg->a ^= val;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 8;
        }
            break;
        case 0xaf:          //XOR A
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= reg->a;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        
        /********************************************************/
        
        case 0xb0:          //OR B
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= reg->b;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xb1:          //OR C
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= reg->c;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xb2:          //OR D
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= reg->d;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xb3:          //OR E
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= reg->e;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xb4:          //OR H
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= reg->h;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xb5:          //OR L
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= reg->l;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xb6:          //OR (HL)
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr);
            reg->a |= val;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 8;
        }
            break;
        case 0xb7:          //OR A
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= reg->a;
            reg->cc.z = (reg->a==0);
            reg->clock.m = 4;
        }
            break;
        case 0xb8:          //CP B
        {
            reg->cc.n = 1;
            reg->cc.z = (reg->a==reg->b);
            reg->cc.h = ((reg->a & 0xf) - (reg->b & 0xf)) < 0;
            reg->cc.c = (reg->a < reg->b);
            reg->clock.m = 4;
        }
            break;
        case 0xb9:          //CP C
        {
            reg->cc.n = 1;
            reg->cc.z = (reg->a==reg->c);
            reg->cc.h = ((reg->a & 0xf) - (reg->c & 0xf)) < 0;
            reg->cc.c = (reg->a < reg->b);
            reg->clock.m = 4;
        }
            break;
        case 0xba:          //CP D
        {
            reg->cc.n = 1;
            reg->cc.z = (reg->a==reg->d);
            reg->cc.h = ((reg->a & 0xf) - (reg->d & 0xf)) < 0;
            reg->cc.c = (reg->a < reg->d);
            reg->clock.m = 4;
        }
            break;
        case 0xbb:          //CP E
        {
            reg->cc.n = 1;
            reg->cc.z = (reg->a==reg->e);
            reg->cc.h = ((reg->a & 0xf) - (reg->e & 0xf)) < 0;
            reg->cc.c = (reg->a < reg->e);
            reg->clock.m = 4;
        }
            break;
        case 0xbc:          //CP H
        {
            reg->cc.n = 1;
            reg->cc.z = (reg->a==reg->h);
            reg->cc.h = ((reg->a & 0xf) - (reg->h & 0xf)) < 0;
            reg->cc.c = (reg->a < reg->h);
            reg->clock.m = 4;
        }
            break;
        case 0xbd:          //CP L
        {
            reg->cc.n = 1;
            reg->cc.z = (reg->a==reg->c);
            reg->cc.h = ((reg->a & 0xf) - (reg->c & 0xf)) < 0;
            reg->cc.c = (reg->a < reg->c);
            reg->clock.m = 4;
        }
            break;
        case 0xbe:          //CP (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            uint8_t val = read_byte(mmu, addr);
            reg->cc.n = 1;
            reg->cc.z = (reg->a==val);
            reg->cc.h = ((reg->a & 0xf) - (val & 0xf)) < 0;
            reg->cc.c = (reg->a < val);
            reg->clock.m = 8;
        }
            break;
        case 0xbf:          //CP A
        {
            reg->cc.n = 1;
            uint8_t val = reg->a;
            reg->cc.z = (reg->a==val);
            reg->cc.h = ((reg->a & 0xf) - (val & 0xf)) < 0;
            reg->cc.c = (reg->a < val);
            reg->clock.m = 4;
        }
            break;

        /********************************************************/
        
        case 0xc0:          //RET NZ
        {
            if(reg->cc.z == 0)
            {
                uint16_t addr = read_word(mmu, reg->sp);
                reg->sp += 2;
                reg->pc = addr;
                reg->clock.m = 20;
            } else
            {
                reg->clock.m = 12;
            }
        }
            break;
        case 0xc1:          //POP BC
        {
            pop(mmu, reg, &reg->b, &reg->c);
            reg->clock.m = 12;
        }
            break;
        case 0xc2:          //JP NZ, A16
        {
            if (reg->cc.z == 0)
            {
                uint16_t addr = read_word(mmu, reg->pc);
                reg->pc = addr - 0x02;
                reg->clock.m = 16;
            }
            else
            {
                reg->pc += 2;
                reg->clock.m = 12;
            }
        }
            break;
        case 0xc3:          //JP a16
        {
            
            uint16_t addr = read_word(mmu, reg->pc);
            reg->pc = addr - 0x02;
            reg->clock.m = 16;
            reg->pc += 2;
        }
            break;
        case 0xc4:          //CALL NZ,a16
        {
            if(reg->cc.z == 0)
            {
                reg->sp -=2;
                uint16_t addr = read_word(mmu, reg->pc);
                write_word(mmu, reg->sp, reg->pc + 2);
                reg->pc = addr;
                reg->clock.m = 24;
            }
            else
            {
                reg->pc += 2;
                reg->clock.m = 12;   
            }
        }
            break;
        case 0xc5:          //PUSH BC
        {
            push(mmu, reg, reg->b, reg->c);
            reg->clock.m = 16;
        }
            break;
        case 0xc6:          //ADD A,d8
        {
      
            uint8_t a = reg->a;
            reg->cc.n = 0;
            uint8_t ans = a + read_byte(mmu, reg->pc);
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + read_byte(mmu, reg->pc)) > 0xff);
            reg->cc.h = ((a&0xf)+(read_byte(mmu, reg->pc)&0xf) > 0xf);
            reg->a = ans;
            reg->pc++;
            reg->clock.m = 8;
        }
            break;
        case 0xc7:          //RST 00H
        {
            reg->sp -=2;
            write_word(mmu, 0x0000, reg->pc + 1);
            reg->pc = 0x0000;
            reg->clock.m = 16;
        }
            break;
        case 0xc8:          //RET Z
        {
            if(reg->cc.z == 1)
            {
                uint16_t addr = read_word(mmu, reg->sp);
                reg->sp += 2;
                reg->pc = addr;
                reg->clock.m = 20;
            } else
            {
                reg->clock.m = 8;
            }
        }
            break;
        case 0xc9:          //RET
        {
            uint16_t addr = (read_byte(mmu, reg->sp + 1) << 8) | read_byte(mmu, reg->sp);
            printf("\n**Sp[%04x]***\n", read_word(mmu,0xdff9));
            reg->pc = addr;
            reg->sp += 2;
            reg->clock.m = 16;
        }
            break;
        case 0xca:          //JP Z,a16
        {
            if (reg->cc.z == 1)
            {
                uint16_t addr = read_word(mmu, reg->pc);
                reg->pc = addr - 0x02;
                reg->clock.m = 16;
            }
            else
            {
                reg->pc += 2;
                reg->clock.m = 12;
            }
        }
            break;
        case 0xcb:          //PREFIX CB
            switch (read_byte(mmu, reg->pc+1))
            {
                case 0x00:      //RLC B
                {
                    uint8_t x = reg->b;
                    reg->b = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->b == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x01:      //RLC C
                {
                    uint8_t x = reg->c;
                    reg->c = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->c == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x02:      //RLC D
                {
                    uint8_t x = reg->d;
                    reg->d = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->d == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x03:      //RLC E
                {
                    uint8_t x = reg->e;
                    reg->e = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->e == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x04:      //RLC H
                {
                    uint8_t x = reg->h;
                    reg->h = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->h == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x05:      //RLC L
                {
                    uint8_t x = reg->l;
                    reg->l = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->l == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x06:      //RLC HL
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t x = ((hl_data&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->hl == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    write_byte(mmu, hl_data, x);
                } break;
                case 0x07:      //RLC A
                {
                    uint8_t x = reg->a;
                    reg->a = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->a == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x08:      //RRC B
                {
                    uint8_t x = reg->b;
                    reg->b = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->b == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x09:      //RRC C
                {
                    uint8_t x = reg->c;
                    reg->c = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->c == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x0a:      //RRC D
                {
                    uint8_t x = reg->d;
                    reg->d = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->d == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x0b:      //RRC E
                {
                    uint8_t x = reg->e;
                    reg->e = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->b == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break; 
                case 0x0c:      //RRC H
                {
                    uint8_t x = reg->h;
                    reg->h = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->h == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                }break;
                case 0x0d:      //RRC L
                {
                    uint8_t x = reg->l;
                    reg->b = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->b == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                }break;
                case 0x0e:      //RRC HL
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t x = ((hl_data&0x80) >> 7) | (x << 1);
                    reg->cc.z = (x == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    write_byte(mmu, reg_hl, x);
                }break;
                case 0x0f:      //RRC A
                {
                    uint8_t x = reg->a;
                    reg->a = ((x&0x80) >> 7) | (x << 1);
                    reg->cc.z = (reg->a == 0);
                    reg->cc.c = (1 == (x&1));
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                }break;
                case 0x10:      //RL B
                {
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint8_t x = ((reg->b << 1) | carry);
                    reg->cc.z = (x == 0);
                    reg->b = x;
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x11:      //RL C
                {
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint8_t x = ((reg->c << 1) | carry);
                    reg->cc.z = (x == 0);
                    reg->c = x;
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x12:      //RL D
                {
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint8_t x = ((reg->c << 1) | carry);
                    reg->cc.z = (x == 0);
                    reg->d = x;
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x13:      //RL E
                {
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint8_t x = ((reg->e << 1) | carry);
                    reg->cc.z = (x == 0);
                    reg->e = x;
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x14:      //RL H
                {
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint8_t x = ((reg->h << 1) | carry);
                    reg->cc.z = (x == 0);
                    reg->h = x;
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                }break; 
                case 0x15:      //RL L
                {
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint8_t x = ((reg->l << 1) | carry);
                    reg->cc.z = (x == 0);
                    reg->l = x;
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x16:      //RL HL
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint8_t x = ((hl_data << 1) | carry);
                    reg->cc.z = (x == 0);
                    write_byte(mmu, reg_hl, x);
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                } break;
                case 0x17:      //RL A
                {
                    uint8_t carry = reg->cc.c;
                    reg->cc.c = 0;
                    uint16_t x = ((reg->a << 1) | carry);
                    reg->cc.z = (x == 0);
                    reg->a = x;
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                }break;
                case 0x18:      //RR B
                {
                    uint8_t reg_letter = reg->b;
                    reg->b = reg->cc.c | (reg_letter >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (reg->b == 0);
                    reg->cc.n = 0;
                }break;
                case 0x19:      //RR C
                {
                    uint8_t reg_letter = reg->c;
                    reg->c = reg->cc.c | (reg_letter >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (reg->c == 0);
                    reg->cc.n = 0;
                }break;
                case 0x1a:      //RR D
                {
                    uint8_t reg_letter = reg->d;
                    reg->d = reg->cc.c | (reg_letter >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (reg->d == 0);
                    reg->cc.n = 0;
                }break;
                case 0x1b:      //RR E
                {
                    uint8_t reg_letter = reg->e;
                    reg->e = reg->cc.c | (reg_letter >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (reg->e == 0);
                    reg->cc.n = 0;  
                } break;
                case 0x1c:      //RR H
                {
                    uint8_t reg_letter = reg->h;
                    reg->h = reg->cc.c | (reg_letter >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (reg->h == 0);
                    reg->cc.n = 0;
                }break;
                case 0x1d:      //RR L
                {
                    uint8_t reg_letter = reg->l;
                    reg->l = reg->cc.c | (reg_letter >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (reg->l == 0);
                    reg->cc.n = 0;
                }break;
                case 0x1e:      //RR HL
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t x = reg->cc.c | (hl_data >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (x == 0);
                    write_byte(mmu, reg_hl, x);
                    reg->cc.n = 0;
                }break;
                case 0x1f:      //RR A
                {
                    uint8_t reg_letter = reg->a;
                    reg->a = reg->cc.c | (reg_letter >> 1);
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    reg->cc.z = (reg->a == 0);
                    reg->cc.n = 0;
                }break;
                case 0x20:      //SLA B
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->b & 0x01)==0x01);
                    uint8_t x = ((reg->b << 1));
                    reg->cc.c = carry;
                    reg->b = x;
                    reg->cc.z = (x == 0);
                } break;
                case 0x21:      //SLA C
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->c & 0x01)==0x01);
                    uint8_t x = ((reg->c << 1));
                    reg->cc.c = carry;
                    reg->c = x;
                    reg->cc.z = (x == 0);
                } break;
                case 0x22:      //SLA D
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->d & 0x01)==0x01);
                    uint8_t x = ((reg->d << 1));
                    reg->cc.c = carry;
                    reg->d = x;
                    reg->cc.z = (x == 0);
                } break;
                case 0x23:      //SLA E
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->e & 0x01)==0x01);
                    uint8_t x = ((reg->e << 1));
                    reg->cc.c = carry;
                    reg->e = x;
                    reg->cc.z = (x == 0);
                } break;
                case 0x24:      //SLA H
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->h & 0x01)==0x01);
                    uint8_t x = ((reg->h << 1));
                    reg->cc.c = carry;
                    reg->h = x;
                    reg->cc.z = (x == 0);
                } break;
                case 0x25:      //SLA L
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->l & 0x01)==0x01);
                    uint8_t x = ((reg->l << 1));
                    reg->cc.c = carry;
                    reg->l = x;
                    reg->cc.z = (x == 0);
                } break;
                case 0x26:      //SLA HL
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t carry = ((hl_data & 0x01)==0x01);
                    uint8_t x = ((hl_data << 1));
                    reg->cc.c = carry;
                    write_byte(mmu, reg_hl, x);
                    reg->cc.z = (x == 0);
                } break;
                case 0x27:      //SLA A
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = ((reg->a & 0x01)==0x01);
                    uint8_t x = ((reg->l << 1));
                    reg->a = x;
                    reg->cc.z = (x == 0);
                }break;
                case 0x28:      //SRA B
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->b & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((reg->b >> 1) | carry);
                    reg->b = x;
                    reg->cc.z = (x == 0);
                }break;
                case 0x29:      //SRA C
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->c & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((reg->c >> 1) | carry);
                    reg->c = x;
                    reg->cc.z = (x == 0);
                }break;
                case 0x2a:      //SRA D
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->d & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((reg->d >> 1) | carry);
                    reg->d = x;
                    reg->cc.z = (x == 0);
                }break;
                case 0x2b:      //SRA E
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->e & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((reg->e >> 1) | carry);
                    reg->e = x;
                    reg->cc.z = (x == 0);
                }  break; 
                case 0x2c:      //SRA H
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->h & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((reg->h >> 1) | carry);
                    reg->h = x;
                    reg->cc.z = (x == 0);
                }break;
                case 0x2d:      //SRA L
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->l & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((reg->l >> 1) | carry);
                    reg->l = x;
                    reg->cc.z = (x == 0);
                }break;
                case 0x2e:      //SRA HL
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t carry = ((hl_data & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((hl_data >> 1) | carry);
                    write_byte(mmu, reg_hl, x);
                    reg->cc.z = (x == 0);
                }break;
                case 0x2f:      //SRA A
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->a & 0x80)==0x80);
                    reg->cc.c = carry;
                    uint8_t x = ((reg->a >> 1) | carry);
                    reg->a = x;
                    reg->cc.z = (x == 0);
                }break;
                case 0x30:      //SWAP B
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (reg->b & 0xf0);
                    uint8_t upper = (reg->b >> 4);
                    reg->b = lower << 4 | upper;
                    reg->cc.z = (reg->b == 0);
                }break; 
                case 0x31:      //SWAP C
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (reg->c & 0xf0);
                    uint8_t upper = (reg->c >> 4);
                    reg->c = lower << 4 | upper;
                    reg->cc.z = (reg->c == 0);
                } break;
                case 0x32:      //SWAP D
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (reg->d & 0xf0);
                    uint8_t upper = (reg->d >> 4);
                    reg->d = lower << 4 | upper;
                    reg->cc.z = (reg->d == 0);
                } break;
                case 0x33:      //SWAP E
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (reg->e & 0xf0);
                    uint8_t upper = (reg->e >> 4);
                    reg->e = lower << 4 | upper;
                    reg->cc.z = (reg->e == 0);
                } break;
                case 0x34:      //SWAP H
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (reg->h & 0xf0);
                    uint8_t upper = (reg->h >> 4);
                    reg->h = lower << 4 | upper;
                    reg->cc.z = (reg->h == 0);
                } break;
                case 0x35:      //SWAP L
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (reg->l & 0xf0);
                    uint8_t upper = (reg->l >> 4);
                    reg->l = lower << 4 | upper;
                    reg->cc.z = (reg->l == 0);
                } break;
                case 0x36:      //SWAP HL
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (hl_data & 0xf0);
                    uint8_t upper = (hl_data >> 4);
                    uint8_t x = lower << 4 | upper;
                    write_byte(mmu, reg_hl, x);
                    reg->cc.z = (x == 0);
                } break;
                case 0x37:      //SWAP A
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    reg->cc.c = 0;
                    uint8_t lower = (reg->a & 0xf0);
                    uint8_t upper = (reg->a >> 4);
                    reg->a = lower << 4 | upper;
                    reg->cc.z = (reg->l == 0);
                }break;
                case 0x38:      //SRL B
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->b & 0x80)==0x80);
                    uint8_t x = ((reg->b >> 1));
                    x = x & ~(0x01 << 7);
                    reg->b = x;
                    reg->cc.c = carry;
                    reg->cc.z = (x == 0);
                }break;
                case 0x39:      //SRL C
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->c & 0x80)==0x80);
                    uint8_t x = ((reg->c >> 1));
                    x = x & ~(0x01 << 7);
                    reg->c = x;
                    reg->cc.c = carry;
                    reg->cc.z = (x == 0);
                }break;
                case 0x3a:      //SRL D
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->d & 0x80)==0x80);
                    uint8_t x = ((reg->d >> 1));
                    x = x & ~(0x01 << 7);
                    reg->d = x;
                    reg->cc.c = carry;
                    reg->cc.z = (x == 0);
                }break;
                case 0x3b:      //SRL E
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->e & 0x80)==0x80);
                    uint8_t x = ((reg->e >> 1));
                    x = x & ~(0x01 << 7);
                    reg->e = x;
                    reg->cc.c = carry;
                    reg->cc.z = (x == 0);
                }  break; 
                case 0x3c:      //SRL H
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->h & 0x80)==0x80);
                    uint8_t x = ((reg->h >> 1));
                    x = x & ~(0x01 << 7);
                    reg->h = x;
                    reg->cc.c = carry;
                    reg->cc.z = (x == 0);
                }break;
                case 0x3d:      //SRL L
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->l & 0x80)==0x80);
                    uint8_t x = ((reg->l >> 1));
                    x = x & ~(0x01 << 7);
                    reg->l = x;
                    reg->cc.c = carry;
                    reg->cc.z = (x == 0);
                }break;
                case 0x3e:      //SRL HL
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((hl_data & 0x80)==0x80);
                    uint8_t x = ((hl_data >> 1));
                    x = x & ~(0x01 << 7);
                    write_byte(mmu, reg_hl, x);
                    reg->cc.z = (x == 0);
                    reg->cc.c = carry;
                }break;
                case 0x3f:      //SRL A
                {
                    reg->cc.n = 0;
                    reg->cc.h = 0;
                    uint8_t carry = ((reg->a & 0x80)==0x80);
                    uint8_t x = ((reg->a >> 1));
                    x = x & ~(0x01 << 7);
                    reg->a = x;
                    reg->cc.c = carry;
                    reg->cc.z = (x == 0);
                }break;
                case 0x40:      //BIT 0,B
                {
                    reg->cc.z = ((reg->b << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x41:      //BIT 0,C
                {
                    reg->cc.z = ((reg->c << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x42:      //BIT 0,D
                {
                    reg->cc.z = ((reg->d << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x43:      //BIT 0,E
                {
                    reg->cc.z = ((reg->e << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x44:      //BIT 0,H
                {
                    reg->cc.z = ((reg->h << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x45:      //BIT 0,L
                {
                    reg->cc.z = ((reg->l << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x46:      //BIT 0,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                } break;
                case 0x47:      //BIT 0,A
                {
                    reg->cc.z = ((reg->a << 7) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x48:      //BIT 1,B
                {
                    reg->cc.z = ((reg->b & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x49:      //BIT 1,C
                {
                    reg->cc.z = ((reg->c & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x4a:      //BIT 1,D
                {
                    reg->cc.z = ((reg->d & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x4b:      //BIT 1,E
                {
                    reg->cc.z = ((reg->e & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;  
                case 0x4c:      //BIT 1,H
                {
                    reg->cc.z = ((reg->h & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x4d:      //BIT 1,L
                {
                    reg->cc.z = ((reg->l & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x4e:      //BIT 1,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0x4f:      //BIT 1,A
                {
                    reg->cc.z = ((reg->a & 0x02) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x50:      //BIT 2,B
                {
                    reg->cc.z = ((reg->b & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x51:      //BIT 2,C
                {
                    reg->cc.z = ((reg->c & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break; 
                case 0x52:      //BIT 2,D
                {
                    reg->cc.z = ((reg->d & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break; 
                case 0x53:      //BIT 2,E
                {
                    reg->cc.z = ((reg->e & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break; 
                case 0x54:      //BIT 2,H
                {
                    reg->cc.z = ((reg->h & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x55:      //BIT 2,L
                {
                    reg->cc.z = ((reg->l & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x56:      //BIT 2,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                } break;
                case 0x57:      //BIT 2,A
                {
                    reg->cc.z = ((reg->a & 0x04) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x58:      //BIT 3,B
                {
                    reg->cc.z = ((reg->b & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x59:      //BIT 3,C
                {
                    reg->cc.z = ((reg->c & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x5a:      //BIT 3,D
                {
                    reg->cc.z = ((reg->d & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x5b:      //BIT 3,E
                {
                    reg->cc.z = ((reg->e & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;  
                case 0x5c:      //BIT 3,H
                {
                    reg->cc.z = ((reg->h & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x5d:      //BIT 3,L
                {
                    reg->cc.z = ((reg->l & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x5e:      //BIT 3,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0x5f:      //BIT 3,A
                {
                    reg->cc.z = ((reg->a & 0x08) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x60:      //BIT 4,B
                {
                    reg->cc.z = ((reg->b & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x61:      //BIT 4,C
                {
                    reg->cc.z = ((reg->c & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x62:      //BIT 4,D
                {
                    reg->cc.z = ((reg->d & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break; 
                case 0x63:      //BIT 4,E
                {
                    reg->cc.z = ((reg->e & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break; 
                case 0x64:      //BIT 4,H
                {
                    reg->cc.z = ((reg->h & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break; 
                case 0x65:      //BIT 4,L
                {
                    reg->cc.z = ((reg->l & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x66:      //BIT 4,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                } break;
                case 0x67:      //BIT 4,A
                {
                    reg->cc.z = ((reg->a & 0x10) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x68:      //BIT 5,B
                {
                    reg->cc.z = ((reg->b & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x69:      //BIT 5,C
                {
                    reg->cc.z = ((reg->c & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x6a:      //BIT 5,D
                {
                    reg->cc.z = ((reg->d & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x6b:      //BIT 5,E
                {
                    reg->cc.z = ((reg->e & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }  break; 
                case 0x6c:      //BIT 5,H
                {
                    reg->cc.z = ((reg->h & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x6d:      //BIT 5,L
                {
                    reg->cc.z = ((reg->l & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x6e:      //BIT 5,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0x6f:      //BIT 5,A
                {
                    reg->cc.z = ((reg->a & 0x20) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x70:      //BIT 6,B
                {
                    reg->cc.z = ((reg->b & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x71:      //BIT 6,C
                {
                    reg->cc.z = ((reg->c & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x72:      //BIT 6,D
                {
                    reg->cc.z = ((reg->d & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x73:      //BIT 6,E
                {
                    reg->cc.z = ((reg->e & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break; 
                case 0x74:      //BIT 6,H
                {
                    reg->cc.z = ((reg->h & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x75:      //BIT6,L
                {
                    reg->cc.z = ((reg->l & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;
                case 0x76:      //BIT 6,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                } break;
                case 0x77:      //BIT 6,A
                {
                    reg->cc.z = ((reg->a & 0x40) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x78:      //BIT 7,B
                {
                    reg->cc.z = ((reg->b & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x79:      //BIT 7,C
                {
                    reg->cc.z = ((reg->c & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x7a:      //BIT 7,D
                {
                    reg->cc.z = ((reg->d & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x7b:      //BIT 7,E
                {
                    reg->cc.z = ((reg->e & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                } break;  
                case 0x7c:      //BIT 7,H
                {
                    reg->cc.z = ((reg->h & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x7d:      //BIT 7,L
                {
                    reg->cc.z = ((reg->l & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x7e:      //BIT 7,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    reg->cc.z = ((hl_data & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0x7f:      //BIT 7,A
                {
                    reg->cc.z = ((reg->a & 0x80) == 0);
                    reg->cc.n = 0;
                    reg->cc.h = 1;
                }break;
                case 0x80:      //RES 0,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 0);
                    reg->b = res;
                } break;
                case 0x81:      //RES 0,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 0);
                    reg->c = res;
                } break;
                case 0x82:      //RES 0,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 0);
                    reg->d = res;
                } break;
                case 0x83:      //RES 0,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 0);
                    reg->e = res;
                } break;
                case 0x84:      //RES 0,H
                {
                    uint8_t res = reg->h;
                    res &= ~(0x01 << 0);
                    reg->h = res;
                } break;
                case 0x85:      //RES 0,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 0);
                    reg->l = res;
                } break;
                case 0x86:      //RES 0,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res &= ~(0x01 << 0);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0x87:      //RES 0,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 0);
                    reg->a = res;
                } break;
                case 0x88:      //RES 1,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 1);
                    reg->l = res;
                } break;
                case 0x89:      //RES 1,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 1);
                    reg->c = res;
                } break;
                case 0x8a:      //RES 1,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 1);
                    reg->d = res;
                } break;
                case 0x8b:      //RES 1,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 1);
                    reg->e = res;
                } break;  
                case 0x8c:      //RES 1,H
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 1);
                    reg->h = res;
                } break;
                case 0x8d:      //RES 1,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 1);
                    reg->l = res;
                } break;
                case 0x8e:      //RES 1,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data &= ~(0x01 << 1);
                    write_byte(mmu, reg_hl, hl_data);
                } break;
                case 0x8f:      //RES 1,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 1);
                    reg->a = res;
                } break;
                case 0x90:      //RES 2,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 2);
                    reg->b = res;
                } break;
                case 0x91:      //RES 2,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 2);
                    reg->c = res;
                } break;
                case 0x92:      //RES 2,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 2);
                    reg->d = res;
                } break;
                case 0x93:      //RES 2,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 2);
                    reg->e = res;
                } break;
                case 0x94:      //RES 2,H
                {
                    uint8_t res = reg->h;
                    res &= ~(0x01 << 2);
                    reg->h = res;
                } break;
                case 0x95:      //RES 2,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 2);
                    reg->l = res;
                } break;
                case 0x96:      //RES 2,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res &= ~(0x01 << 2);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0x97:      //RES 2,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 2);
                    reg->a = res;
                }break;
                case 0x98:      //RES 3,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 3);
                    reg->l = res;
                }break;
                case 0x99:      //RES 3,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 3);
                    reg->c = res;
                }break;
                case 0x9a:      //RES 3,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 3);
                    reg->d = res;
                }break;
                case 0x9b:      //RES 3,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 3);
                    reg->e = res;
                } break;  
                case 0x9c:      //RES 3,H
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 3);
                    reg->h = res;
                }break;
                case 0x9d:      //RES 3,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 3);
                    reg->l = res;
                }break;
                case 0x9e:      //RES 3,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data &= ~(0x01 << 3);
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0x9f:      //RES 3,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 3);
                    reg->a = res;
                }break;
                case 0xa0:      //RES 4,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 4);
                    reg->b = res;
                } break;
                case 0xa1:      //RES 4,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 4);
                    reg->c = res;
                } break;
                case 0xa2:      //RES 4,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 4);
                    reg->d = res;
                } break;
                case 0xa3:      //RES 4,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 4);
                    reg->e = res;
                } break;
                case 0xa4:      //RES 4,H
                {
                    uint8_t res = reg->h;
                    res &= ~(0x01 << 4);
                    reg->h = res;
                } break;
                case 0xa5:      //RES 4,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 4);
                    reg->l = res;
                } break;
                case 0xa6:      //RES 4,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res &= ~(0x01 << 4);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0xa7:      //RES 4,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 4);
                    reg->a = res;
                }break;
                case 0xa8:      //RES 5,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 5);
                    reg->l = res;
                }break;
                case 0xa9:      //RES 5,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 5);
                    reg->c = res;
                }break;
                case 0xaa:      //RES 5,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 5);
                    reg->d = res;
                }break;
                case 0xab:      //RES 5,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 5);
                    reg->e = res;
                } break;  
                case 0xac:      //RES 5,H
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 5);
                    reg->h = res;
                }break;
                case 0xad:      //RES 5,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 5);
                    reg->l = res;
                }break;
                case 0xae:      //RES 5,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data &= ~(0x01 << 5);
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0xaf:      //RES 5,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 5);
                    reg->a = res;
                }break;
                case 0xb0:      //RES 6,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 6);
                    reg->b = res;
                } break;
                case 0xb1:      //RES 6,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 6);
                    reg->c = res;
                } break;
                case 0xb2:      //RES 6,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 4);
                    reg->d = res;
                } break;
                case 0xb3:      //RES 6,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 6);
                    reg->e = res;
                } break;
                case 0xb4:      //RES 6,H
                {
                    uint8_t res = reg->h;
                    res &= ~(0x01 << 6);
                    reg->h = res;
                } break;
                case 0xb5:      //RES 6,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 6);
                    reg->l = res;
                } break;
                case 0xb6:      //RES 6,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res &= ~(0x01 << 6);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0xb7:      //RES 6,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 6);
                    reg->a = res;
                }break;
                case 0xb8:      //RES 7,B
                {
                    uint8_t res = reg->b;
                    res &= ~(0x01 << 5);
                    reg->l = res;
                }break;
                case 0xb9:      //RES 7,C
                {
                    uint8_t res = reg->c;
                    res &= ~(0x01 << 5);
                    reg->c = res;
                }break;
                case 0xba:      //RES 7,D
                {
                    uint8_t res = reg->d;
                    res &= ~(0x01 << 5);
                    reg->d = res;
                }break;
                case 0xbb:      //RES 7,E
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 7);
                    reg->e = res;
                } break;  
                case 0xbc:      //RES 7,H
                {
                    uint8_t res = reg->e;
                    res &= ~(0x01 << 7);
                    reg->h = res;
                }break;
                case 0xbd:      //RES 7,L
                {
                    uint8_t res = reg->l;
                    res &= ~(0x01 << 7);
                    reg->l = res;
                }break;
                case 0xbe:      //RES 7,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data &= ~(0x01 << 7);
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0xbf:      //RES 7,A
                {
                    uint8_t res = reg->a;
                    res &= ~(0x01 << 7);
                    reg->a = res;
                }break;
                case 0xc0:      //SET 0,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 0);
                    reg->b = res;
                } break;
                case 0xc1:      //SET 0,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 0);
                    reg->c = res;
                } break;
                case 0xc2:      //SET 0,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 0);
                    reg->d = res;
                } break;
                case 0xc3:      //SET 0,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 0);
                    reg->e = res;
                } break;
                case 0xc4:      //RES 0,H
                {
                    uint8_t res = reg->h;
                    res |= (0x01 << 0);
                    reg->h = res;
                } break;
                case 0xc5:      //SET 0,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 0);
                    reg->l = res;
                } break;
                case 0xc6:      //SET 0,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res |= (0x01 << 0);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0xc7:      //SET 0,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 0);
                    reg->a = res;
                }break;
                case 0xc8:      //SET 1,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 1);
                    reg->l = res;
                }break;
                case 0xc9:      //SET 1,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 1);
                    reg->c = res;
                }break;
                case 0xca:      //SET 1,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 1);
                    reg->d = res;
                }break;
                case 0xcb:      //SET 1,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 1);
                    reg->e = res;
                } break; 
                case 0xcc:      //SET 1,H
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 1);
                    reg->h = res;
                } break;
                case 0xcd:      //SET 1,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 1);
                    reg->l = res;
                } break;
                case 0xce:      //SET 1,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data |= (0x01 << 1);
                    write_byte(mmu, reg_hl, hl_data);
                } break;
                case 0xcf:      //SET 1,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 1);
                    reg->a = res;
                }break;
                case 0xd0:      //SET 2,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 2);
                    reg->b = res;
                } break;
                case 0xd1:      //SET 2,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 2);
                    reg->c = res;
                } break;
                case 0xd2:      //SET 2,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 2);
                    reg->d = res;
                } break;
                case 0xd3:      //SET 2,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 2);
                    reg->e = res;
                } break;
                case 0xd4:      //SET 2,H
                {
                    uint8_t res = reg->h;
                    res |= (0x01 << 2);
                    reg->h = res;
                } break;
                case 0xd5:      //SET 2,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 2);
                    reg->l = res;
                } break;
                case 0xd6:      //SET 2,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res |= (0x01 << 2);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0xd7:      //SET 2,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 2);
                    reg->a = res;
                }break;
                case 0xd8:      //SET 3,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 3);
                    reg->l = res;
                }break;
                case 0xd9:      //SET 3,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 3);
                    reg->c = res;
                }break;
                case 0xda:      //SET 3,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 3);
                    reg->d = res;
                }break;
                case 0xdb:      //SET 3,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 3);
                    reg->e = res;
                } break;  
                case 0xdc:      //SET 3,H
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 3);
                    reg->h = res;
                }break;
                case 0xdd:      //SET 3,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 3);
                    reg->l = res;
                }break;
                case 0xde:      //SET 3,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data |= (0x01 << 3);
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0xdf:      //SET 3,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 3);
                    reg->a = res;
                }break;
                case 0xe0:      //SET 4,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 4);
                    reg->b = res;
                } break;
                case 0xe1:      //SET 4,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 4);
                    reg->c = res;
                } break;
                case 0xe2:      //SET 4,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 4);
                    reg->d = res;
                } break;
                case 0xe3:      //SET 4,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 4);
                    reg->e = res;
                } break;
                case 0xe4:      //SET 4,H
                {
                    uint8_t res = reg->h;
                    res |= (0x01 << 4);
                    reg->h = res;
                } break;
                case 0xe5:      //SET 4,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 4);
                    reg->l = res;
                } break;
                case 0xe6:      //SET 4,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res |= (0x01 << 4);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0xe7:      //SET 4,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 4);
                    reg->a = res;
                } break;
                case 0xe8:      //SET 5,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 5);
                    reg->l = res;
                } break;
                case 0xe9:      //SET 5,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 5);
                    reg->c = res;
                } break;
                case 0xea:      //SET 5,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 5);
                    reg->d = res;
                }break;
                case 0xeb:      //SET 5,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 5);
                    reg->e = res;
                } break; 
                case 0xec:      //SET 5,H
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 5);
                    reg->h = res;
                } break;
                case 0xed:      //SET 5,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 5);
                    reg->l = res;
                } break;
                case 0xee:      //SET 5,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data |= (0x01 << 5);
                    write_byte(mmu, reg_hl, hl_data);
                } break;
                case 0xef:      //SET 5,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 5);
                    reg->a = res;
                } break;
                case 0xf0:      //SET 6,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 6);
                    reg->b = res;
                }  break;
                case 0xf1:      //SET 6,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 6);
                    reg->c = res;
                } break;
                case 0xf2:      //SET 6,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 6);
                    reg->d = res;
                } break;
                case 0xf3:      //SET 6,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 6);
                    reg->e = res;
                } break;
                case 0xf4:      //SET 6,H
                {
                    uint8_t res = reg->h;
                    res |= (0x01 << 6);
                    reg->h = res;
                } break;
                case 0xf5:      //SET 6,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 6);
                    reg->l = res;
                } break;
                case 0xf6:      //SET 6,(hl)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    uint8_t res = hl_data;
                    res |= (0x01 << 6);
                    write_byte(mmu, reg_hl, res);
                } break;
                case 0xf7:      //SET 6,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 6);
                    reg->a = res;
                } break;
                case 0xf8:      //SET 7,B
                {
                    uint8_t res = reg->b;
                    res |= (0x01 << 7);
                    reg->l = res;
                } break;
                case 0xf9:      //SET 7,C
                {
                    uint8_t res = reg->c;
                    res |= (0x01 << 7);
                    reg->c = res;
                }break;
                case 0xfa:      //SET 7,D
                {
                    uint8_t res = reg->d;
                    res |= (0x01 << 7);
                    reg->d = res;
                }break;
                case 0xfb:      //SET 7,E
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 7);
                    reg->e = res;
                }  break; 
                case 0xfc:      //SET 7,H
                {
                    uint8_t res = reg->e;
                    res |= (0x01 << 7);
                    reg->h = res;
                }break;
                case 0xfd:      //SET 7,L
                {
                    uint8_t res = reg->l;
                    res |= (0x01 << 7);
                    reg->l = res;
                }break;
                case 0xfe:      //SET 7,(HL)
                {
                    uint16_t reg_hl = read_hl_16bit(reg);
                    uint8_t hl_data = read_byte(mmu, reg_hl);
                    hl_data |= (0x01 << 7);
                    write_byte(mmu, reg_hl, hl_data);
                }break;
                case 0xff:      //SET 7,A
                {
                    uint8_t res = reg->a;
                    res |= (0x01 << 7);
                    reg->a = res;
                } break;
                return cb_cycles[read_byte(mmu, reg->pc)];   
                
            } break;
        case 0xcc:          //CALL Z,a16
        {
            if(reg->cc.z == 1)
            {
                reg->sp -=2;
                uint16_t addr = read_word(mmu, reg->pc);
                write_word(mmu, reg->sp, reg->pc + 2);
                reg->pc = addr;
                reg->clock.m = 24;
            }
            else
            {
                reg->pc += 2;
                reg->clock.m = 12;

            }
        }
            break;
        case 0xcd:          //CALL a16
        {
            reg->sp -=2;
            uint16_t addr = read_word(mmu, reg->pc);
            write_word(mmu, reg->sp, reg->pc + 2);
            reg->pc = addr;
            reg->clock.m = 24;
        }
            break;
        case 0xce:          //ADC A,d8
        {
            uint8_t a = reg->a;
            uint8_t val = read_byte(mmu, reg->pc) + reg->cc.c;
            reg->cc.n = 0;
            uint8_t ans = a + val;
            reg->a = ans;
            reg->cc.z = (ans == 0);
            reg->cc.c = ((a + val) > 0xff);
            reg->cc.h = ((a&0xf)+(val&0xf) > 0xf);  
            reg->pc++;
            reg->clock.m = 8;
        }
            break;
        case 0xcf:          //RST 08H
        {
            uint16_t addr = 0x0008;
            reg->sp -=2;
            write_word(mmu, reg->sp, reg->pc);
            reg->pc = addr;
            reg->clock.m = 16;
        } break;


        /********************************************************/
        
        case 0xd0:          //RET NC
        {
            if(!reg->cc.c)
            {
                uint16_t addr = read_word(mmu, reg->sp);
                reg->sp += 2;
                reg->pc = addr;
                reg->clock.m = 20;
            } else
            {
                reg->clock.m = 8;
            }
        }
            break;
        case 0xd1:          //POP DE
        {
            pop(mmu, reg, &reg->d, &reg->e);
            reg->clock.m = 12;
        }
            break;
        case 0xd2:          //JP NC,a16
        {
            if (!reg->cc.c)
            {
                uint16_t addr = read_word(mmu, reg->pc);
                reg->pc = addr - 0x02;
                reg->clock.m = 16;
            }
            else
            {
                reg->pc += 2;
                reg->clock.m = 12;
            }
        }
            break;
        case 0xd3:          //NOP
            break;
        case 0xd4:          //CALL NC,a16
        {
            if(!reg->cc.c)
            {
                reg->sp -=2;
                uint16_t addr = read_word(mmu, reg->pc);
                write_word(mmu, reg->sp, reg->pc + 2);
                reg->pc = addr;
                reg->clock.m = 24;
            }
            else
            {
                reg->clock.m = 12;
                reg->pc += 2;
            }
        }
            break;
        case 0xd5:          //PUSH DE
        {
            push(mmu, reg, reg->d, reg->e);
            reg->clock.m = 16;
        }
            break;
        case 0xd6:          //SUB,d8
        {
            reg->cc.n = 1;
            uint8_t val = read_byte(mmu, reg->pc);
            reg->cc.c = (val > reg->a);
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 8; 
            reg->pc++;     
        }
            break;
        case 0xd7:          //RST 10H
        {
            uint16_t addr = 0x0010;
            reg->sp -=2;
            write_word(mmu, reg->sp, reg->pc);
            reg->pc = addr;
            reg->clock.m = 16;
        }
            break;
        case 0xd8:          //RET C
        {
            if(!reg->cc.c)
            {
                uint16_t addr = read_word(mmu, reg->sp);
                reg->sp += 2;
                reg->pc = addr;
                reg->clock.m = 20;
            } else
            {
                reg->clock.m = 12;
            }
        }
            break;
        case 0xd9:          //RETI ****needs time*****
        {
            uint16_t addr = read_word(mmu, reg->sp);
            reg->sp += 2;
            reg->pc = addr;
            reg->clock.ime = 1;
        }
            break;
        case 0xda:          //JP C,a16
        {
           if (reg->cc.c)
            {
                uint16_t addr = read_word(mmu, reg->pc);
                reg->pc = addr - 0x02;
                reg->clock.m = 16;
            }
            else
            {
                reg->pc += 2;
                reg->clock.m = 12;
            }
        }
            break;
        case 0xdb:          //NOP
            break;
        case 0xdc:          //CALL C,a16
        {
            if(reg->cc.c)
            {
                reg->sp -=2;
                uint16_t addr = read_word(mmu, reg->pc);
                write_word(mmu, reg->sp, reg->pc + 2);
                reg->pc = addr;
                reg->clock.m = 24;
            }
            else
            {
                reg->clock.m = 12;
                reg->pc += 2;
            }
        }
            break;
        case 0xdd:          //NOP
            break;
        case 0xde:          //SBC A,d8
        {
            reg->cc.n = 1;
            uint8_t val = read_byte(mmu, reg->pc) + reg->cc.c;
            int8_t sa = (int8_t) (reg->a - val);
            uint8_t a = (uint8_t) sa;
            reg->cc.c = (sa < 0);
            reg->cc.h = ((a&0xf) > (reg->a&0xf));  
            reg->cc.z = (a == 0);
            reg->a = a; 
            reg->clock.m = 8;
            reg->pc++;
        }
            break;
        case 0xdf:          //RST 18H
        {
            uint16_t addr = 0x0018;
            reg->sp -=2;
            write_word(mmu, reg->sp, reg->pc);
            reg->pc = addr;
            reg->clock.m = 16;
        }
            break;
        
        /********************************************************/
        
        case 0xe0:          //LDH (a8),A
        {
            uint8_t byte = read_byte(mmu, reg->pc);
            uint16_t addr = 0xff00 + byte;
            write_byte(mmu, addr, reg->a);
            reg->clock.m = 12;
            reg->pc++;
        }
            break;
        case 0xe1:          //POP HL
        {
            pop(mmu, reg, &reg->h, &reg->l);
            reg->clock.m = 12;
        }
            break;
        case 0xe2:          //LD (C),A
        {
            uint16_t addr = 0xff00 + reg->c;
            write_byte(mmu, addr, reg->a);
            reg->pc++;
            reg->clock.m = 8; 
        }
            break;
        case 0xe3:          //NOP
            break;
        case 0xe4:          //NOP
            break;
        case 0xe5:          //PUSH HL
        {
            push(mmu, reg, reg->h, reg->l);
            reg->clock.m = 16;
        }
            break;
        case 0xe6:          //AND,d8
        {
            reg->cc.c = 0;
            reg->cc.h = 1;
            reg->cc.n = 0;
            reg->a &= read_byte(mmu, reg->pc);
            reg->cc.z = (reg->a==0);
            reg->clock.m = 8;
            reg->pc++;
        }
            break;
        case 0xe7:          //RST 20H
        {
            uint16_t addr = 0x0020;
            reg->sp -=2;
            write_word(mmu, reg->sp, reg->pc);
            reg->pc = addr;
            reg->clock.m = 16;
        }
            break;
        case 0xe8:          //ADD SP, R8
        {
            int8_t val = (int8_t) read_byte(mmu, reg->pc);
            reg->cc.n = 0;
            reg->cc.z = 0;
            reg->cc.c = (((reg->sp) + val) > 0xffff);
            reg->cc.h = ((reg->sp &0x0fff) + (val & 0x0fff)) > 0x0fff;
            reg->sp += val;
            reg->clock.m = 16;
            reg->pc++;
        }
            break;
        case 0xe9:          //JP (HL)
        {
            uint16_t addr = read_hl_16bit(reg);
            reg->pc = addr;
            reg->clock.m = 4;
        }
            break;
        case 0xea:          //LD (a16),A
        {          
            uint16_t addr = read_word(mmu, reg->pc);
            write_byte(mmu, addr, reg->a);
            reg->pc+=2;
            reg->clock.m = 16;
        }
            break;
        case 0xeb:          //NOP
            break;
        case 0xec:          //NOP
            break;
        case 0xed:          //NOP
            break;
        case 0xee:          //XOR
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a ^= read_byte(mmu, reg->pc);
            reg->cc.z = (reg->a==0);
            reg->clock.m = 8;
            reg->pc++;
        }
            break;
        case 0xef:          //RST 28H
        {
           uint16_t addr = 0x0028;
            reg->sp -=2;
            write_word(mmu, reg->sp, reg->pc);
            reg->pc = addr;
            reg->clock.m = 16;
        }
            break;
    /**************************************************************/

        case 0xf0:          //LDH A, (a8)
        {
            uint8_t byte = read_byte(mmu, reg->pc);
            uint16_t addr = 0xff00 + byte;
            reg->a = read_byte(mmu, addr);
            reg->clock.m = 12;
            reg->pc++;
        }
            break;
        case 0xf1:          //POP AF
        {
            pop(mmu, reg, &reg->a, (unsigned char*) &reg->cc);
            reg->clock.m = 12;
        }
            break;
        case 0xf2:          //LD A, (C)
        {
            uint16_t addr = 0xff00 + reg->c;
            reg->a = read_byte(mmu, addr);
            reg->pc++;
            reg->clock.m = 8; 
        }
            break;
        case 0xf3:          //DI
        {
            reg->clock.ime = 0;
            reg->clock.m = 4;
        } break;
        case 0xf4:          //NOP
            break;
        case 0xf5:          //PUSH AF
        {
            push(mmu, reg, reg->a, *(unsigned char*)&reg->cc);
            reg->clock.m = 16;
        }
            break;
        case 0xf6:          //OR,d8
        {
            reg->cc.c = 0;
            reg->cc.h = 0;
            reg->cc.n = 0;
            reg->a |= read_byte(mmu, reg->pc);
            reg->cc.z = (reg->a==0);
            reg->clock.m = 8;
            reg->pc++;
        }
            break;
        case 0xf7:          //RST 30H
        {
            uint16_t addr = 0x0030;
            reg->sp -=2;
            write_word(mmu, reg->sp, reg->pc);
            reg->pc = addr;
            reg->clock.m = 16;
        }
            break;
        case 0xf8:          //LD HL, SP + R8
        {
            int8_t val = (int8_t) read_byte(mmu, reg->pc);
            reg->cc.n = 0;
            reg->cc.z = 0;
            reg->cc.c = (((reg->sp) + val) > 0xffff);
            reg->cc.h = ((reg->sp &0x0fff) + (val & 0x0fff)) > 0x0fff;
            uint8_t new_hl = reg->sp + val;
            write_hl_16bit(reg,new_hl);
            reg->clock.m = 12;
            reg->pc++;
        }
            break;
        case 0xf9:          //LD SP, (HL)
        {
            reg->sp = read_hl_16bit(reg);
            reg->clock.m = 8;
        }
            break;
        case 0xfa:          //LD (a16),A
        {          
            uint16_t addr = read_word(mmu, reg->pc);
            reg->a = read_byte(mmu, addr);
            reg->clock.m = 16;
            reg->pc+= 2;
        }
            break;
        case 0xfb:          //NOP
            reg->clock.ime = 1;
            reg->clock.m = 4;
            break;
        case 0xfc:          //NOP
            break;
        case 0xfd:          //NOP
            break;
        case 0xfe:          //CP
        {
            reg->cc.n = 1;
            uint8_t val = read_byte(mmu, reg->pc);
            reg->cc.z = (reg->a==val);
            reg->cc.h = ((reg->a & 0xf) - (val & 0xf)) < 0;
            reg->cc.c = (reg->a < val);
            reg->clock.m = 8;
            reg->pc++;
        }
            break;
        case 0xff:          //RST 38H
        {
            uint16_t addr = 0x0038;
            reg->sp -=2;
            write_word(mmu, reg->sp, reg->pc);
            reg->pc = addr;
            reg->clock.m = 16;
        }
        break;
    }
    printf("\t");
	printf("%c", reg->cc.z ? 'z' : '.');
	printf("%c", reg->cc.n ? 'n' : '.');
	printf("%c", reg->cc.h ? 'h' : '.');
	printf("%c", reg->cc.c ? 'c' : '.');
	printf("%c  ", reg->cc.halt ? 'k' : '.');
	printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x PC %04x Opcode $%02x\n\n", reg->a, reg->b, reg->c,
           reg->d, reg->e, reg->h, reg->l, reg->sp, pc_print, read_byte(mmu, reg->pc));
    if (read_byte(mmu, 0xff02) == 0x81) {
            uint8_t c = mmu->addr[0xff01];
            printf("\n\n**************************this is ff01: %0x \n ",c);
            mmu->addr[0xff02] = 0x0;
        }
    return 0;
}
void handleTimer(MMU *mmu, int cycle) {
	//	set divider
	div_clocksum += cycle;
	if (div_clocksum >= 256) {
		div_clocksum -= 256;
		aluToMem(mmu, 0xff04, 1);
	}

	//	check if timer is on
	if ((read_byte(mmu, 0xff07) >> 2) & 0x1) {
		//	increase helper counter
		timer_clocksum += cycle * 4;

		//	set frequency
		int freq = 4096;					//	Hz
		if ((read_byte(mmu, 0xff07) & 3) == 1)		//	mask last 2 bits
			freq = 262144;
		else if ((read_byte(mmu, 0xff07) & 3) == 2)	//	mask last 2 bits
			freq = 65536;
		else if ((read_byte(mmu, 0xff07) & 3) == 3)	//	mask last 2 bits
			freq = 16384;

		//	increment the timer according to the frequency (synched to the processed opcodes)
		while (timer_clocksum >= (4194304 / freq)) {
			//	increase TIMA
			aluToMem(mmu, 0xff05, 1);
			//	check TIMA for overflow
			if (read_byte(mmu,0xff05) == 0x00) {
				//	set timer interrupt request
				write_byte(mmu, 0xff0f, read_byte(mmu,0xff0f) | 4);
				//	reset timer to timer modulo
				write_byte(mmu, 0xff05, read_byte(mmu, 0xff06));
			}
			timer_clocksum -= (4194304 / freq);
		}
	}
}

void handleInterrupts(Register *reg, MMU *mmu) {

	//	handle interrupts
	if (reg->int_enable) {
		//	some interrupt is enabled and allowed
		if (read_byte(mmu, 0xffff) & read_byte(mmu, 0xff0f)) {
			//	handle interrupts by priority (starting at bit 0 - vblank)
			
			//	v-blank interrupt
			if ((read_byte(mmu, 0xffff) & 1) & (read_byte(mmu, 0xff0f) & 1)) {
				reg->sp--;
				write_byte(mmu, reg->sp, reg->pc >> 8);
				reg->sp--;
				write_byte(mmu, reg->sp, reg->pc & 0xff);
				reg->pc = 0x40;
				write_byte(mmu, 0xff0f, read_byte(mmu, 0xff0f) & ~1);
			}
		}
	}
}


int main()
{
    MMU *mmu = init();
    Register *reg = (Register*) malloc(sizeof(Register));
    Cart *cart = load_rom("/Users/Matt1/Gameboy_emu/gb-test-roms-master/cpu_instrs/individual/03-op sp,hl.gb");
    load_rom_to_mmu(mmu, cart, "/Users/Matt1/Gameboy_emu/gb-test-roms-master/cpu_instrs/individual/03-op sp,hl.gb");
    mmu_load_bios(mmu);
    printf("\n\n****\n");
   
    //printf("cart: %0x", cart->rom[1]);
    init_cpu(reg, mmu);
    //mem_map(mmu,0x0000,0xffff);
/*
    while(1)
    {
         printf("\n\n****\n");
        //printf("this is data a pc: %0x and pc %0x \n ",c,reg->pc);
        emulate_instrucions(reg, mmu);
        printf("\nbyte at ff44: %0x\n", mmu->addr[0xff02]);
        if (mmu->addr[0xff02] == 0x81) 
        {
            break;
            uint8_t c = read_byte(mmu, 0xff01);
            printf("\nThis is wrong %0x\n", c);
            write_byte(mmu, 0xff02, 0x0);
        }
    }
*/
    int cyc = 0;
    while (1) {
	
		//	step cpu if not halted
		if (!reg->int_enable)
			cyc = emulate_instrucions(reg, mmu);
		//	if system is halted just idle, but still commence timers and condition for while loop
		else
			cyc = 1;

		//	handle timer
		handleTimer(mmu, cyc);

		//	handle interrupts
		handleInterrupts(reg, mmu);

	
    }
}

