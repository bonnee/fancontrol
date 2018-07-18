#include "button.h"

void Button::setup()
{
	pinMode(pin, INPUT_PULLUP);
	state = HIGH;
}

bool Button::get_state()
{
	int prev_state = state;
	state = digitalRead(pin);

	if (prev_state == HIGH && state == LOW)
	{
		button_down_ms = millis();
	}
	else if (prev_state == LOW && state == HIGH)
	{
		if (millis() - button_down_ms < 50)
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