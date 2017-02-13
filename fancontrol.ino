/*    DC PWM Fan controller
  This software is meant to drive one or multiple PC fans through an Atmel 32u4 based controller.
  The PWM frequency is set to 23437 Hz that is within the 21k-25k Hz range so it should work with any PC fan.
  For more details on how PWM works on the 32u4, check this link:

  http://r6500.blogspot.it/2014/12/fast-pwm-on-arduino-leonardo.html

  Note that the 32u4 has 2 pins (6 and 13) hooked to Timer4, but since my board doesn't have pin 13 I only configure pin 6.
  A RPM reading system is also featured in this example (although it has proven to be not that accurate, at least on my setup).

  The sketch will:
   Fade the fan down to 15%;
   Keep the fan at 15% for 10 seconds;
   Fade it up to 100%;
   Keep the fan at 100% for 10 seconds;
   Repeat.

  This code has been tested on a SparkFun Pro Micro 16MHz Clone with 4 Arctic F12 PWM PST Fans connected to the same connector.
*/

#include <PID_v1.h>
#include <DHT.h>
#include <LedControl.h>
#include <EEPROM.h>

// Change this if you want your current settings to be overwritten.
#define CONFIG_VERSION "f02"
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

// Debug macro to print messages to serial
#define DEBUG(x)  if(Serial) { Serial.println (x); }

// Tells the amount of time (in ms) to wait between updates
#define WAIT 500

#define DUTY_MIN 25        // The minimum fans speed (0...255)
#define DUTY_DEAD_ZONE 25  // The delta between the minimum output for the PID and DUTY_MIN (DUTY_MIN - DUTY_DEAD_ZONE).

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
double temp;

bool fanRunning = true;

struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  double target;
} storage = { // Default values
  CONFIG_VERSION,
  40
};

PID fanPID(&temp, &duty, &storage.target, 1, 0.5, 1, REVERSE);
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

void loadConfig() {
  // Check if saved bytes have the same "version" and loads them. Otherwise it will load the default values.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t = 0; t < sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
  for (unsigned int t = 0; t < sizeof(storage); t++)
    EEPROM.update(CONFIG_START + t, *((char*)&storage + t));
}

void writeSeg(char string[8]) {
  for (byte i = 0; i < 8; i++) {
    lc.setChar(0, 7 - i, string[i], false);
  }
}


// Routine that updates the display
void printSeg() {
  char buf[8] = "";   // The complete buffer to print
  char tmp[4] = "";   // A temporary buffer to compose the string

  // Values are rounded before printing, avoiding deciamls on the 7-seg
  byte _duty = round(duty);
  int _temp = round(temp);
  int _target = round(storage.target);

  if (targetMode) {   // If +/- have been pressed, show the target temp
    strcat(buf, "Set ");
    sprintf(tmp, "%3uC", _target);
    strcat(buf, tmp);
  } else {
    sprintf(buf, "%3uC", _temp);

    if (isnan(_duty)) {   // Error checking
      strcat(buf, " Err");
    } else if (fanRunning) {
      sprintf(tmp, "%4u", map(_duty, 0, 255, 0, 100));
      strcat(buf, tmp);
    }
    else if (!fanRunning)
      strcat(buf, " 0FF");
  }

  writeSeg(buf);
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
  lc.setIntensity(0, 2);

  writeSeg("Fan ctrl");

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

  if (cur - prev3 >= sensor.getMinimumSamplingPeriod()) {
    if (sensor.getStatus() == 0) {
      prev3 = cur;
      double t = sensor.getTemperature();
      if (!isnan(t))
        temp = t;
    } else {
      prev3 += 5000;
      sensor.setup(TEMP_IN);
    }
  }

  fanPID.Compute();

  if (cur - prev1 >= WAIT) {
    prev1 = cur;
    unsigned long _duration = duration;
    unsigned long _ticks = ticks;
    duration = 0;

    float Freq = (1e6 / float(_duration) * _ticks) / 2;
    speed = Freq * 60;
    ticks = 0;

    if (round(duty) < DUTY_MIN) {
      digitalWrite(RELAY, HIGH);
      PWM6 = 0;
      fanRunning = false;
    }
    else {
      fanRunning = true;
      PWM6 = duty;
      digitalWrite(RELAY, LOW);
    }

    shouldPrint = true;

    char *out = sprintf("%s - Target: %i - Temp: %i - Duty: %i", sensor.getStatusString(), round(storage.target), round(temp), map(duty, 0, 255, 0, 100));

    DEBUG(out);
  }

  /*
    Checks if the +/- buttons are pressed and if it's not the first time they've been pressed.
  */
  if (up && !lastUp == up && targetMode) {
    storage.target++;
  }

  if (down && !lastDown == down && targetMode) {
    storage.target--;
  }

  /*
     If either + or - buttons are pressed, enter target mode and displays the current target.
  */
  if (up || down) {
    targetMode = true;
    shouldPrint = true;
    prev2 = cur;
  }

  /*
    If 3 secs have elapsed and no button is pressed, exits target mode.
  */
  if (targetMode && cur - prev2 >= 3000) {
    targetMode = false;
    shouldPrint = true;

    saveConfig();
  }

  if (shouldPrint)
    printSeg();
}

