/* fancontrol
 
  fancontrol is free software.
  Copyright (C) 2017 Matteo Bonora (bonora.matteo@gmail.com) - All Rights Reserved
 
  fancontrol is available under the GNU LGPLv3 License which is available at <http://www.gnu.org/licenses/lgpl.html>

  This is a temperature-based fan controller using PID logic and PWM signals to control PC fans.

  Check out my instructable on this project for more info
  https://www.instructables.com/id/Temperature-Control-With-Arduino-and-PWM-Fans/
  
  The PWM frequency is set to 23437 Hz that is within the 21k-25k Hz range so it should work with any PC fan.
  For more details on how PWM works on the 32u4, check this link:

  http://r6500.blogspot.it/2014/12/fast-pwm-on-arduino-leonardo.html

  Note that the 32u4 has 2 pins (6 and 13) hooked to Timer4, but since my board doesn't have pin 13 I only configure pin 6.
  A RPM reading system is also featured in this example (although it has proven to be not that accurate, at least on my setup).

  This code has been tested on a SparkFun Pro Micro 16MHz Clone with 4 Arctic F12 PWM PST Fans connected to the same connector.
*/

#include <PID_v1.h> // https://github.com/br3ttb/Arduino-PID-Library
#include "config.h"
#include "temp.h"
#include "fan.h"
#include "lcd.h"
#include "button.h"

/*        Pinouts       */
#define SPD_IN 7 // RPM input pin

// 7-segment display
#define SEG_DIN 16
#define SEG_CLK 14
#define SEG_CS 15
#define LCD_BRIGHTNESS 15

#define RELAY 9 // Relay output pin

#define TEMP_IN 4    // Temperature sensor input pin
#define TARGET_UP 18 // Up/Down buttons pins
#define TARGET_DOWN 19

#define DBG false
// Debug macro to print messages to serial
#define DEBUG(x)     \
  if (DBG && Serial) \
  {                  \
    Serial.print(x); \
  }

// Tells the amount of time (in ms) to wait between updates
#define WAIT 500

#define KP 0.4
#define KI 0.4
#define KD 0.05

/* Target set vars */
bool target_mode = false;
bool up, down = false;

unsigned long prev1, prev2, prev3 = 0; // Time placeholders

// Display temp, .5 rounded and Compute temp, integer (declared as double because of PID library input);
double display_temp, compute_temp;

// Fan status
bool fanRunning = true;

double duty, target;

// Initialize all the libraries.
Config cfg = Config();
PID pid = PID(&compute_temp, &duty, &target, KP, KI, KD, REVERSE);
LCD lcd = LCD(SEG_DIN, SEG_CLK, SEG_CS);
Temp sensor = Temp(TEMP_IN);
Fan fan = Fan(RELAY, SPD_IN);
Button up_btn = Button(TARGET_UP);
Button down_btn = Button(TARGET_DOWN);

void step()
{
  fan.step();
}

void setup()
{
  Serial.begin(19200);
  if (DBG)
  {
    while (!Serial)
    {
    }
  }

  DEBUG("Fans...");

  attachInterrupt(digitalPinToInterrupt(SPD_IN), step, FALLING);
  fan.setup();

  while (fan.get_speed() < 1000)
  {
    delay(500);
    DEBUG(fan.get_speed());
    DEBUG("RPM...");
  }
  fan.set_duty(0);
  DEBUG("Ok.");

  DEBUG("Sensor...");
  sensor.setup();

  DEBUG("Display...");
  lcd.setup(LCD_BRIGHTNESS);

  DEBUG("I/O...");
  up_btn.setup();
  down_btn.setup();

  cfg.setup();

  DEBUG("Logic...");
  // Setup the PID to work with our settings
  pid.SetSampleTime(WAIT);
  pid.SetOutputLimits(DUTY_MIN - DUTY_DEAD_ZONE, 255);
  pid.SetMode(AUTOMATIC);

  DEBUG("Ready.\n\n");
  //lc.clearDisplay(0);

  prev1 = millis();
}

void loop()
{
  bool should_print = false;

  // Update the temperature

  if (sensor.loop())
  {
    double temp = sensor.get_temp();

    display_temp = round(temp * 2.0) / 2.0;
    compute_temp = round(temp);

    should_print = true;
  }

  target = cfg.get_target();
  pid.Compute();
  fan.set_duty(round(duty));

  if (Serial && millis() - prev1 >= WAIT)
  {
    prev1 = millis();

    char *status = "";
    sprintf(status, "trg:%f:temp:%f:perc:%u:rpm:%u\n", cfg.get_target(), display_temp, fan.get_percent(), fan.get_speed());
  }

  bool up = up_btn.get_state(), down = down_btn.get_state();

  if (up)
    cfg.set_target(target + 1);
  if (down)
    cfg.set_target(target - 1);

  // If either + or - buttons are pressed, enter target mode and display the current target on the lcd.
  if (up || down)
  {
    prev2 = millis();
    target_mode = true;
    should_print = true;
  }

  // If 3 secs have elapsed and no button has been pressed, exit target mode.
  if (target_mode && millis() - prev2 >= 3000)
  {
    target_mode = false;
    should_print = true;

    cfg.flush(); // Save config to EEPROM
  }

  if (should_print)
    lcd.update(display_temp, cfg.get_target(), fan.get_percent(), target_mode);
}
