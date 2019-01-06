#include "keyboard.h"
#include "synced_lcd.h"

// Keyboard layout definition.
char* layout[4][4] = {
    {"1", "abc2", "def3", ""},     
    {"ghi4", "jkl5", "mno6", ""},
    {"prs7", "tuv8", "wxy9", ""},
    {"*", " 0", "#", ""},
};

// Special char: clear.
struct {
    int row;
    int col;
} clear_button = {2, 3};

// Special char: backspace.
struct {
    int row;
    int col;
} backspace_button = {1, 3};

// Is roundabout on? Which button?
struct {
    int row;
    int col;
} current_roundabout_button = {-1, -1};

// Which choice, e.g. 0 for a, 1 for b.
int current_roundabout_position = 0;

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
    SyncedLCDbackspace();
    SyncedLCDputcharWrap(current_char);
}

void ButtonPressed(int row, int col) {
    if (clear_button.row == row && clear_button.col == col) {
        SyncedLCDclear();
        SyncedLCDputcharWrap('_');
    } else if(backspace_button.row == row && backspace_button.col == col) {
        if (current_roundabout_button.row == -1) {
            SyncedLCDbackspace();
        }
        SyncedLCDbackspace();
        SyncedLCDputcharWrap('_');
    } else {
        if (current_roundabout_button.row == row &&
            current_roundabout_button.col == col) {
            ButtonRepeat();
        } else {
            if (current_roundabout_button.row == -1)
                SyncedLCDbackspace();  // erase cursor
            current_roundabout_button.row = -1;
            current_roundabout_button.col = -1;  // call fix button?
            current_roundabout_position = 0;
            char* choice = layout[row][col];
            if (*choice && choice[1]) {
                current_roundabout_button.row = row;
                current_roundabout_button.col = col;
                SyncedLCDputcharWrap(*choice);
                // cursor will be added at fix

            } else if (*choice) {
                SyncedLCDputcharWrap(*choice);
                SyncedLCDputcharWrap('_');
            }
        }
    }
}

void FixButton(void) {
    if (current_roundabout_button.row != -1) {
        SyncedLCDputcharWrap('_');
    }
    current_roundabout_button.row = -1;
    current_roundabout_button.col = -1;
}

void AmbiguousPress(void) {
    // Ambiguous press - do nothing.
}

int main() {
    SyncedLCDconfigure();
    SyncedLCDclear();
    SyncedLCDputcharWrap('_');

    KeyboardConfigure();

    for (int i = 0;; ++i) {
        SyncedLCDsync();
    }
}
