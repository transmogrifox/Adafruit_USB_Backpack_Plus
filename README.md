# Adafruit USB Backpack Plus
A somewhat cleaned-up fork of CanyonCasa USB Backpack Plus

See https://github.com/CanyonCasa/BackpackPlus

Main feature added is a command to enable or disable baud / config splash on startup.

Command: 0x43, one argument 0x00 or 0x01

Example:

```0xFE 0x43 0x00```  (default) Enables the display of config data on power-up.  For this to be guaranteed default assumes EEPROM has been erased before you startup the new image the first time.

```0xFE 0x43 0x01```  Disables the display of config data on power-up.


This feature is useful for certain projects where you want to finalize a design implementation in which blinking of the configuration information detracts from the user experience.
