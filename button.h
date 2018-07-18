#ifndef BUTTON_H
#include <Arduino.h>

class Button
{
	const byte pin;
	int state;
	unsigned long button_down_ms;

public:
	Button(byte attachTo) : pin(attachTo) {}

	void setup();
	bool get_state();
};
#endif