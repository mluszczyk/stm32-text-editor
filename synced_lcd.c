#define WIDTH 9
#define HEIGHT 5

#include <stdbool.h>
#include "synced_lcd.h"
#include "lcd.h"

static char state[HEIGHT][WIDTH];
static int current_row, current_col;
static bool is_synced;

void SyncedLCDconfigure(void) {
    SyncedLCDclear();
    LCDconfigure();
    SyncedLCDsync();
}

void SyncedLCDclear(void) {
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            state[i][j] = ' ';
        }
    }
    current_row = 0;
    current_col = 0;
    is_synced = false;
}

void SyncedLCDputcharWrap(char c) {
    if (current_row < HEIGHT && current_col < WIDTH) {
        state[current_row][current_col] = c;
    }
    is_synced = false;
    // Advance position.
    if (current_col + 1 < WIDTH) {
        current_col++;
    } else {
        current_row++; // may get out of bound, but it's OK
        current_col = 0;
    }
}

void SyncedLCDbackspace(void) {
    if (current_col > 0) {
        current_col--;
    } else if (current_row > 0) {
        current_row--;
        current_col = WIDTH - 1;
    }
    if (current_row >= 0 && current_row < HEIGHT &&
        current_col >= 0 && current_col < WIDTH) {
        state[current_row][current_col] = ' ';
    }
    is_synced = false;
}

void SyncedLCDsync() {
    if (is_synced) return;
    LCDgoto(0, 0);
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            LCDgoto(i, j);
            LCDputchar(state[i][j]);
        }
    }
    is_synced = true;
}
