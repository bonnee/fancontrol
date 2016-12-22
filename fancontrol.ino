#include "libraries/ledcontrol/src/LedControl.h"
#include "libraries/pid/PID_v1.h"

#define THERMO_IN 2
#define FAN_OUT 5
#define SCREEN_OUT_A 10
#define SCREEN_OUT_B 11

double Kp=1, Ki=0.05, Kd=0.25;

double Temperature, TargetTemp, FanSpeed;

PID pid(&Temperature, &FanSPeed, &TargetTemp, Kp, Ki, Kd, DIRECT);

void setup() {
        pinMode(THERMO_IN,INPUT);
        pinMode(FAN_OUT,OUTPUT);
        pinMode(SCREEN_OUT_A,OUTPUT);
        pinMode(SCREEN_OUT_B,OUTPUT);

        TargetTemp=35;

        pid.SetMode(AUTOMATIC);
}

void loop() {


}
