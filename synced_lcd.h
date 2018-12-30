#ifndef _SYNCED_LCD_H
#define _SYNCED_LCD_H 1

void SyncedLCDconfigure(void);
void SyncedLCDclear(void);
void SyncedLCDputcharWrap(char c);
void SyncedLCDbackspace(void);
void SyncedLCDsync(void);

#endif
