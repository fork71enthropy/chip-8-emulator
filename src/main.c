#include "chip8.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_CYCLES_PER_FRAME 10U
#define DEFAULT_MAX_CYCLES 1000000ULL

static void print_usage(const char *prog_name) {
    if (prog_name == NULL) {
        prog_name = "chip8";
    }

    printf("Usage: %s <rom_path> [--cycles-per-frame N] [--max-cycles N] [--ascii]\n", prog_name);
    printf("\n");
    printf("Options:\n");
    printf("  --cycles-per-frame N   Number of CPU cycles between display refreshes (default: %u).\n", DEFAULT_CYCLES_PER_FRAME);
    printf("  --max-cycles N         Stop after N cycles (default: %" PRIu64 ").\n", (uint64_t)DEFAULT_MAX_CYCLES);
    printf("  --ascii                Print display frames to stdout when draw occurs.\n");
}

static bool parse_u32(const char *text, uint32_t *out_value) {
    char *end_ptr;
    unsigned long parsed;

    if ((text == NULL) || (out_value == NULL) || (*text == '\0')) {
        return false;
    }

    errno = 0;
    parsed = strtoul(text, &end_ptr, 10);
    if ((errno != 0) || (end_ptr == text) || (*end_ptr != '\0')) {
        return false;
    }
    if (parsed > UINT32_MAX) {
        return false;
    }

    *out_value = (uint32_t)parsed;
    return true;
}

static bool parse_u64(const char *text, uint64_t *out_value) {
    char *end_ptr;
    unsigned long long parsed;

    if ((text == NULL) || (out_value == NULL) || (*text == '\0')) {
        return false;
    }

    errno = 0;
    parsed = strtoull(text, &end_ptr, 10);
    if ((errno != 0) || (end_ptr == text) || (*end_ptr != '\0')) {
        return false;
    }

    *out_value = (uint64_t)parsed;
    return true;
}

static void render_ascii(const chip8_t *chip8) {
    const uint8_t *display;
    size_t y;

    if (chip8 == NULL) {
        return;
    }

    display = chip8_get_display(chip8);
    if (display == NULL) {
        return;
    }

    puts("----------------------------------------------------------------");
    for (y = 0U; y < CHIP8_DISPLAY_HEIGHT; y++) {
        size_t x;
        for (x = 0U; x < CHIP8_DISPLAY_WIDTH; x++) {
            size_t idx = y * CHIP8_DISPLAY_WIDTH + x;
            putchar(display[idx] ? '#' : '.');
        }
        putchar('\n');
    }
}

int main(int argc, char **argv) {
    chip8_t chip8;
    chip8_status_t status;
    const char *rom_path;
    uint32_t cycles_per_frame = DEFAULT_CYCLES_PER_FRAME;
    uint64_t max_cycles = DEFAULT_MAX_CYCLES;
    bool ascii_output = false;
    int i;
    uint64_t cycles_executed = 0U;
    uint32_t frame_cycle_count = 0U;

    if ((argc < 2) || (argv == NULL)) {
        print_usage((argc > 0 && argv != NULL) ? argv[0] : "chip8");
        return EXIT_FAILURE;
    }

    rom_path = argv[1];
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--ascii") == 0) {
            ascii_output = true;
            continue;
        }

        if ((strcmp(argv[i], "--cycles-per-frame") == 0) && ((i + 1) < argc)) {
            uint32_t parsed;
            i++;
            if (!parse_u32(argv[i], &parsed) || (parsed == 0U)) {
                fprintf(stderr, "Invalid value for --cycles-per-frame: %s\\n", argv[i]);
                return EXIT_FAILURE;
            }
            cycles_per_frame = parsed;
            continue;
        }

        if ((strcmp(argv[i], "--max-cycles") == 0) && ((i + 1) < argc)) {
            uint64_t parsed;
            i++;
            if (!parse_u64(argv[i], &parsed) || (parsed == 0U)) {
                fprintf(stderr, "Invalid value for --max-cycles: %s\\n", argv[i]);
                return EXIT_FAILURE;
            }
            max_cycles = parsed;
            continue;
        }

        fprintf(stderr, "Unknown argument: %s\\n", argv[i]);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    chip8_init(&chip8);
    status = chip8_load_rom(&chip8, rom_path);
    if (status != CHIP8_OK) {
        fprintf(stderr, "Failed to load ROM '%s': %s\\n", rom_path, chip8_status_str(status));
        return EXIT_FAILURE;
    }

    while (cycles_executed < max_cycles) {
        status = chip8_cycle(&chip8);
        if (status != CHIP8_OK) {
            fprintf(stderr, "Runtime error after %" PRIu64 " cycles at PC=0x%03X: %s\\n",
                    cycles_executed,
                    chip8.pc,
                    chip8_status_str(status));
            return EXIT_FAILURE;
        }

        cycles_executed++;
        frame_cycle_count++;

        if (frame_cycle_count >= cycles_per_frame) {
            chip8_tick_timers(&chip8);
            frame_cycle_count = 0U;

            if (chip8.sound_timer > 0U) {
                fputc('\a', stdout);
                fflush(stdout);
            }

            if (ascii_output && chip8_consume_draw_flag(&chip8)) {
                render_ascii(&chip8);
            } else {
                (void)chip8_consume_draw_flag(&chip8);
            }
        }
    }

    printf("Execution finished after %" PRIu64 " cycles.\\n", cycles_executed);
    return EXIT_SUCCESS;
}