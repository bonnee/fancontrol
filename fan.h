#ifndef FAN_H
#include <Arduino.h>

// Pin 6 shortcut
#define PWM6 OCR4D

// Terminal count
#define PWM6_MAX OCR4C

// The minimum fan speed (0...255)
#define DUTY_MIN 64

// The delta between the minimum output for the PID and DUTY_MIN (DUTY_MIN - DUTY_DEAD_ZONE).
#define DUTY_DEAD_ZONE 64

#define RPM_FREQ 500

class Speed
{
	byte hall;
	int ticks = 0, speed = 0;

	unsigned long duration = 0; // accumulates pulse width
	unsigned int pulsecount = 0;
	unsigned long previousMicros = 0;

	unsigned long wait_start;

	void calc_speed();

  public:
	Speed(byte hall_pin) : hall(hall_pin) {}

	void step();
	void loop();

	int get_speed();
};

class Fan
{
	byte relay;

	Speed rpm = Speed(0);
	bool running;
	byte duty;

	void update_fan();

  public:
	Fan(byte relay_pin, byte hall_pin);

	void setup();
	void loop();

	void step();
	bool is_running();

	void set_percent(byte percent);
	byte get_percent();

	void set_duty(byte duty);
	byte get_duty();

	int get_speed();
};
#endif