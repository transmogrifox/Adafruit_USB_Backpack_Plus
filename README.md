# Adafruit USB Backpack Plus
A fork of CanyonCasa USB Backpack Plus with main additional feature being an audio level meter.

See https://github.com/CanyonCasa/BackpackPlus

##Audio Level Meter:
Allows writing 1 stereo channel / 2 mono channels per row to the LCD as audio level meter.
On a 20x4 meter this is 4 stereo / 8 mono channels.

###Usage
Command: 0xD6 [CHANNEL] [LEVEL]

*CHANNEL: Range is 0 - 7. Values greater than 7 disable the audio level meter.
*LEVEL:   Range is nominally 0 to number of columns (16 on a 16x2 display, 20 on 20x4 display)
** Writing a level greater than number of columns will cause the display to flash RED until it decays below number of columns.
** Writing a level greater than 0x1F is allowed.  Such a level will be clipped to 0x1F.

The audio level meter has an exponential peak decay function built-in so your application
doesn't need to burn cycles on this.  You only need to keep track of a max value over a
certain time interval and send that with the 0xD6 command mapped to the range 0 : nColumns
to avoid clipping indication, or 0 : (nColumns + overdrive).  nColumns is your maximum
dynamic range, and anything above that is considered clipped. The slow response time of the
LCD matrix helps average what would otherwise appear as a "blocky" response.

The duration of the red flash during clipping is dependent upon the amount of overdrive.  If you want
it red longer then continuously write 0x1F as long as you want the clipping indicated.

You can send text at the same time the audio level meter is active.  It will overwrite anything on the rows it is using, but it will leave the other inactive rows alone.

*Example:*
```0xFE 0xD6 0x02 0x0A```  

This will write a level to the peak detector and enable the audio level meter on channel 2, which is the third channel.  Issuing this command once will cause the bottom bar of row 2 to fill up to the 10th element and then decay back to zero.  If it is sent repeatedly, then it will keep bouncing back up from wherever it is at the time the next command is written.

###Additional Notes:

The audio level meter creates custom characters on the fly in spaces 0 through 7.  When it is inactivated it restores custom characters from bank 0 from EEPROM.  If you are using a different custom character bank then you will need to issue the command to specifically load this custom character bank if you are about to send text using custom characters.

Sending text also disables the audio level meter.  If you don't immediately send another audio level to re-activate it the bars will not decay to zero.

If you want to disable the audio level meter and leave the screen blank then send zeros until it decays on its own.  This will take about 1/2 second to go from max to min.

A second option is to disable it and then write all spaces to the lines that were used by the level meter.

This may change as the code is improved, but for now it seems easy enough to leave some of this clean-up-after-yourself stuff to the user.

##Config blink disable option:
A nice "house-keeping feature" is a command to enable or disable baud / config splash on startup.

Command: 0x43, one argument 0x00 or 0x01

*Example:*

```0xFE 0x43 0x00```  (default) Enables the display of config data on power-up.  For this to be guaranteed default assumes EEPROM has been erased before you startup the new image the first time.

```0xFE 0x43 0x01```  Disables the display of config data on power-up.


This feature is useful for certain projects where you want to finalize a design implementation in which blinking of the configuration information detracts from the user experience.
