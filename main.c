#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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

/* reads current program counter (pc) position into op - does NOT update pc */
static void c8_read_op(void)
{
    op = (mem[pc] << 8) + mem[pc + 1];
}

/* take a look at whatever is current in op and act upon it. updates pc */
static void c8_decode_op(void)
{
    /* 'god' switch here for now */
}

static void c8_timers(void)
{
    if (snd)
    {
        if (snd == 1)
        {
            /* buzzer here */
        }
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
        /* dont have to worry about height, we naturally run to end of bmp */
    }
#endif
}
int main(int argc, char** argv)
{
    memset(screenb, 0xAA, 256);
    //screenb[3] = 0xFF;
    c8_display();
    return 0;
}