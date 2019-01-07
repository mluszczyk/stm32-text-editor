#ifndef _SYNCED_LCD_H
#define _SYNCED_LCD_H 1

// These functions operate on device memory only
// without communicating with LCD.
void SyncedLCDconfigure(void);
void SyncedLCDclear(void);
void SyncedLCDgoto(int textLine, int charPos);
void SyncedLCDputcharWrap(char c);
void SyncedLCDbackspace(void);

// This function synchronises the modified cells
// with LCD.
void SyncedLCDsync(void);

#endif
