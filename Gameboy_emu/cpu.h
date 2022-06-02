#ifndef _CPU_H_
#define _CPU_H_

#include <stdlib.h>
#include "mmu.h"

typedef struct Flag
{
    uint8_t z:1;
    uint8_t n:1;
    uint8_t h:1;
    uint8_t c:1;
    uint8_t halt:1;
}Flag;

typedef struct Clock
{
    char m;
    uint8_t ime:1;
}Clock;

typedef struct Register
{
    //8-bit registers
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint8_t f;
    //Operation Flags
    struct Flag cc;
    //16-bit registers
    uint16_t hl;
    uint16_t sp;
    uint16_t pc;
    uint8_t	int_enable;
    //Clock
    struct Clock clock;
}Register;

void init_cpu(Register *reg, MMU *mmu);
int emulate_instrucions(Register *reg, MMU *mmu);
void handleTimer(MMU *mmu, int cycle);
void handleInterrupts(Register *reg, MMU *mmu);

#endif