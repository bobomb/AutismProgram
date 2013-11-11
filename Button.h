/*
 * Button.h
 *
 *  Created on: Nov 3, 2013
 *      Author: Mo
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#define BUTTON_PRESS_TICKS 25
#define BUTTON_HOLD_TICKS 1000

typedef enum { Pressed, Unpressed, Held, Debounced, DebounceUnpressed} ButtonState;
extern volatile unsigned long currentTimeMS;

typedef struct {
	ButtonState buttonState;
	unsigned int pressCounter;
	unsigned int holdCounter;
	char buttonValue;
	char buttonPressed;
	char pressEvent;
	char holdEvent;
	char releaseEvent;
} TButtonInfo;

void ButtonUpdate(TButtonInfo * pButton, unsigned long deltaT, char buttonValue);
void ButtonSetPressed(TButtonInfo * pButton);

#endif /* BUTTON_H_ */
