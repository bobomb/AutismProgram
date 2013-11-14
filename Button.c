/*
 * Button.c
 *
 *  Created on: Nov 3, 2013
 *      Author: Mo
 */
#include "Button.h"


void ButtonUpdate(TButtonInfo * pButton, unsigned long deltaT, char buttonValue)
{
	//interrupt triggered, reset button status
	if(pButton->interruptEvent)
	{
		pButton->interruptEvent = 0;
		pButton->buttonState = Pressed;
		pButton->pressCounter = 0;
		pButton->pressEvent = 0;
		pButton->holdEvent  = 0;
		pButton->releaseEvent = 0;
		return;
	}

	if(pButton->buttonState == Pressed || pButton->buttonState == Debounced || pButton->buttonState == Held)
		pButton->pressCounter += deltaT;

	switch(pButton->buttonState)
	{
	case Pressed:
		if((pButton->pressCounter >= BUTTON_PRESS_TICKS) && !buttonValue) //button is active-low
		{
			pButton->pressEvent = 1;
			pButton->pressCounter = 0;
			pButton->buttonState = Debounced;
			pButton->buttonValue = buttonValue;
		}
		break;
	case Debounced:
		if(pButton->pressCounter >= BUTTON_HOLD_TICKS && pButton->buttonValue == buttonValue)
		{
			pButton->holdEvent = 1;
			pButton->buttonState = Held;
		}
		else if (pButton->buttonValue != buttonValue)
		{
			pButton->buttonState = Unpressed;
			pButton->releaseEvent = 1;
			//pButton->pressCounter = 0;
		}
		break;
	case Held:
		if(pButton->buttonValue != buttonValue)
		{
			pButton->releaseEvent = 1;
			pButton->buttonState = Unpressed;
		}
		break;
	}


}


void ButtonClearEvents(TButtonInfo * pButton)
{
	pButton->pressCounter = 0;
	pButton->pressEvent = 0;
	pButton->holdEvent  = 0;
	pButton->releaseEvent = 0;
}
