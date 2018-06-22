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

bool Fan::isRunning()
{
	return running;
}

void Fan::setDuty(double duty)
{
	// Turn the fans ON/OFF
	if (round(duty) < DUTY_MIN)
	{
		digitalWrite(relay, HIGH);
		PWM6 = 0;
		running = false;
	}
	else
	{
		digitalWrite(relay, LOW);
		PWM6 = duty;
		running = true;
	}
}

int Fan::getSpeed()
{
	return rpm.getSpeed();
}