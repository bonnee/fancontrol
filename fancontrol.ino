#include "libraries/ledcontrol/src/LedControl.h"
#include "libraries/pid/PID_v1.h"

#define THERMO_IN 2
#define FAN_OUT 5
#define LED_OUT_A 10
#define LED_OUT_B 11
#define LED_OUT_C 12

double Kp=1, Ki=0.05, Kd=0.25;

double Temperature, TargetTemp, FanSpeed;

PID fanPid(&Temperature, &FanSpeed, &TargetTemp, Kp, Ki, Kd, DIRECT);

LedControl led=LedControl(LED_OUT_A,LED_OUT_B,LED_OUT_C,1);

// For the delay
int wait = 5000;
unsigned long prev;

void setup() {
        pinMode(THERMO_IN,INPUT);
        pinMode(FAN_OUT,OUTPUT);
        pinMode(LED_OUT_A,OUTPUT);
        pinMode(LED_OUT_B,OUTPUT);
        pinMode(LED_OUT_C,OUTPUT);

        TargetTemp=35;

        /* Exit led display power-saving mode*/
        led.shutdown(0,false);
        /* Set the brightness to a medium values */
        led.setIntensity(0,8);
        /* and clear the display */
        led.clearDisplay(0);


        fanPid.SetMode(AUTOMATIC);

        prev = millis();
}

void printTemp(int v){
        int ones;
        int tens;
        int hundreds;

        boolean negative=false;

        if(v < -999 || v > 999)
                return;
        if(v<0) {
                negative=true;
                v=v* -1;
        }
        ones=v%10;
        v=v/10;
        tens=v%10;
        v=v/10; hundreds=v;
        if(negative) {
                //print character '-' in the leftmost column
                led.setChar(0,3,'-',false);
        }
        else {
                //print a blank in the sign column
                led.setChar(0,3,' ',false);
        }
        //Now print the number digit by digit
        led.setDigit(0,2,(byte)hundreds,false);
        led.setDigit(0,1,(byte)tens,false);
        led.setDigit(0,0,(byte)ones,false);
}

/* Converts a number into bytes */
void getBytes(){

}

void loop() {
        unsigned long cur = millis();
        if (cur - prev >= wait) {
                prev = cur;

                fanPid.Compute();

                digitalWrite(FAN_OUT,FanSpeed);

                printLed(Temperature, false);
                printLed(FanSpeed, true);
        }
}
