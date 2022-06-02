#include"mmu.h"
#include "rom.h"
#include <string.h>

void mem_map(MMU *mmu, uint16_t start, uint16_t end)
{
    char s[] = "Offset      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\r\n\r\n";
    for (int i = start; i < end; i+= 0x10) {
		char title[70];
		printf("0x%04x      %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \r\n", i,
			read_byte(mmu, i),
			read_byte(mmu, i + 1),
			read_byte(mmu, i + 2),
			read_byte(mmu, i + 3),
			read_byte(mmu, i + 4),
			read_byte(mmu, i + 5),
			read_byte(mmu, i + 6),
			read_byte(mmu, i + 7),
			read_byte(mmu, i + 8),
			read_byte(mmu, i + 9),
			read_byte(mmu, i + 10),
			read_byte(mmu, i + 11),
			read_byte(mmu, i + 12),
			read_byte(mmu, i + 13),
			read_byte(mmu, i + 14),
			read_byte(mmu, i + 15)
		);
    }
}