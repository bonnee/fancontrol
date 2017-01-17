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

  This code has been tested on a SparkFun Pro Micro 16MHz with up to 4 Arctic F12 PWM PST Fans connected to the same pin, and it works just fine.
*/

#include <PID_v1.h>
#include <LedControl.h>
#include <EEPROM.h>

#define CONFIG_VERSION "f02"
// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

// Pin 6 shortcut
#define PWM6        OCR4D

// Terminal count
#define PWM6_MAX OCR4C

/* Configure the PWM clock*/
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
  OCR4D = value; // Set PWM value
  DDRD |= 1 << 7; // Set Output Mode D7
  TCCR4C |= 0x09; // Activate channel D
}

/*************** ADDITIONAL DEFINITIONS ******************/

// Macro to converts from duty (0..100) to PWM (0..255)
#define DUTY2PWM(x)  ((255*(x))/100)

/**********************************************************/

struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  double target;
} storage = {
  CONFIG_VERSION,
  // The default values
  40
};


/*        Pinouts       */
byte SpdIn = 7;                             // Hall sensor reading pinout
byte segDin = 15, segClk = 16, segCs = 14;  // 7-segment display pinout
byte relay = 4;                             // Relay input pin

byte tempIn = 4;                            // Temperature sensor pinout
byte targetUp = 2, targetDown = 3;        // Up/Down buttons pinout
bool targetMode = false;
bool lastUp = false, lastDown = false;
bool up, down = false;

volatile unsigned long duration = 0; // accumulates pulse width
volatile unsigned int pulsecount = 0;
volatile unsigned long previousMicros = 0;
int ticks = 0, speed = 0;

long wait = 1e6;  // Time to wait between updates. In micros
unsigned long prev1, prev2 = 0; // Timer placeholders

double duty;
double temp = 40;

byte minDuty = 25;

bool fanRunning = true;

PID fanPID(&temp, &duty, &storage.target, 4, 1, 0.2, REVERSE);
LedControl lc = LedControl(segDin, segClk, segCs, 1);

//  Called when hall sensor pulses
void pickrpm ()
{
  volatile unsigned long currentMicros = micros();

  if (currentMicros - previousMicros > 20000) {   // Prevent pulses less than 20k micros far.
    duration += currentMicros - previousMicros;
    previousMicros = currentMicros;
    ticks++;
  }
}

void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
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

// Routine that updates the display
void printSeg() {
  char buf[8] = "";
  char tmp[4] = "";

  Serial.println(targetMode);

  if (targetMode) {
    strcat(buf, "Set ");
    sprintf(tmp, "%3uC", round(storage.target));
    strcat(buf, tmp);
  } else {
    sprintf(buf, "%3uC", (int)temp);

    if (fanRunning) {
      sprintf(tmp, "%4u", map(duty, 0, 255, 0, 100));
      strcat(buf, tmp);
    }
    else
      strcat(buf, " 0FF");
  }

  //Serial.println(buf);

  writeSeg(buf);
}

void writeSeg(char string[8]) {
  for (byte i = 0; i < 8; i++) {
    lc.setChar(0, 7 - i, string[i], false);
  }
}

void setup()
{
  pinMode(SpdIn, INPUT);
  pinMode(relay, OUTPUT);
  pinMode(targetUp, INPUT_PULLUP);
  pinMode(targetDown, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SpdIn), pickrpm, FALLING);

  // Configure Timer 4 (Pin 6)
  pwm6configure();

  loadConfig();

  fanPID.SetSampleTime(1000);
  fanPID.SetOutputLimits(minDuty - 1, 255);
  fanPID.SetMode(AUTOMATIC);

  lc.shutdown(0, false);
  /* Set the brightness */
  lc.setIntensity(0, 1);
  /* and clear the display */
  lc.clearDisplay(0);

  Serial.begin(19200);
  //while (!Serial) {}
  Serial.print("Fans...");
  pwmSet6(255);
  writeSeg("Fan ctrl");
  delay(5000);
  Serial.println("Ready\n\n");

  prev1 = micros();
}

void loop()
{
  unsigned long cur = micros();
  bool shouldPrint = false;

  lastUp = up;
  lastDown = down;
  up = !digitalRead(targetUp);
  down = !digitalRead(targetDown);

  fanPID.Compute();

  if (cur - prev1 >= wait) {
    prev1 = cur;
    unsigned long _duration = duration;
    unsigned long _ticks = ticks;
    duration = 0;

    float Freq = (1e6 / float(_duration) * _ticks) / 2;
    Serial.println(_ticks);
    speed = Freq * 60;
    ticks = 0;

    if (round(duty) < minDuty) {
      digitalWrite(relay, HIGH);
      PWM6 = 0;
      fanRunning = false;
    }
    else {
      fanRunning = true;
      PWM6 = duty;
      digitalWrite(relay, LOW);
    }

    shouldPrint = true;

    char prnt[] = "";
    if (Serial)
      sprintf(prnt, "Target %uC - Temp %uC - Duty %u", (int)storage.target, (int)temp, round(duty));
    Serial.println(prnt);
  }

  if (up && !lastUp == up && targetMode) {
    storage.target++;
  }

  if (down && !lastDown == down && targetMode) {
    storage.target--;
  }

  if (targetMode && cur - prev2 >= 3e6) {
    targetMode = false;
    shouldPrint = true;
  }

  if (up || down) {
    targetMode = true;
    shouldPrint = true;
    prev2 = micros();
  }

  if (Serial.available() > 0)
    temp = Serial.parseFloat();

  if (shouldPrint)
    printSeg();
}

