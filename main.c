#include <stdbool.h>
#include "keyboard.h"
#include "synced_lcd.h"

#define BUFFER_SIZE 45

// Keyboard layout definition.
char* layout[4][4] = {
    {"1", "abc2", "def3", ""},     
    {"ghi4", "jkl5", "mno6", ""},
    {"prs7", "tuv8", "wxy9", ""},
    {"*", " 0", "#", ""},
};

struct Button {
    int row;
    int col;
};

// Special button: left arrow.
const struct Button LEFT_BUTTON = {0, 3};

// Special button: backspace.
const struct Button BACKSPACE_BUTTON = {1, 3};

// Special button: clear.
const struct Button CLEAR_BUTTON = {2, 3};

// Special button: right arrow.
const struct Button RIGHT_BUTTON = {3, 3};

// Is roundabout on? Which button?
struct Button current_roundabout_button = {-1, -1};

// Which choice, e.g. 0 for a, 1 for b.
int current_roundabout_position = 0;

// Character count.
// Used to block the input when the screen is full.
int character_count = 0;

void BufferReplaceChar(char new_char) {
    SyncedLCDbackspace();
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(new_char);
    SyncedLCDputcharWrap('/');
}

void BufferClear(void) {
    SyncedLCDclear();
    SyncedLCDputcharWrap('_');
    character_count = 0;
}

void BufferBackspace(void) {
    SyncedLCDbackspace();
    SyncedLCDbackspace();
    SyncedLCDputcharWrap('_');
    if (character_count)
        --character_count;
}

void BufferAddTemporary(char new_char) {
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(new_char);
    SyncedLCDputcharWrap('/');
    ++character_count;
}

void BufferAddFixed(char new_char) {
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(new_char);
    SyncedLCDputcharWrap('_');
    ++character_count;
}

void BufferFix(void) {
    SyncedLCDbackspace();
    SyncedLCDputcharWrap('_');
}

bool BufferIsFull(void) {
    // Always leave one char for the cursor.
    return character_count + 1 >= BUFFER_SIZE;
}

void ButtonRepeat(void) {
    current_roundabout_position++;
    if (!layout[current_roundabout_button.row]
               [current_roundabout_button.col]
               [current_roundabout_position]) {
        current_roundabout_position = 0;
    }
    char current_char = layout[current_roundabout_button.row]
               [current_roundabout_button.col]
               [current_roundabout_position];
    BufferReplaceChar(current_char);
}

void ButtonPressed(int row, int col) {
    if (current_roundabout_button.row == row &&
        current_roundabout_button.col == col) {
        ButtonRepeat();
    } else {
        current_roundabout_button.row = -1;
        current_roundabout_button.col = -1;  // call fix button?
        current_roundabout_position = 0;

        if (CLEAR_BUTTON.row == row && CLEAR_BUTTON.col == col) {
            BufferClear();
        } else if(BACKSPACE_BUTTON.row == row && BACKSPACE_BUTTON.col == col) {
            BufferBackspace();
        } else {
            if (BufferIsFull()) return;
            char* choice = layout[row][col];
            if (*choice && choice[1]) {
                current_roundabout_button.row = row;
                current_roundabout_button.col = col;
                BufferAddTemporary(*choice);

            } else if (*choice) {
                BufferAddFixed(*choice);
            }
        }
    }
}

void FixButton(void) {
    if (current_roundabout_button.row != -1) {
        BufferFix();
    }
    current_roundabout_button.row = -1;
    current_roundabout_button.col = -1;
}

void AmbiguousPress(void) {
    // Ambiguous press - do nothing.
}

int main() {
    SyncedLCDconfigure();
    BufferClear();

    KeyboardConfigure();

    for (int i = 0;; ++i) {
        SyncedLCDsync();
    }
}
