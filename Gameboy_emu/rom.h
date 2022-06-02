#ifndef _ROM_H
#define _ROM_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct _cart
{
    uint8_t *title;
    uint8_t *rom;
    uint32_t size;
}Cart;

Cart *load_rom(char *file_name);
void free_rom(Cart *rom);
#endif