#include <stdbool.h>
#include "keyboard.h"
#include "synced_lcd.h"

#define SCREEN_WIDTH 9
#define SCREEN_HEIGHT 5
#define BUFFER_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

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

// Maintained for supporting the cursor (editing
// in the middle of the text buffer). Does not
// contain the cursor character.
char buffer_content[BUFFER_SIZE + 1] = "";
int cursor_position = 0;

void BufferReplaceChar(char new_char) {
    SyncedLCDbackspace();
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(new_char);
    SyncedLCDputcharWrap('/');
    buffer_content[cursor_position - 1] = new_char;
}

void BufferClear(void) {
    SyncedLCDclear();
    SyncedLCDputcharWrap('_');
    character_count = 0;
    cursor_position = 0;
    // Clear the whole buffer.
    // Setting 0 at position 0 is not sufficient, as it may get
    // overwritten and the garbage may reappear.
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        buffer_content[i] = '\0';
    }
}

void SynchroniseLCDCursor(void) {
    // Keep the LCD library cursor one char past the cursor char.
    int past_cursor_row = (cursor_position + 1) / SCREEN_WIDTH;
    int past_cursor_col = (cursor_position + 1) % SCREEN_WIDTH;
    SyncedLCDgoto(past_cursor_row, past_cursor_col);
}

void SynchroniseBufferPastCursor(void) {
    for (int i = cursor_position; buffer_content[i]; ++i) {
        SyncedLCDputcharWrap(buffer_content[i]);
    }
    SyncedLCDputcharWrap(' ');
    SynchroniseLCDCursor();
}

void InsertCharIntoBuffer(int position, char new_char, char *buffer) {
    // Assumes there is enough space in the buffer.
    int i;
    for (i = position; new_char; ++i) {
        char to_shift = buffer[i];
        buffer[i] = new_char;
        new_char = to_shift;
    }
    buffer[i] = '\0';
}

void BufferBackspace(void) {
    if (!cursor_position || !character_count) {
        return;
    }
    --cursor_position;
    int i;
    for (i = cursor_position; buffer_content[i]; ++i) {
        buffer_content[i] = buffer_content[i + 1];
    }
    --character_count;
    SyncedLCDbackspace();
    SyncedLCDbackspace();
    SyncedLCDputcharWrap('_');
    SynchroniseBufferPastCursor();
}

void BufferAddTemporary(char new_char) {
    // This assumes there is enough space in the buffer
    // for adding new charater - the user has to check that first.
    InsertCharIntoBuffer(cursor_position, new_char, buffer_content);
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(new_char);
    SyncedLCDputcharWrap('/');
    ++character_count;
    ++cursor_position;
    SynchroniseBufferPastCursor();
}

void BufferAddFixed(char new_char) {
    // Same assumption as in BufferAddTemporary.
    InsertCharIntoBuffer(cursor_position, new_char, buffer_content);
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(new_char);
    SyncedLCDputcharWrap('_');
    ++character_count;
    ++cursor_position;
    SynchroniseBufferPastCursor();
}

void BufferMoveLeft(void) {
    // ab_c -> a_bc
    if (!cursor_position) return;
    cursor_position--;
    SyncedLCDbackspace();
    SyncedLCDbackspace();
    SyncedLCDputcharWrap('_');
    SyncedLCDputcharWrap(buffer_content[cursor_position]);
    SynchroniseLCDCursor();
}

void BufferMoveRight(void) {
    // a_bc -> ab_c
    if (cursor_position >= character_count) return;
    cursor_position++;
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(buffer_content[cursor_position - 1]);
    SyncedLCDputcharWrap('_');
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

        if (LEFT_BUTTON.row == row && LEFT_BUTTON.col == col) {
            BufferMoveLeft();
        } else if (CLEAR_BUTTON.row == row && CLEAR_BUTTON.col == col) {
            BufferClear();
        } else if(BACKSPACE_BUTTON.row == row && BACKSPACE_BUTTON.col == col) {
            BufferBackspace();
        } else if (RIGHT_BUTTON.row == row && RIGHT_BUTTON.col == col) {
            BufferMoveRight();
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
