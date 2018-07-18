#include "fan.h"

void Speed::loop()
{
	if (wait_start - millis() >= RPM_FREQ)
	{
		wait_start = millis();
		calc_speed();
	}
}

void Speed::step()
{
	volatile unsigned long currentMicros = micros();

	if (currentMicros - previousMicros > 20000) // Prevent pulses less than 20k micros far.
	{
		duration += currentMicros - previousMicros;
		previousMicros = currentMicros;
		ticks++;
	}
}

void Speed::calc_speed()
{
	unsigned long _duration = duration;
	unsigned long _ticks = ticks;
	duration = 0;

	float Freq = (1e6 / float(_duration) * _ticks) / 2;
	speed = Freq * 60;
	ticks = 0;
}

int Speed::get_speed()
{
	return speed;
}

Fan::Fan(byte relay_pin, byte hall_pin)
{
	relay = relay_pin;
	rpm = Speed(hall_pin);
}

void Fan::setup()
{
	pinMode(relay, OUTPUT);

	/* Configure PWM clock */
	TCCR4B = 4; /* 4 sets 23437Hz */
	TCCR4C = 0;
	TCCR4D = 0;
	PLLFRQ = (PLLFRQ & 0xCF) | 0x30;
	OCR4C = 255; // Terminal count for Timer 4 PWM

	/* Set PWM to D6 (Timer4 D) */
	OCR4D = 255;	// Set PWM value (0-255)
	DDRD |= 1 << 7; // Set Output Mode D7
	TCCR4C |= 0x09; // Activate channel D
}

void Fan::loop()
{
	rpm.loop();
}

void Fan::step()
{
	rpm.step();
}

bool Fan::is_running()
{
	return running;
}

void Fan::update_fan()
{
	// Turn the fan ON/OFF
	running = (duty >= DUTY_MIN);
	duty *= running;

	digitalWrite(relay, !running); // HIGH = Off
	PWM6 = duty;
}

void Fan::set_percent(byte percent)
{
	duty = (percent * 255) / 100;

	update_fan();
}

byte Fan::get_percent()
{
	return (duty * 100) / 255;
}

void Fan::set_duty(byte duty)
{
	this->duty = duty;

	update_fan();
}

byte Fan::get_duty()
{
	return duty;
}

int Fan::get_speed()
{
	return rpm.get_speed();
}