#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const uint8_t k_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

static void chip8_seed_rng_once(void) {
    static bool seeded = false;
    if (!seeded) {
        seeded = true;
        srand((unsigned)time(NULL));
    }
}

void chip8_init(chip8_t *chip8) {
    if (chip8 == NULL) {
        return;
    }

    memset(chip8, 0, sizeof(*chip8));
    chip8->pc = CHIP8_PROGRAM_START;

    if ((CHIP8_FONT_BASE + sizeof(k_fontset)) <= CHIP8_MEMORY_SIZE) {
        memcpy(&chip8->memory[CHIP8_FONT_BASE], k_fontset, sizeof(k_fontset));
    }

    chip8_seed_rng_once();
}

chip8_status_t chip8_load_rom(chip8_t *chip8, const char *rom_path) {
    FILE *fp;
    long rom_size;
    size_t read_count;

    if ((chip8 == NULL) || (rom_path == NULL)) {
        return CHIP8_ERR_INVALID_ARG;
    }

    fp = fopen(rom_path, "rb");
    if (fp == NULL) {
        return CHIP8_ERR_FILE;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return CHIP8_ERR_FILE;
    }

    rom_size = ftell(fp);
    if (rom_size < 0) {
        fclose(fp);
        return CHIP8_ERR_FILE;
    }

    if ((size_t)rom_size > (CHIP8_MEMORY_SIZE - CHIP8_PROGRAM_START)) {
        fclose(fp);
        return CHIP8_ERR_ROM_TOO_LARGE;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        return CHIP8_ERR_FILE;
    }

    read_count = fread(&chip8->memory[CHIP8_PROGRAM_START], 1U, (size_t)rom_size, fp);
    fclose(fp);

    if (read_count != (size_t)rom_size) {
        return CHIP8_ERR_FILE;
    }

    return CHIP8_OK;
}

static chip8_status_t ensure_memory_index(uint16_t index) {
    return (index < CHIP8_MEMORY_SIZE) ? CHIP8_OK : CHIP8_ERR_MEMORY_OOB;
}

chip8_status_t chip8_cycle(chip8_t *chip8) {
    uint16_t opcode;
    uint8_t x;
    uint8_t y;
    uint8_t kk;
    uint16_t nnn;
    uint8_t n;

    if (chip8 == NULL) {
        return CHIP8_ERR_INVALID_ARG;
    }

    if (chip8->waiting_for_key) {
        return CHIP8_OK;
    }

    if ((chip8->pc + 1U) >= CHIP8_MEMORY_SIZE) {
        return CHIP8_ERR_PC_OOB;
    }

    opcode = (uint16_t)((uint16_t)chip8->memory[chip8->pc] << 8U) |
             (uint16_t)chip8->memory[chip8->pc + 1U];
    chip8->pc = (uint16_t)(chip8->pc + 2U);

    x = (uint8_t)((opcode & 0x0F00U) >> 8U);
    y = (uint8_t)((opcode & 0x00F0U) >> 4U);
    kk = (uint8_t)(opcode & 0x00FFU);
    nnn = (uint16_t)(opcode & 0x0FFFU);
    n = (uint8_t)(opcode & 0x000FU);

    switch (opcode & 0xF000U) {
        case 0x0000U:
            if (opcode == 0x00E0U) {
                memset(chip8->display, 0, sizeof(chip8->display));
                chip8->draw_flag = true;
            } else if (opcode == 0x00EEU) {
                if (chip8->sp == 0U) {
                    return CHIP8_ERR_STACK_UNDERFLOW;
                }
                chip8->sp--;
                chip8->pc = chip8->stack[chip8->sp];
            } else {
                return CHIP8_ERR_BAD_OPCODE;
            }
            break;

        case 0x1000U:
            if (nnn >= CHIP8_MEMORY_SIZE) {
                return CHIP8_ERR_PC_OOB;
            }
            chip8->pc = nnn;
            break;

        case 0x2000U:
            if (chip8->sp >= CHIP8_STACK_DEPTH) {
                return CHIP8_ERR_STACK_OVERFLOW;
            }
            if (nnn >= CHIP8_MEMORY_SIZE) {
                return CHIP8_ERR_PC_OOB;
            }
            chip8->stack[chip8->sp] = chip8->pc;
            chip8->sp++;
            chip8->pc = nnn;
            break;

        case 0x3000U:
            if (chip8->V[x] == kk) {
                chip8->pc = (uint16_t)(chip8->pc + 2U);
            }
            break;

        case 0x4000U:
            if (chip8->V[x] != kk) {
                chip8->pc = (uint16_t)(chip8->pc + 2U);
            }
            break;

        case 0x5000U:
            if ((opcode & 0x000FU) != 0U) {
                return CHIP8_ERR_BAD_OPCODE;
            }
            if (chip8->V[x] == chip8->V[y]) {
                chip8->pc = (uint16_t)(chip8->pc + 2U);
            }
            break;

        case 0x6000U:
            chip8->V[x] = kk;
            break;

        case 0x7000U:
            chip8->V[x] = (uint8_t)(chip8->V[x] + kk);
            break;

        case 0x8000U:
            switch (opcode & 0x000FU) {
                case 0x0U:
                    chip8->V[x] = chip8->V[y];
                    break;
                case 0x1U:
                    chip8->V[x] = (uint8_t)(chip8->V[x] | chip8->V[y]);
                    break;
                case 0x2U:
                    chip8->V[x] = (uint8_t)(chip8->V[x] & chip8->V[y]);
                    break;
                case 0x3U:
                    chip8->V[x] = (uint8_t)(chip8->V[x] ^ chip8->V[y]);
                    break;
                case 0x4U: {
                    uint16_t sum = (uint16_t)chip8->V[x] + (uint16_t)chip8->V[y];
                    chip8->V[0xFU] = (sum > 0xFFU) ? 1U : 0U;
                    chip8->V[x] = (uint8_t)(sum & 0xFFU);
                    break;
                }
                case 0x5U:
                    chip8->V[0xFU] = (chip8->V[x] >= chip8->V[y]) ? 1U : 0U;
                    chip8->V[x] = (uint8_t)(chip8->V[x] - chip8->V[y]);
                    break;
                case 0x6U:
                    chip8->V[0xFU] = (uint8_t)(chip8->V[x] & 0x01U);
                    chip8->V[x] = (uint8_t)(chip8->V[x] >> 1U);
                    break;
                case 0x7U:
                    chip8->V[0xFU] = (chip8->V[y] >= chip8->V[x]) ? 1U : 0U;
                    chip8->V[x] = (uint8_t)(chip8->V[y] - chip8->V[x]);
                    break;
                case 0xEU:
                    chip8->V[0xFU] = (uint8_t)((chip8->V[x] & 0x80U) >> 7U);
                    chip8->V[x] = (uint8_t)(chip8->V[x] << 1U);
                    break;
                default:
                    return CHIP8_ERR_BAD_OPCODE;
            }
            break;

        case 0x9000U:
            if ((opcode & 0x000FU) != 0U) {
                return CHIP8_ERR_BAD_OPCODE;
            }
            if (chip8->V[x] != chip8->V[y]) {
                chip8->pc = (uint16_t)(chip8->pc + 2U);
            }
            break;

        case 0xA000U:
            if (nnn >= CHIP8_MEMORY_SIZE) {
                return CHIP8_ERR_MEMORY_OOB;
            }
            chip8->I = nnn;
            break;

        case 0xB000U: {
            uint16_t target = (uint16_t)(nnn + chip8->V[0]);
            if (target >= CHIP8_MEMORY_SIZE) {
                return CHIP8_ERR_PC_OOB;
            }
            chip8->pc = target;
            break;
        }

        case 0xC000U:
            chip8->V[x] = (uint8_t)(((uint8_t)(((unsigned int)rand()) & 0xFFU)) & kk);
            break;

        case 0xD000U: {
            uint8_t vx = chip8->V[x];
            uint8_t vy = chip8->V[y];
            uint8_t row;

            chip8->V[0xFU] = 0U;
            for (row = 0U; row < n; row++) {
                uint8_t sprite_byte;
                uint16_t addr = (uint16_t)(chip8->I + row);
                uint8_t bit;

                if (ensure_memory_index(addr) != CHIP8_OK) {
                    return CHIP8_ERR_MEMORY_OOB;
                }

                sprite_byte = chip8->memory[addr];
                for (bit = 0U; bit < 8U; bit++) {
                    uint8_t pixel;
                    size_t idx;
                    uint8_t x_pos = (uint8_t)((vx + bit) % CHIP8_DISPLAY_WIDTH);
                    uint8_t y_pos = (uint8_t)((vy + row) % CHIP8_DISPLAY_HEIGHT);

                    pixel = (uint8_t)((sprite_byte >> (7U - bit)) & 0x01U);
                    if (pixel == 0U) {
                        continue;
                    }

                    idx = (size_t)y_pos * CHIP8_DISPLAY_WIDTH + x_pos;
                    if (chip8->display[idx] == 1U) {
                        chip8->V[0xFU] = 1U;
                    }
                    chip8->display[idx] ^= 1U;
                }
            }

            chip8->draw_flag = true;
            break;
        }

        case 0xE000U:
            if (kk == 0x9EU) {
                if (chip8->keypad[chip8->V[x] & 0x0FU] != 0U) {
                    chip8->pc = (uint16_t)(chip8->pc + 2U);
                }
            } else if (kk == 0xA1U) {
                if (chip8->keypad[chip8->V[x] & 0x0FU] == 0U) {
                    chip8->pc = (uint16_t)(chip8->pc + 2U);
                }
            } else {
                return CHIP8_ERR_BAD_OPCODE;
            }
            break;

        case 0xF000U:
            switch (kk) {
                case 0x07U:
                    chip8->V[x] = chip8->delay_timer;
                    break;
                case 0x0AU:
                    chip8->waiting_for_key = true;
                    chip8->wait_reg = x;
                    break;
                case 0x15U:
                    chip8->delay_timer = chip8->V[x];
                    break;
                case 0x18U:
                    chip8->sound_timer = chip8->V[x];
                    break;
                case 0x1EU: {
                    uint16_t sum = (uint16_t)(chip8->I + chip8->V[x]);
                    chip8->V[0xFU] = (sum > 0x0FFFU) ? 1U : 0U;
                    chip8->I = (uint16_t)(sum & 0x0FFFU);
                    break;
                }
                case 0x29U:
                    chip8->I = (uint16_t)(CHIP8_FONT_BASE + ((chip8->V[x] & 0x0FU) * 5U));
                    break;
                case 0x33U:
                    if ((chip8->I + 2U) >= CHIP8_MEMORY_SIZE) {
                        return CHIP8_ERR_MEMORY_OOB;
                    }
                    chip8->memory[chip8->I] = (uint8_t)(chip8->V[x] / 100U);
                    chip8->memory[chip8->I + 1U] = (uint8_t)((chip8->V[x] / 10U) % 10U);
                    chip8->memory[chip8->I + 2U] = (uint8_t)(chip8->V[x] % 10U);
                    break;
                case 0x55U: {
                    uint8_t i;
                    if ((chip8->I + x) >= CHIP8_MEMORY_SIZE) {
                        return CHIP8_ERR_MEMORY_OOB;
                    }
                    for (i = 0U; i <= x; i++) {
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    }
                    break;
                }
                case 0x65U: {
                    uint8_t i;
                    if ((chip8->I + x) >= CHIP8_MEMORY_SIZE) {
                        return CHIP8_ERR_MEMORY_OOB;
                    }
                    for (i = 0U; i <= x; i++) {
                        chip8->V[i] = chip8->memory[chip8->I + i];
                    }
                    break;
                }
                default:
                    return CHIP8_ERR_BAD_OPCODE;
            }
            break;

        default:
            return CHIP8_ERR_BAD_OPCODE;
    }

    return CHIP8_OK;
}

void chip8_tick_timers(chip8_t *chip8) {
    if (chip8 == NULL) {
        return;
    }

    if (chip8->delay_timer > 0U) {
        chip8->delay_timer--;
    }
    if (chip8->sound_timer > 0U) {
        chip8->sound_timer--;
    }
}

void chip8_set_key(chip8_t *chip8, uint8_t key, bool pressed) {
    uint8_t key_index;

    if (chip8 == NULL) {
        return;
    }

    key_index = (uint8_t)(key & 0x0FU);
    chip8->keypad[key_index] = pressed ? 1U : 0U;

    if (pressed && chip8->waiting_for_key) {
        chip8->V[chip8->wait_reg] = key_index;
        chip8->waiting_for_key = false;
    }
}

const uint8_t *chip8_get_display(const chip8_t *chip8) {
    if (chip8 == NULL) {
        return NULL;
    }
    return chip8->display;
}

bool chip8_consume_draw_flag(chip8_t *chip8) {
    bool should_draw;

    if (chip8 == NULL) {
        return false;
    }

    should_draw = chip8->draw_flag;
    chip8->draw_flag = false;
    return should_draw;
}

const char *chip8_status_str(chip8_status_t status) {
    switch (status) {
        case CHIP8_OK:
            return "OK";
        case CHIP8_ERR_INVALID_ARG:
            return "invalid argument";
        case CHIP8_ERR_FILE:
            return "file error";
        case CHIP8_ERR_ROM_TOO_LARGE:
            return "ROM too large";
        case CHIP8_ERR_PC_OOB:
            return "program counter out of bounds";
        case CHIP8_ERR_STACK_OVERFLOW:
            return "stack overflow";
        case CHIP8_ERR_STACK_UNDERFLOW:
            return "stack underflow";
        case CHIP8_ERR_MEMORY_OOB:
            return "memory access out of bounds";
        case CHIP8_ERR_BAD_OPCODE:
            return "invalid/unsupported opcode";
        default:
            return "unknown error";
    }
}
