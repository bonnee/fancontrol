/*    fancontrol
 *
 * fancontrol is free software.
 * Copyright (C) 2017 Matteo Bonora (bonora.matteo@gmail.com) - All Rights Reserved
 *
 * fancontrol is available under the GNU LGPLv3 License which is available at <http://www.gnu.org/licenses/lgpl.html>

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

#include <PID_v1.h>     // https://github.com/br3ttb/Arduino-PID-Library
#include <LedControl.h> // https://github.com/wayoda/LedControl
#include "config.h"
#include "temp.h"
#include "fan.h"
#include "button.h"

/*        Pinouts       */
#define SPD_IN 7 // RPM input pin

// 7-segment display
#define SEG_DIN 16
#define SEG_CLK 14
#define SEG_CS 15

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
bool targetMode = false;
bool up, down = false;

unsigned long prev1, prev2, prev3 = 0; // Time placeholders

// Display temp, .5 rounded and Compute temp, integer (declared as double because of PID library input);
double display_temp, compute_temp;

// Fan status
bool fanRunning = true;

double duty, target;

// Initialize all the libraries.
Config cfg = Config();
PID fanPID(&compute_temp, &duty, &target, KP, KI, KD, REVERSE);
LedControl lc = LedControl(SEG_DIN, SEG_CLK, SEG_CS, 1);
Temp sensor = Temp(TEMP_IN);

Button up_btn = Button(TARGET_UP);
Button down_btn = Button(TARGET_DOWN);

Fan fan = Fan(RELAY, SPD_IN);

/* LCD MANAGEMENT FUNCTIONS */

/* Writes 'str' to the lcd, starting at 'index' */
void writeSeg(const char str[], byte index)
{
  int size = strlen(str);

  for (int i = 0; i < size; i++)
  {
    lc.setChar(0, index + i, str[(size - 1) - i], false);
  }
}

/* writes the temperature on the lcd. 'off' defines the offset and dInt defines whether the temp is an int or a float */
void writeTemp(float temp, byte off, bool dInt = false)
{
  byte t[3];

  if (!dInt) // If it's a float, then multiply by 10 to get rid of the decimal value
  {
    temp *= 10;
  }

  // Split the value in an array of bytes
  t[0] = (int)temp % 10;
  temp /= 10;

  t[1] = (int)temp % 10;

  if (!dInt)
  {
    temp /= 10;
    t[2] = (int)temp % 10;
  }

  // Do the actual printing
  for (byte i = 1; i < 4; i++)
  {
    lc.setDigit(0, i + off, t[i - 1], (i == 2 && !dInt));
  }
  lc.setChar(0, off, 'C', false);
}

/* Calls the right functions to fill the left half of the lcd */
void writeLeft()
{
  if (targetMode)
  {
    writeSeg("Set ", 4);
  }
  else
  {
    writeTemp(display_temp, 4);
  }
}

/* Calls the right functions to fill the right half of the lcd */
void writeRight()
{
  if (targetMode)
  {
    writeTemp(cfg.getTarget(), 0, true);
  }
  else
  {
    char tmp[5];
    if (fanRunning)
    {
      sprintf(tmp, "%4u", map(round(duty), 0, 255, 0, 100));
    }
    else
    {
      strcpy(tmp, " 0ff");
    }
    writeSeg(tmp, 0);
  }
}

/* Routine that updates the display */
void printSeg()
{
  writeRight();
  writeLeft();
}

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
    } /*   WAIT FOR THE SERIAL CONNECTION FOR DEBUGGING   */
  }

  DEBUG("Fans...");

  attachInterrupt(digitalPinToInterrupt(SPD_IN), step, FALLING);
  fan.setup();

  // Let the fan run for 5s. Here we could add a fan health control to see if the fan revs to a certain value.
  delay(5000);

  DEBUG("Sensor...");
  sensor.setup();

  DEBUG("I/O...");
  up_btn.setup();
  down_btn.setup();

  DEBUG("Display...");
  lc.clearDisplay(0);
  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

  writeSeg("Fan Ctrl", 0);

  cfg.setup();

  DEBUG("Controller...");
  // Setup the PID to work with our settings
  fanPID.SetSampleTime(WAIT);
  fanPID.SetOutputLimits(DUTY_MIN - DUTY_DEAD_ZONE, 255);
  fanPID.SetMode(AUTOMATIC);

  DEBUG("Ready.\n\n");
  lc.clearDisplay(0);

  prev1 = millis();
}

void loop()
{
  unsigned long now = millis();
  bool shouldPrint = false;

  // Update the temperature
  int temp = sensor.loop(now);
  if (temp > 0)
  {
    display_temp = round(temp * 2.0) / 2.0;
    compute_temp = round(temp);
  }

  target = cfg.getTarget();
  fanPID.Compute(); // Do magic
  fan.setDuty(duty);

  if (now - prev1 >= WAIT)
  {
    shouldPrint = true; // Things have changed. remind to update the display

    //DEBUG(sensor.getStatusString());
    DEBUG(" - Target: ");
    DEBUG(cfg.getTarget());
    DEBUG(" - Temp: ");
    DEBUG(compute_temp);
    DEBUG(" - Duty: ");
    DEBUG(map(round(duty), 0, 255, 0, 100));
    DEBUG("\n");
  }

  if (up_btn.loop())
    cfg.setTarget(target + 1);
  if (down_btn.loop())
    cfg.setTarget(target - 1);

  /* If either + or - buttons are pressed, enter target mode and display the current target on the lcd. */
  if (up || down)
  {
    targetMode = true;
    shouldPrint = true;
    prev2 = now;
  }

  /* If 3 secs have elapsed and no button has been pressed, exit target mode. */
  if (targetMode && now - prev2 >= 3000)
  {
    targetMode = false;
    shouldPrint = true;

    cfg.write(); // Save the config only when exiting targetMode to reduce EEPROM wear
  }

  if (shouldPrint)
    printSeg();
}
