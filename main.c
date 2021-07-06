#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define TEST_ROM_FILE ("roms/maze.ch8")

#define C8_REG_MAX_IDX (15)

static bool done = false;

/* regs V0 - V15  (VF in hex). VF is flag register*/
static uint8_t v[16];
static uint8_t* vf = &v[15];

/* main memory 0x1000*/
static uint8_t mem[4096];

/* monochrome 64x32 - we can just bitmap, this would be 2048 chars otherwise. just use 256 bytes and bitmap it */
static uint8_t screenb[256];

/* countdown timers - */
static uint8_t delay;
static uint8_t snd;

/* TODO: inputs */
uint8_t inputs[16];

/* TODO: check this depth is accurate */
static uint16_t stack[16];
static uint16_t sp;
/* note these are two words */
static uint16_t i; /* index reg */
static uint16_t pc; /* program counter */

/* current opcode */
static uint16_t op;

static uint16_t rom_size = 0;

static void c8_display_sprite(uint8_t x, uint8_t y, uint8_t nbytes)
{
    printf("c8_display_sprite: x = %x y = %x nbytes=%x\n", x, y, nbytes);
    /* start at addr i - sprites are xor'd onto existing screen. 
    if any pixels go from 1 to 0, VF = 1, otherwise 0. if written outside, it wraps around to opposite side */
    /* TODO - */
    *vf = 0;
}

static void c8_fatal(void)
{
    /* DEBUG HOOK */
    abort();
}

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

static void c8_handle_fop(uint8_t x, uint8_t lobyte)
{
    printf("c8_handle_fop: x=%x op=%x\n", x, lobyte);
    if (x >= C8_REG_MAX_IDX)
    {
        c8_fatal();
    }

    switch (lobyte)
    {
    case 0x07:
        v[x] = delay;
        break;
    case 0x0a:
        /* TODO: wait for keypress*/
        v[x] = 0x00;
        break;
    case 0x15:
        delay = v[x];
        break;
    case 0x18:
        snd = v[x];
        break;
    case 0x1e:
        i += v[x];
        break;
    case 0x29:
        /* TODO: I = location of sprite for digit v[x] ?? font  */
        break;
    case 0x33:
        /* TODO: store bcd of v[x] in i, i+1, i+2 */
        break;
    case 0x55:
        /* store V0 .. Vx into memory starting at i */
        for (uint8_t c = 0; c <= x; ++c)
        {
            mem[i + c] = v[c];
        }
        break;
    case 0x65:
        /* load V0 .. Vx from memory starting at i */
        for (uint8_t c = 0; c <= x; ++c)
        {
            v[c] = mem[i + c];
        }
        break;
    }
}

static void c8_handle_8op(uint8_t x, uint8_t y, uint8_t eightop)
{
    printf("c8_handle_8op: x=%x y=%x op=%x\n", x, y, eightop);

    /* 0x8xyN has lots of ops - handle here */
    switch (eightop)
    {
    case 0:
        v[x] = v[y];
        break;
    case 1:
        v[x] = v[x] | v[y];
        break;
    case 2:
        v[x] = v[x] & v[y];
        break;
    case 3:
        v[x] = v[x] ^ v[y];
        break;
    case 4:
        bool carry = v[x] + v[y] > 255;
        *vf = carry;
        v[x] = v[x] + v[y];
        break;
    case 5:
        bool borrow = v[x] > v[y];
        *vf = borrow;
        v[x] = v[x] - v[y];
        break;
    case 6:
        /* TODO: check if flag is before or after */
        *vf = (v[x] & 1) == 1;
        v[x] = v[x] >> 1;
        break;
    case 7:
        bool nborrow = v[y] > v[x];
        *vf = nborrow;
        v[x] = v[y] - v[x];
        break;
    case 0xe:
        /* TODO: check if flag is before or after */
        *vf = (v[x] & 128) != 0;
        v[x] = v[x] << 1;
        break;
    }
}

/* take a look at whatever is current in op and act upon it. updates pc */
static void c8_decode_op(void)
{
    /*  hi nibble / lo nibble - dont forget its big endian */
    uint8_t nib1 = (op & 0xf000) >> 12;
    uint8_t lobyte = op & 0x00ff;

	uint16_t nnn = op & 0x0fff;
	uint8_t x = (op & 0x0f00) >> 8;
	uint8_t y = (op & 0x00f0) >> 4;
    uint8_t last_nib = op & 0x000f;
    uint16_t pc_start = pc;

	printf("c8_decode_op: ");

    /* most opcodes can be completely keyed off first nibble */
    switch (nib1)
    {
    case 0:
        /* 0NNN - call */
        /* 00E0 - display clear */
        /* 00EE - return from sub */
        if (nib1 == 0)
        {
            if (lobyte == 0xe0)
            {
				printf("cls\n");
                memset(&screenb, 0, sizeof(screenb));
            }
            else if (lobyte == 0xee)
            {
                printf("ret\n");
                /* ret - pop stack */
                pc = stack[sp];
                --sp;
            }
            /* 0nnn - SYS not implemented */
        }
        break;
    case 1:
    {
        /* goto 0xNNN */
        pc = nnn;
		printf("goto 0x%03x\n", nnn);
        break;
    }
    case 2:
    {
        /* call subroutine at 0xNNN */
        ++sp;
        if (sp > 15)
        {
            /* stack too big */
            c8_fatal();
        }
        else
        {
            printf("call 0x%03x\n", nnn);
            stack[sp] = pc; /* TODO: check this needs to be incr before? */
            pc = nnn;
        }
        break;
    }
    case 3:
    case 4: /* intentional fallthrough */
    {
        uint8_t cmp = op & 0xff;
        printf("skip next if v%x %s 0x%02x\n", x, nib1 == 3 ? "==" : "!=", cmp);
        if (x >= C8_REG_MAX_IDX)
        {
            /* err */
            c8_fatal();
        }
        else if ((nib1 == 3 && v[x] == cmp) || (nib1 == 4 && v[x] != cmp))
        {
		    pc += 4;
        }
        break;
    }
    case 5:
    {
        printf("skip next if v%x == v%x\n", x, y);
		if (x == y)
		{
			pc += 4;
		}
		break;
    }
    case 6:
    {
        if (x >= C8_REG_MAX_IDX)
        {
            c8_fatal();
        }
        else
        {
            printf("v%x = %x\n", x, lobyte);
            v[x] = lobyte;
        }
        break;
    }
    case 7:
    {
        if (x >= C8_REG_MAX_IDX)
        {
            c8_fatal();
        }
        else
        {
            printf("v%x = v%x + %x\n", x, x, lobyte);
            v[x] = v[x] + lobyte;
        }
        break;
    }
    case 8:
        c8_handle_8op(x, y, last_nib);
        break;
    case 9: 
        if (x >= C8_REG_MAX_IDX || y >= C8_REG_MAX_IDX)
        {
            c8_fatal();
        }
        printf("skip next if v%x != v%x\n", x, y);
        if (v[x] != v[y])
        {
            pc += 4;
        }
        break;
    case 0xa:
        printf("I = 0x%03x\n", nnn);
        i = nnn;
        break;
    case 0xb:
        /* jmp to nnn + v0 - check if we need to multiply v[0] */
        printf("jmp to 0x%03x\n", nnn);
        pc = nnn + v[0];
        break;
    case 0xc:
    {
        /* vx = random byte & kk */
        if (x >= C8_REG_MAX_IDX)
        {
            c8_fatal();
        }
        int rv = rand() % 256;
        printf("v%x = randbyte & 0x%02x\n", x, lobyte);
        v[x] = rv & lobyte;
        break;
    }
    case 0xd:
        if (x >= C8_REG_MAX_IDX || y >= C8_REG_MAX_IDX)
        {
            c8_fatal();
        }
        c8_display_sprite(v[x], v[y], last_nib);
        break;
    case 0xe:
        if (lobyte == 0x9e)
        {
            /* skip next if key w/ value of vx pressed */
            if (x >= C8_REG_MAX_IDX)
            {
                c8_fatal();
            }
            printf("skip next if key %x pressed\n", x);
            /* TODO: */
        }
        else if (lobyte == 0xa1)
        {
            /* skip next instructino if key with value of vx is not pressed */
            if (x >= C8_REG_MAX_IDX)
            {
                c8_fatal();
            }

            printf("skip next if key %x not pressed\n", x);
            /* TODO: */
        }
        break;
    case 0xf:
        c8_handle_fop(x, lobyte);
        break;
    }

    /* normal exec: if pc is same, update by one inst */
    if (pc == pc_start)
    {
        pc += 2;
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

#if 0
    /* this wouldnt do any branching logic correctly but its useful to print out non data parts of rom */
    /* HACK: run through rom ops linearly debugging*/
    pc += 2;
#endif
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
    (void*)argc;
    (void*)argv;
    c8_init();
    c8_load_rom(TEST_ROM_FILE);

    while (!done)
    {
        c8_cycle();
    }
    return 0;
}