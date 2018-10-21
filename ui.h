#ifndef __UI_H__
#define __UI_H__

#include "board.h"

void ui_init(void);
void ui_buttons_isr(void);
void ui_lcd_transfer_complete( int );
#endif // __UI_H__

