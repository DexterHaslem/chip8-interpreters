#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include <SDL.h>

#define C8_WIDTH (64)
#define C8_HEIGHT (32)

//#define TEST_ROM_FILE ("../roms/maze.ch8")

#define C8_REG_MAX_IDX          (15)
#define C8_PIXEL_SCALE          (8)
#define C8_CYCLES_PER_FRAME     (15)
#define C8_FRAME_DELAY_MS       (16)

bool c8_load_rom(const char* filename);
void c8_cycle(void);
void c8_init(void);
void c8_draw_frame(SDL_Renderer* renderer);
bool c8_running(void);
bool c8_gfx_dirty(void);
