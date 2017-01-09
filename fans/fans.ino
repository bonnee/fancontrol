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

#include <EEPROM.h>

#define CONFIG_VERSION "fan"
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

byte SpdIn = 7;

volatile unsigned long duration = 0; // accumulates pulse width
volatile unsigned int pulsecount = 0;
volatile unsigned long previousMicros = 0;

int ticks = 0, Speed = 0;

long wait = 1e6;
unsigned long prev1, prev2;

void pickrpm ()
{
  unsigned long currentMicros = micros();
  if (currentMicros - previousMicros > 20000) {
    duration += currentMicros - previousMicros;
    previousMicros = currentMicros;
    ticks++;
  }
}

struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  byte value;
  bool rising;
  int stayCount;
} storage = {
  CONFIG_VERSION,
  // The default values
  50,
  false,
  -1
};

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

void setup()
{
  pinMode(SpdIn, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SpdIn), pickrpm, FALLING);

  // Configure Timer 4 (Pin 6)
  pwm6configure();

  // Prepare pin 6 to use PWM
  // We need to call pwm6configure
  // For now, we set it at 0%
  pwmSet6(0);

  loadConfig();

  Serial.begin(19200);
  //while (!Serial) {}
  Serial.println("Fans...Ready.\n\n");
  prev1, prev2 = micros();
}

void loop()
{
  unsigned long cur = micros();

  if (cur - prev1 >= wait) {
    unsigned long _duration = duration;
    unsigned long _ticks = ticks;
    duration = 0;


    float Freq = (1e6 / float(_duration) * _ticks) / 2;
    Speed = Freq * 60;

    ticks = 0;

    Serial.print(storage.value, DEC);
    Serial.print("% - ");
    Serial.print (Speed, DEC);
    Serial.println(" RPM");

    prev1 = cur;
    //interrupts();
  }

  if (cur - prev2 >= wait / 2) {
    prev2 = cur;

    if (storage.stayCount == -1) {
      if (storage.value < 100 && storage.rising)
        storage.value++;
      else if (storage.value > 15 && !storage.rising)
        storage.value--;
    }
    if (storage.value == 100 || storage.value == 15) {
      storage.rising = !storage.rising;
      if (storage.stayCount == 19)
        storage.stayCount = -1;
      else
        storage.stayCount++;
    }

    saveConfig();

    PWM6 = DUTY2PWM(storage.value); //map(value, 0, 100, 0, 255);
  }
}

