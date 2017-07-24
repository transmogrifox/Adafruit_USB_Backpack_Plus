# Adafruit USB Backpack Plus
A fork of CanyonCasa USB Backpack Plus with main additional feature being an audio level meter.

See https://github.com/CanyonCasa/BackpackPlus

## Audio Level Meter:
Allows writing 1 stereo channel / 2 mono channels per row to the LCD as audio level meter.
On a 20x4 meter this is 4 stereo / 8 mono channels.

### Usage
Command: 0xD6 [CHANNEL] [LEVEL]

* CHANNEL: Range is 0 - 7. 
* DISABLE: Write audio level meter with a channel greater than 7 to disable the audio level meter.
* LEVEL:   Range is 0 to 255.
  * NORMAL: The nominal input range is 0 to number of columns (16 on a 16x2 display, 20 on 20x4 display)
  * CLIPPING INDICATION: Writing a level greater than number of columns will cause the display to flash RED until it decays below number of columns.  More overdrive increases the amount of time that clipping indication persists.  
    * Severe clipping will result in an alarming (attention-grabbing) red flickering.  
    * Mild clipping will appear as a glitch too short to even perceive RED.  If perception of red is desired on all clipping than the user-side commands will need to add more overdrive to anything intended to indicate clipping.
  * MAXIMUM MEANINGFUL LEVEL: Writing a level greater than 0x1F will be clipped to 0x1F. Numbers above 0x1F are handled gracefully, but this will not cause clipping indication to persist for a longer duration than 0x1F.

The audio level meter has an exponential peak decay function built-in so your application
doesn't need to burn cycles on this.  You only need to keep track of a max value over a
certain time interval and send that level mapped into the range 0 : nColumns
to avoid clipping indication, or 0 : (nColumns + overdrive).  

The slow response time of the LCD matrix helps average what would otherwise appear as a "blocky" response.  The LCD "ghosting" part of the visual effect and has been considered in the timing of the meter's decay rate.

You can send text at the same time the audio level meter is active.  It will overwrite anything on the rows it is using, but it will leave the other inactive rows alone. Each time through its loop it saves and restores the cursor position. 

*Example:*
```0xFE 0xD6 0x02 0x0A```

This will write a level to the peak detector and enable the audio level meter on channel 2, which is the third channel.  Issuing this command once will cause the bottom bar of row 2 to fill up to the 11th element and then decay back to zero.  If it is sent repeatedly, then it will keep bouncing back up from wherever it is at the time the next command is written.

### Additional Notes:

The audio level meter creates custom characters on the fly in spaces 0 through 2.  When it is inactivated it restores custom characters from bank 0 from EEPROM.  If you are using a different custom character bank then you will need to issue the command to specifically load this custom character bank before you send text using custom characters from a bank other than bank 0.

Sending text disables the audio level meter. If you send a level to the meter, then send text, but don't immediately follow up with another level then the meter will appear "paused".  If you immediately follow up text with another level to the meter then the interruption will go un-noticed.

If you want to disable the audio level meter and leave the screen blank there are a few ways to do this, depending on the effect you want:
* Cease updating the level meter long enough for all to active channels to decay to zero. This will take about 1/2 second to go from max to min. After the delay send text or an audio level meter command with an out-of-range channel number to disable it.
* Disable the audio meter and then write all spaces to the lines that were used by the level meter.
* Disable the audio level meter, send the clear screen command and optionally move cursor position to home.

This may change as the code is improved, but for now it seems easy enough to leave some of this clean-up-after-yourself stuff to the user.

## Config blink disable option:
A nice "house-keeping feature" is a command to enable or disable baud / config splash on startup.

Command: 0x43, one argument 0x00 or 0x01

*Example:*

```0xFE 0x43 0x00```  (default) Enables the display of config data on power-up.  For this to be guaranteed default assumes EEPROM has been erased before you startup the new image the first time.

```0xFE 0x43 0x01```  Disables the display of config data on power-up.


This feature is useful for certain projects where you want to finalize a design implementation in which blinking of the configuration information detracts from the user experience.
