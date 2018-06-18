#include "button.h"

void Button::setup()
{
	pinMode(pin, INPUT_PULLUP);
	state = HIGH;
}

bool Button::loop()
{
	int prevState = state;
	state = digitalRead(pin);

	if (prevState == HIGH && state == LOW)
	{
		buttonDownMs = millis();
	}
	else if (prevState == LOW && state == HIGH)
	{
		if (millis() - buttonDownMs < 50)
		{
			// ignore this for debounce
		}
		else
		{
			return true;
		}
	}
	return false;
}