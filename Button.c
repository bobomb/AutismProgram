/*
 * Button.c
 *
 *  Created on: Nov 3, 2013
 *      Author: Mo
 */
#include "Button.h"


void ButtonUpdate(TButtonInfo * pButton, unsigned long deltaT, char buttonValue)
{
	pButton->pressCounter += deltaT;

	switch(pButton->buttonState)
	{
	case Pressed:
		if(pButton->pressCounter >= BUTTON_PRESS_TICKS)
		{
			pButton->pressCounter = 0;
			pButton->pressEvent = 1;
			pButton->buttonState = Debounced;
			pButton->buttonValue = buttonValue;
		}
		break;
	case Debounced:
		//if(pButton->pressCounter >= BUTTON_HOLD_TICKS && pButton->buttonValue == buttonValue)
		if(pButton->pressCounter >= BUTTON_HOLD_TICKS && pButton->buttonValue == buttonValue)
		{
			pButton->pressCounter = 0;
			pButton->holdEvent = 1;
			pButton->buttonState = Held;
			pButton->buttonValue = buttonValue;
		}
		else if (pButton->buttonValue != buttonValue) //we went from press to debounce, but the button changed...must have been released
		{
			pButton->pressCounter = 0;
			pButton->pressEvent = 0;
			pButton->holdEvent = 0;
			//pButton->releaseEvent = 1;
			pButton->buttonState = Unpressed;
		}
		break;
	case Unpressed:
		break;
	case Held:
			pButton->pressEvent = 0;
			pButton->holdEvent = 0;
			pButton->buttonState = Unpressed;
			pButton->buttonValue = buttonValue;
		break;
	case DebounceUnpressed:
		if(pButton->pressCounter >= BUTTON_PRESS_TICKS)
		{
			pButton->pressCounter = 0;
			pButton->releaseEvent = 1;
			pButton->buttonState = Unpressed;
		}
		break;
	}
}

void ButtonSetPressed(TButtonInfo * pButton)
{
	if(pButton->buttonState == Unpressed)
	{
	pButton->buttonState = Pressed;
	pButton->pressCounter = 0;
	pButton->pressEvent = 0;
	pButton->holdEvent  = 0;
	pButton->releaseEvent = 0;
	}
}
