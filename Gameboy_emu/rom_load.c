#include <string.h>
#include "rom.h"

Cart *load_rom(char *file_name)
{
    Cart *cart = NULL;
    FILE *file = NULL;

    file = fopen(file_name, "rb");

    if (file)
    {
        fseek(file, 0, SEEK_END);
        uint32_t total = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (total > 0)
        {
            cart = (Cart*)malloc(sizeof(Cart));
            cart->rom = (uint8_t*)malloc(sizeof(uint8_t) * total);
            cart->size = total;
            if (fread(cart->rom, 1, total, file) != total)
            {
                free(cart->rom);
                free(cart);
                fclose(file);
            }
        }
        fclose(file);
    }
    return cart ;
}