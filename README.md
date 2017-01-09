# Fancontrol.ino
Arduino PWM DC fan control

This project I made is meant to control the temperature of a network/server rack by using 4 ceiling-mounted temperature-driven BLDC 12V PWM Fans using an Arduino board.

## 1. The fans

4 [Arctic F12 PWM PST fans](https://www.arctic.ac/eu_en/f12-pwm-pst.html) are used because they're cheap but good quality, horizontally-mountable, 4 pin PWM and PST wich means that every fan has an extra female connector to hook up another fan so that I can decrease the PCB footprint by using only one fan connector.

## 2. The temperature sensor

I went with an DHT22 - AM2302 Temperature/Umidity sensor because is cheap and simple to program.

## 3. The 7 segment display

I wanted to give the box a fancy look so I found a MAX7219-based 8-digit 7-segment display that should be able to show the current temperature and the fans percentage.

## 4. The controller

I chose a cheap and tiny [SparkFun Pro Micro](https://www.sparkfun.com/products/12640) clone (not SparkFun branded) that I found for about 15€ on Amazon. This board is basically a stripped-down Arduino Leonardo featuring an Atmel 32u4 microcontroller that fits all my needs.

The system is mounted in a custom box, hooked to the ceiling of the rack, blowing hot air out.
