#define WIDTH 9
#define HEIGHT 5

#include <stdbool.h>
#include "synced_lcd.h"
#include "lcd.h"

static char state[HEIGHT][WIDTH];
static int current_row, current_col;
static bool is_synced[HEIGHT][WIDTH];

void SyncedLCDconfigure(void) {
    SyncedLCDclear();
    LCDconfigure();
    SyncedLCDsync();
}

void SyncedLCDclear(void) {
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            state[i][j] = ' ';
            is_synced[i][j] = false;
        }
    }
    current_row = 0;
    current_col = 0;
}

void SyncedLCDputcharWrap(char c) {
    if (current_row < HEIGHT && current_col < WIDTH) {
        state[current_row][current_col] = c;
        is_synced[current_row][current_col] = false;
    }
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
        is_synced[current_row][current_col] = false;
    }
}

void SyncedLCDsync() {
    LCDgoto(0, 0);
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            if (is_synced[current_row][current_col]) continue;
            LCDgoto(i, j);
            LCDputchar(state[i][j]);
            is_synced[current_row][current_col] = true;
        }
    }
}

void SyncedLCDgoto(int row, int col) {
    current_row = row;
    current_col = col;
}
