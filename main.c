#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define TEST_ROM_FILE ("roms/maze.ch8")

static bool done = false;

/* regs V0 - V15 */
static uint8_t v[016];
/* main memory 0x1000*/
static uint8_t mem[4096];

/* monochrome 64x32 - we can just bitmap, this would be 2048 chars otherwise. just use 256 bytes and bitmap it */
static uint8_t screenb[256];

/* countdown timers - */
static uint8_t delay;
static uint8_t snd;

/* TODO: inputs */
uint8_t inputs[16];

/* TODO: cehck this depth is accurate */
static uint16_t stack[16];
static uint16_t sp;
/* note these are two words */
static uint16_t i; /* index reg */
static uint16_t pc; /* program counter */

/* current opcode */
static uint16_t op;

static uint16_t rom_size = 0;

static void c8_load_rom(const char* filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "c8_load_rom: failed to open file '%s'!\n", filename);
        return;
    }
    
    fseek(f, 0, SEEK_END);
    size_t fsz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsz > 4096 - 512)
    {
        fprintf(stderr, "c8_load_rom: file is too big: %zu\n", fsz);
    }
    else if (fsz > 0)
    {
        /* made sure its not too big, skip first sector */
        uint8_t* pmem1 = &mem[512];
        fread(pmem1, 1, fsz, f);
        printf("c8_load_rom: loaded rom %s into memory (%zu bytes)\n", filename, fsz);
        rom_size = (uint16_t)fsz;
    }
    fclose(f);
}

/* reads current program counter (pc) position into op - does NOT update pc */
static void c8_read_op(void)
{
    op = (mem[pc] << 8) + mem[pc + 1];
    printf("c8_read_op: pc=%u op=%04x\n", pc, op);
}

/* take a look at whatever is current in op and act upon it. updates pc */
static void c8_decode_op(void)
{
    /* hi byte hi nibble / lo nibble */
    uint8_t hinh = (op & 0xf000) >> 12;
    uint8_t hinl = (op & 0x0f00) >> 8;

    uint8_t hi = (op & 0xff00) >> 8;
    uint8_t lo = op & 0x00ff;

    uint8_t lonh = lo & 0xf0;
    uint8_t lonl = lo & 0x0f;

    switch (hinh) /* get us most the way with the highest nibble */
    {
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        break;
    case 8:
        break;
    case 9: 
        break;
    case 0xa:
        break;
    case 0xb:
        break;
    case 0xc:
        break;
    case 0xd:
        break;
    case 0xe:
        break;
    case 0xf:
        break;
    }
    
}

static void c8_timers(void)
{
    if (snd)
    {
        /* buzzer here */
        --snd;
    }

    if (delay)
    {
        --delay;
    }
}

static void c8_cycle(void)
{
    c8_read_op();
    c8_decode_op();

    c8_timers();
    /* HACK: run through rom ops linearly debugging*/
    pc += 2;
    done = pc - 512 >= rom_size || pc >= 4096;
}

static void c8_init(void)
{
    pc = 512; /* skip first sector - orig had chip8 vm, modern puts fonts in there */
    i = 0;
    sp = 0;
    op = 0;
}

static void c8_display(void)
{
    /* our memory for screen is represented as 256 bytes in a bitmap.
    we need to represent 64x32 here a char for each position */

#if 1 /* DEBUG CONSOLE DUMP */
    /* gross : - ( */
    system("cls");

    /* this is not (word)/byte because we dont want to overflow on our size oops */
    for (uint32_t by = 1; by <= 256; ++by)
    {
        /* for each byte, go through each bit manually on it, there is probably a more clever way but for now
        just do things in the most obvious verbose way */
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            uint8_t bitv = screenb[by - 1] & 1 << bit;
            printf("%c", bitv ? '*' : ' ');
        }

        /* we just handled 8 pixels. every 8x8 = 64 = line*/
        if (by % 8 == 0)
        {
            printf("\n");
        }
        /* /dont have to worry about height, we naturally run to end of bmp */
    }
#endif/
}
int main(int argc, char** argv)
{
    c8_init();
    c8_load_rom(TEST_ROM_FILE);

    while (!done)
    {
        c8_cycle();
    }
    return 0;
}