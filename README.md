RPi Pico version of iBus decoder for brushed motor driven vehicles and aircraft

This code is written for the RPi RP2040 microcontroller typically packaged as "RP2040-zero" tiny dev boards from china (with USB-C connector etc. It takes iBus input from a suitable RC receiver (flysky etc) and outputs servo, motor and switch signals as configured in the code.

Motor control outputs a PWM signal between 0 and 100% duty cycle, at about 10khz, for driving most DC brushed motors via a motor driver chip (like TB6612FNG etc) or simple mosfet if you're building a tiny airoplane.
It's a modernised version of my previous iBus decoder project originally written for Arduino Nano (Atmel ATMEGA328P) but now running on the much more capable RP2040.
