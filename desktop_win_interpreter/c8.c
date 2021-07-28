#include "c8.h"

static bool initd = false;
static bool rom_loaded = false;

/* monochrome 64x32 - TODO: try to cleverly bitmap instead of storing byte for each pixel.
makes setting and reading lot easier though */
static uint8_t screenb[C8_WIDTH * C8_HEIGHT];
static bool gfx_dirty = false;

/* c8 countdown timers - */
static uint8_t delay;
static uint8_t snd;

/* TODO: inputs */
static uint8_t inputs[16];

/* TODO: check this depth is accurate */
static uint16_t stack[16];
static uint16_t sp;
/* note these are two words */
static uint16_t i; /* index reg */
static uint16_t pc; /* program counter */
/* regs V0 - V15  (VF in hex). VF is flag register*/
static uint8_t v[16];
static uint8_t* vf = &v[15];

/* main memory 0x1000*/
static uint8_t mem[4096];
/* current opcode */
//static uint16_t op;
static uint16_t rom_size = 0;

static void c8_debug(const char* fmt, ...)
{
#if defined _DEBUG
    va_list vl;
    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);
#endif
}

static void c8_display_sprite(uint8_t x, uint8_t y, uint8_t nlines)
{
    c8_debug("c8_display_sprite: x = 0x%x y = 0x%x nlines=%x\n", x, y, nlines);

    /* flag is set if anything cleared in any loop */
    *vf = 0;

    /* normal 8 x nlines sprite, data starting at I */
    /* few tricky bits - wrap across edges of overflow on x/y */
    for (uint8_t l = 0; l < nlines; ++l)
    {
        for (uint8_t b = 0; b < 8; ++b)
        {
            uint8_t source_bit = (mem[i + l] >> (7 - b)) & 1;
            if (!source_bit)
                continue;
            /* dont forget, we are indexing into array , need an index that will fit lol */
            uint32_t target = ((x + b) % C8_WIDTH) + ((y + l) % C8_HEIGHT) * C8_WIDTH;
            if (screenb[target])
            {
                screenb[target] = 0;
                *vf = 0x01;
            }
            else
            {
                screenb[target] = 0xff;
            }
        }
    }

    /* auto increment i ! dont forget this */
    i += nlines;
    gfx_dirty = true;
}

static void c8_fatal(void)
{
    /* DEBUG HOOK */
    abort();
}

bool c8_load_rom(const char* filename)
{
    rom_size = 0;
    rom_loaded = false;

    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "c8_load_rom: failed to open file '%s'!\n", filename);
        //c8_fatal();
        /* solely for static code analysis */
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t fsz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsz > 4096 - 512)
    {
        fprintf(stderr, "c8_load_rom: file is too big: %zu\n", fsz);
        c8_fatal();
    }
    else if (fsz > 0)
    {
        /* made sure its not too big, skip first sector */
        uint8_t* pmem1 = &mem[512];
        fread(pmem1, 1, fsz, f);
        c8_debug("c8_load_rom: loaded rom %s into memory (%zu bytes)\n", filename, fsz);
        rom_size = (uint16_t)fsz;
    }
    fclose(f);
    rom_loaded = true;
    return true;
}

static void c8_handle_fop(uint8_t x, uint8_t lobyte)
{
    c8_debug("c8_handle_fop: x=%x op=%x\n", x, lobyte);
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
        /* this would be kinda a pita to ferry with our sdl event pump : - ) */
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
        /* store bcd of v[x] in i, i+1, i+2 */
        mem[i] = (v[x] / 100) % 10;
        mem[i + 1] = (v[x] / 10) % 10;
        mem[i + 2] = v[x] % 10;
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
    c8_debug("c8_handle_8op: x=%x y=%x op=%x\n", x, y, eightop);

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
    {
        bool carry = v[x] + v[y] > 255;
        *vf = carry;
        v[x] = v[x] + v[y];
        break;
    }
    case 5:
    {
        bool borrow = v[x] > v[y];
        *vf = borrow;
        v[x] = v[x] - v[y];
        break;
    }
    case 6:
        /* TODO: check if flag is before or after */
        *vf = (v[x] & 1) == 1;
        v[x] = v[x] >> 1;
        break;
    case 7:
    {
        bool nborrow = v[y] > v[x];
        *vf = nborrow;
        v[x] = v[y] - v[x];
        break;
    }
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
    const uint16_t op = (mem[pc] << 8) + mem[pc + 1];

    /* we read it, increment right away. makes jumping around below easier */
    pc += 2;

    /*  hi nibble / lo nibble - dont forget its big endian */
    const uint8_t nib1 = (op & 0xf000) >> 12;
    const uint8_t lobyte = op & 0x00ff;

    const uint16_t nnn = op & 0x0fff;
    const uint8_t x = (op & 0x0f00) >> 8;
    const uint8_t y = (op & 0x00f0) >> 4;
    const uint8_t last_nib = op & 0x000f;

    c8_debug("c8_decode_op: raw=%04x - ", op);

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
                c8_debug("cls\n");
                memset(&screenb, 0, sizeof(screenb));
                gfx_dirty = true;
            }
            else if (lobyte == 0xee)
            {
                c8_debug("ret\n");
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
        c8_debug("goto 0x%03x\n", nnn);
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
            c8_debug("call 0x%03x\n", nnn);
            stack[sp] = pc; /* TODO: check this needs to be incr before? */
            pc = nnn;
        }
        break;
    }
    case 3:
    case 4: /* intentional fallthrough */
    {
        uint8_t cmp = op & 0xff;
        c8_debug("skip next if v%x %s 0x%02x\n", x, nib1 == 3 ? "==" : "!=", cmp);
        if (x >= C8_REG_MAX_IDX)
        {
            /* err */
            c8_fatal();
        }
        else if ((nib1 == 3 && v[x] == cmp) || (nib1 == 4 && v[x] != cmp))
        {
            /* skip next */
            pc += 2;
        }
        break;
    }
    case 5:
    {
        c8_debug("skip next if v%x == v%x\n", x, y);
        if (x == y)
        {
            /* skip next */
            pc += 2;
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
            c8_debug("v%x = %x\n", x, lobyte);
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
            c8_debug("v%x = v%x + %x\n", x, x, lobyte);
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

        c8_debug("skip next if v%x != v%x\n", x, y);
        if (v[x] != v[y])
        {
            pc += 4;
        }
        break;
    case 0xa:
        c8_debug("I = 0x%03x\n", nnn);
        i = nnn;
        break;
    case 0xb:
        /* jmp to nnn + v0 - check if we need to multiply v[0] */
        c8_debug("jmp to 0x%03x\n", nnn);
        pc = nnn + v[0];
        break;
    case 0xc:
    {
        /* vx = random byte & kk */
        if (x >= C8_REG_MAX_IDX)
        {
            c8_fatal();
        }

        uint8_t rv = rand() % 256;
        c8_debug("v%x = randbyte & 0x%02x\n", x, lobyte);
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
            c8_debug("skip next if key %x pressed\n", x);
            /* TODO: */
        }
        else if (lobyte == 0xa1)
        {
            /* skip next instructino if key with value of vx is not pressed */
            if (x >= C8_REG_MAX_IDX)
            {
                c8_fatal();
            }

            c8_debug("skip next if key %x not pressed\n", x);
            /* TODO: */
        }
        break;
    case 0xf:
        c8_handle_fop(x, lobyte);
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

void c8_cycle(void)
{
    c8_decode_op();
    c8_timers();

#if 0
    /* this is a safety guard to catch roms that fall off / bad */
    if (!done)
    {
        done = pc - 512 >= rom_size || pc >= 4096;
    }
#endif
}

void c8_init(void)
{
    /* reset all memory incase something was left oevr from previous rom */
    memset(screenb, 0, sizeof(screenb));

    /* TODO: load any fonts into sector */

    pc = 512; /* skip first sector - orig had chip8 vm, modern puts fonts in there */
    i = 0;
    sp = 0;
    initd = true;
}

bool c8_running(void)
{
    return initd && rom_loaded;
}

bool c8_gfx_dirty(void)
{
    return gfx_dirty;
}

/* convert our mono bitmap to display format. this sucks, probably a better way*/
static void c8_draw_points(SDL_Renderer* renderer)
{
    /* offset slightly into our buffer area for border*/
    int idx = 0;

    for (int y = 0; y < C8_HEIGHT; ++y)
    {
        for (int x = 0; x < C8_WIDTH; ++x, ++idx)
        {
            if (screenb[idx])
            {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}

void c8_draw_frame(SDL_Renderer* renderer)
{
    if (!gfx_dirty)
        return;
    c8_draw_points(renderer);
    gfx_dirty = false;
}
