#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CHIP8_MEMORY_SIZE 4096U
#define CHIP8_DISPLAY_WIDTH 64U
#define CHIP8_DISPLAY_HEIGHT 32U
#define CHIP8_DISPLAY_SIZE (CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT)
#define CHIP8_STACK_DEPTH 16U
#define CHIP8_REGISTER_COUNT 16U
#define CHIP8_PROGRAM_START 0x200U
#define CHIP8_FONT_BASE 0x50U

typedef enum {
    CHIP8_OK = 0,
    CHIP8_ERR_INVALID_ARG,
    CHIP8_ERR_FILE,
    CHIP8_ERR_ROM_TOO_LARGE,
    CHIP8_ERR_PC_OOB,
    CHIP8_ERR_STACK_OVERFLOW,
    CHIP8_ERR_STACK_UNDERFLOW,
    CHIP8_ERR_MEMORY_OOB,
    CHIP8_ERR_BAD_OPCODE
} chip8_status_t;

typedef struct {
    uint8_t memory[CHIP8_MEMORY_SIZE];
    uint8_t V[CHIP8_REGISTER_COUNT];
    uint16_t I;
    uint16_t pc;
    uint16_t stack[CHIP8_STACK_DEPTH];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t keypad[CHIP8_REGISTER_COUNT];
    uint8_t display[CHIP8_DISPLAY_SIZE];
    bool draw_flag;
    bool waiting_for_key;
    uint8_t wait_reg;
} chip8_t;

void chip8_init(chip8_t *chip8);
chip8_status_t chip8_load_rom(chip8_t *chip8, const char *rom_path);
chip8_status_t chip8_cycle(chip8_t *chip8);
void chip8_tick_timers(chip8_t *chip8);
void chip8_set_key(chip8_t *chip8, uint8_t key, bool pressed);
const uint8_t *chip8_get_display(const chip8_t *chip8);
bool chip8_consume_draw_flag(chip8_t *chip8);
const char *chip8_status_str(chip8_status_t status);

#endif
