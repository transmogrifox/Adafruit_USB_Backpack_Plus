# Adafruit USB Backpack Plus
A somewhat cleaned-up fork of CanyonCasa USB Backpack Plus

See https://github.com/CanyonCasa/BackpackPlus

Main feature added is a command to enable or disable baud/config splash on startup.
Command: 0x43, one argument 0x00 or 0x01
Example:
0xFE 0x43 0x00  Enables the baud rate display.  This will be the default if you zero EEPROM before you flash the image.
0xFE 0x43 0x01  Disables display of config data on startup.

This feature is useful for certain projects where you want to finalize the design implementation.  A beautification step is to disable this feature so this information isn't displayed when you no longer need it.
