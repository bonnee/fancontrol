// Frequency modes for TIMER4
#define PWM23k  4   //  23437 Hz

// Direct PWM change variables
#define PWM6        OCR4D

// Terminal count
#define PWM6_13_MAX OCR4C

// Configure the PWM clock
// The argument is one of the 7 previously defined modes
void pwm613configure(int mode)
{
  // TCCR4A configuration
  TCCR4A = 0;

  // TCCR4B configuration
  TCCR4B = mode;

  // TCCR4C configuration
  TCCR4C = 0;

  // TCCR4D configuration
  TCCR4D = 0;

  // TCCR4D configuration
  TCCR4D = 0;

  // PLL Configuration
  // Use 96MHz / 2 = 48MHz
  PLLFRQ = (PLLFRQ & 0xCF) | 0x30;
  // PLLFRQ=(PLLFRQ&0xCF)|0x10; // Will double all frequencies

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

byte SpdIn=7;

int ticks = 0, Speed = 0;

long wait = 1000000;
unsigned long prev1, prev2;

void pickrpm ()
{
  ticks++;
}

void setup()
{
  pinMode(SpdIn, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SpdIn), pickrpm, FALLING);

  // Configure Timer 4 (Pin 6)
  // It will operate at 23kHz
  pwm613configure(PWM23k);
  
  // Prepare pin 6 to use PWM
  // We need to call pwm613configure
  // For now, we set it at 0%
  pwmSet6(0);

  Serial.begin(19200);
  //while (!Serial) {}
  Serial.println("Fans...Ready.\n\n");
  prev1, prev2 = micros();
}

int value = 50;
bool rising = false;

int stayCount = -1;

void loop()
{
  unsigned long cur = micros();

  if (cur - prev1 >= wait) {
    noInterrupts();
    Speed = (ticks / 2) * (60 + ((float)cur - prev1 - wait) / 1000000);

    ticks = 0;

    Serial.print(value, DEC);
    Serial.print("% - ");
    Serial.print (Speed, DEC);
    Serial.println(" RPM");

    prev1 = cur;
    interrupts();
  }

  if (cur - prev2 >= wait) {
    prev2 = cur;

    if (stayCount == -1) {
      if (value < 100 && rising)
        value++;
      else if (value > 15 && !rising)
        value--;
    }
    if (value == 100 || value == 15) {
      rising = !rising;
      if (stayCount == 9)
        stayCount = -1;
      else
        stayCount++;

    }

    PWM6 = DUTY2PWM(value); //map(value, 0, 100, 0, 255);
  }
}

