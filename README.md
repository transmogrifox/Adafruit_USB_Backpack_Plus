# Adafruit USB Backpack Plus
A fork of @CanyonCasa USB Backpack Plus with noteworthy additional features being an audio level meter and a command to optionally disable config blink on power up.

See https://github.com/CanyonCasa/BackpackPlus

If you are doing your own hacking it is recommended to base your development off the @CanyonCasa repo if the audio level meter is not of interest.  The config blink feature is a trivial thing to add if this is of interest.

## ClearEEPROM
This directory contains a C++ program for erasing the device EEPROM.  The compiled .hex file is also included so building this for the AT90 will not be necessary.

This normally will be a one-time usage.  Enter the ClearEEPROM directory and have this command ready to go on the command line:

```avrdude -P [COM_PORT] -c AVR109 -p usb162 -U flash:w:EEPROM_Eraser.hex```

Short the RESET pin on the USB backpack to ground.   The LCD will go red indicating the bootloader is active.  Quickly hit enter on the command line to issue the avrdude command.  If you don't issue the avrdude command before the USB backpack boots then you will have to reset again and get the command out before the usb backpack leaves the bootloader mode.

Once the USB Backpack has been successfully programmed, then unplug the USB cable and power up the USB backpack.  It will start up and flash for a while during the time it is erasing EEPROM.  Once the activity ceases you can move onto programming the Backpack Plus code.

## BackpackPlus:
This is the directory containing the C++ program that runs on the [AT90USB162](http://www.atmel.com/images/doc7707.pdf).

This file is built from the Teensy 1.0 core code and Arduino liraries which are located in the build/teensyplus and build/libraries directories respectively.  @CanyonCasa organized the build system and created a Makefile (makefile found in the build directory).

If you don't intend to modify and build the code, then the following instructions will suffice:

Enter the Backpack Plus directory and have this command ready to go on the command line:
```avrdude -P [COM_PORT] -c AVR109 -p usb162 -U flash:w:BackpackPlus.hex```

As with the EEPROM erase instructions, short the RESET pin to ground and immediately press enter on your keyboard to flash the BackpackPlus image to your USB backpack.

## build
This directory contains the makefile and libraries necessary to build BackpackPlus/BackpackPlus.cpp.  If you intend to modify and build the program then the following will be useful information.

* Make certain you have gnu-make, avr-gcc, avr-libc and avrdude installed.
    * Instructions for setting up your AVR cross-compiler and build system is not in the scope of this README.
    * If you need help for setting up the build system, quick searching reveals hits on many good quality how-to's and tutorials for AVR development.
    * Below instructions assume you are working from a console (cmd.exe, bash, csh, etc.)
* Modify BackpackPlus/BackpackPlus.cpp as desired.
* Edit the makefile '''PORT = /dev/ttyACM0''' to match the port your USB backpack shows up on (on Windows ```COM3```, for example)
* Enter into the build directory and issue ```make```.
* Have the command ```make burn``` ready to go on the command line.
* As with prior instructions, short RESET pin to ground on the USB backpack.
* Immediately press enter to issue the ```make burn``` command.
* Check that avrdude finishes with success.

**Note** The current revision of the code uses almost 100% of the device so you will need to remove features to add more features.  To make space for the audio level meter I had to eliminate some of @CanyonCasa 's debug features and incrementally add things back until I couldn't go any further.  For example, if you don't need the audio level meter you can get back a significant amount of space for something else by commenting it out or deleting it.



