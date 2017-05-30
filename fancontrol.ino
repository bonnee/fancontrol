/*    fancontrol
 *
 * fancontrol is free software.
 * Copyright (C) 2017 Matteo Bonora (bonora.matteo@gmail.com) - All Rights Reserved
 *
 * fancontrol is available under the GNU LGPLv3 License which is available at <http://www.gnu.org/licenses/lgpl.html>
*/

/*
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

#include <PID_v1.h>       // https://github.com/br3ttb/Arduino-PID-Library
#include <DHT.h>          // https://github.com/markruys/arduino-DHT
#include <LedControl.h>   // https://github.com/wayoda/LedControl
#include <EEPROM.h>

// Change this if you want your current settings to be overwritten.
#define CONFIG_VERSION "f01"
// Where to store config data in EEPROM
#define CONFIG_START 32

// Pin 6 shortcut
#define PWM6        OCR4D

// Terminal count
#define PWM6_MAX OCR4C

/*        Pinouts       */
#define SPD_IN 7        // RPM input pin

// 7-segment display
#define SEG_DIN 16
#define SEG_CLK 14
#define SEG_CS 15

#define RELAY 9         // Relay output pin

#define TEMP_IN 4       // Temperature sensor input pin
#define TARGET_UP 18    // Up/Down buttons pins
#define TARGET_DOWN 19

#define DBG true
// Debug macro to print messages to serial
#define DEBUG(x)  if(DBG && Serial) { Serial.print (x); }

// Tells the amount of time (in ms) to wait between updates
#define WAIT 500

#define DUTY_MIN 64        // The minimum fans speed (0...255)
#define DUTY_DEAD_ZONE 64  // The delta between the minimum output for the PID and DUTY_MIN (DUTY_MIN - DUTY_DEAD_ZONE).

/* Target set vars */
bool targetMode = false;
bool lastUp = false, lastDown = false;
bool up, down = false;

/* RPM calculation */
volatile unsigned long duration = 0; // accumulates pulse width
volatile unsigned int pulsecount = 0;
volatile unsigned long previousMicros = 0;
int ticks = 0, speed = 0;

unsigned long prev1, prev2, prev3 = 0; // Time placeholders

double duty;
// Display temp, .5 rounded and Compute temp, integer (double for PID library);
double dtemp, ctemp;

bool fanRunning = true;

struct StoreStruct
{
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  double target;
} storage = { // Default values
  CONFIG_VERSION,
  40
};

PID fanPID(&ctemp, &duty, &storage.target, 1, 0.5, 0.05, REVERSE);
LedControl lc = LedControl(SEG_DIN, SEG_CLK, SEG_CS, 1);
DHT sensor;

/* Configure the PWM clock */
void pwm6configure()
{
  // TCCR4B configuration
  TCCR4B = 4; /* 4 sets 23437Hz */

  // TCCR4C configuration
  TCCR4C = 0;

  // TCCR4D configuration
  TCCR4D = 0;

  // PLL Configuration
  PLLFRQ = (PLLFRQ & 0xCF) | 0x30;

  // Terminal count for Timer 4 PWM
  OCR4C = 255;
}

// Set PWM to D6 (Timer4 D)
// Argument is PWM between 0 and 255
void pwmSet6(int value)
{
  OCR4D = value;  // Set PWM value
  DDRD |= 1 << 7; // Set Output Mode D7
  TCCR4C |= 0x09; // Activate channel D
}

//  Called when hall sensor pulses
void pickRPM ()
{
  volatile unsigned long currentMicros = micros();

  if (currentMicros - previousMicros > 20000) {   // Prevent pulses less than 20k micros far.
    duration += currentMicros - previousMicros;
    previousMicros = currentMicros;
    ticks++;
  }
}

void loadConfig()
{
  // Check if saved bytes have the same "version" and loads them. Otherwise it will load the default values.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t = 0; t < sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig()
{
  for (unsigned int t = 0; t < sizeof(storage); t++)
    EEPROM.update(CONFIG_START + t, *((char*)&storage + t));
}

void writeSeg(char str[], byte index)
{
  int size = strlen(str);

  for (int i = 0; i < size; i++)
  {
    lc.setChar(0, index + i, str[(size - 1) - i], false);
  }
}

void writeTemp(float temp, byte off, bool dInt = false)
{
  byte t[3];

  if (!dInt)
  {
    temp *= 10;
  }

  t[0] = (int)temp % 10;
  temp /= 10;

  t[1] = (int)temp % 10;

  if (!dInt)
  {
    temp /= 10;
    t[2] = (int)temp % 10;
  }


  for (byte i = 1; i < 4; i++)
  {
    lc.setDigit(0, i + off, t[i - 1], (i == 2 && !dInt));
  }
  lc.setChar(0, off, 'C', false);
}

void writeLeft()
{
  if (targetMode)
  {
    writeSeg("Set ", 4);
  }
  else
  {
    writeTemp(dtemp, 4);
  }
}

void writeRight()
{
  if (targetMode)
  {
    writeTemp(storage.target, 0, true);
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

// Routine that updates the display
void printSeg()
{
  writeRight();
  writeLeft();
}

void setup()
{
  pinMode(SPD_IN, INPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(TARGET_UP, INPUT_PULLUP);
  pinMode(TARGET_DOWN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SPD_IN), pickRPM, FALLING);

  lc.clearDisplay(0);
  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

  writeSeg("Fan Ctrl", 0);

  // Configure Timer 4 (Pin 6)
  pwm6configure();

  loadConfig();

  fanPID.SetSampleTime(WAIT);
  fanPID.SetOutputLimits(DUTY_MIN - DUTY_DEAD_ZONE, 255);
  fanPID.SetMode(AUTOMATIC);

  Serial.begin(19200);
  //while (!Serial) {}    /*   WARNING: FOR DEBUG ONLY   */
  DEBUG("Fans...");
  pwmSet6(255);
  delay(5000);
  sensor.setup(TEMP_IN);
  DEBUG("Ready.\n\n");
  lc.clearDisplay(0);

  prev1 = millis();
}

void loop()
{
  unsigned long cur = millis();
  bool shouldPrint = false;

  lastUp = up;
  lastDown = down;
  up = !digitalRead(TARGET_UP);
  down = !digitalRead(TARGET_DOWN);

  if (cur - prev3 >= sensor.getMinimumSamplingPeriod())
  {
    if (sensor.getStatus() == 0)
    {
      prev3 = cur;
      double t = sensor.getTemperature();
      if (!isnan(t))
      {
        dtemp = round(t * 2.0) / 2.0;
        ctemp = round(t);
      }
    } else
    {
      prev3 += 5000;
      sensor.setup(TEMP_IN);
    }
  }

  fanPID.Compute();

  if (cur - prev1 >= WAIT)
  {
    prev1 = cur;
    unsigned long _duration = duration;
    unsigned long _ticks = ticks;
    duration = 0;

    float Freq = (1e6 / float(_duration) * _ticks) / 2;
    speed = Freq * 60;
    ticks = 0;

    if (round(duty) < DUTY_MIN)
    {
      digitalWrite(RELAY, HIGH);
      PWM6 = 0;
      fanRunning = false;
    }
    else
    {
      fanRunning = true;
      PWM6 = duty;
      digitalWrite(RELAY, LOW);
    }

    shouldPrint = true;

    DEBUG(sensor.getStatusString());
    DEBUG(" - Target: ");
    DEBUG(storage.target);
    DEBUG(" - Temp: ");
    DEBUG(ctemp);
    DEBUG(" - Duty: ");
    DEBUG(map(round(duty), 0, 255, 0, 100));
    DEBUG("\n");
  }

  /*
    Checks if the +/- buttons are pressed and if it's not the first time they've been pressed.
  */
  if (up && !lastUp == up && targetMode && storage.target < 255)
  {
    storage.target++;
  }

  if (down && !lastDown == down && targetMode && storage.target > 0)
  {
    storage.target--;
  }

  /*
     If either + or - buttons are pressed, enter target mode and display the current target on screen.
  */
  if (up || down)
  {
    targetMode = true;
    shouldPrint = true;
    prev2 = cur;
  }

  /*
    If 3 secs have elapsed and no button is pressed, exit target mode.
  */
  if (targetMode && cur - prev2 >= 3000)
  {
    targetMode = false;
    shouldPrint = true;

    saveConfig();
  }

  if (shouldPrint)
    printSeg();
}

