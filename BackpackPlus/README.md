Adafruit USB Serial LCD with IO and Other Enhancements
======================================================

* KEYWORDS: Adafruit, USB, serial, LCD, I/O

## Summary

This code is a complete rewrite of the Adafruit RGB Backpack code. This code adds GPIO, specifically input capability to the Backpack functionality. This allows the LCD to function as a simple remote terminal with up to 4 "key/button" inputs and/or "LED indicators" able to communicate over a simple 2 wire serial interface. The code also incorporates a number of other enhancements detailed below and in CanyonCasa's [README](https://github.com/CanyonCasa/BackpackPlus/blob/master/BackpackPlus/BACKPACK_PLUS_README.md).

This was forked from 
[CanyonCasa's Backpack Plus](https://github.com/CanyonCasa/BackpackPlus)
. Follow the link for the full description.  Below has been abbreviated to documentation of code changes upon CanyonCasa's work and a re-iteration of the command summary table (since this is convenient to have all in one place).  

## Code Changes Overview Notes

This following describe the basic aspects of code changes:

* Extended command option to disable 'blink' message.
* Python support scripts with encoded text string support. See [CanyonCasa Repository](https://github.com/CanyonCasa/BackpackPlus/tree/master/PythonScripts) for Python scripts.
* Improvement to switch debounce routine, including re-transmit timer so there aren't a barrage of messages from a shaky finger or a particularly bad switch.

## Functional Code Changes
**Blink Message Disable**  When a project development is complete and it is time to deploy it for user interaction the blink message detracts from the overall user experience.  This fork improves upon the CanyonCasa by providing a command for the user decide if this should be displayed (but it is displayed by default if EEPROM has been erased).

### Usage Cases:

```0xFE 0x43 0x00``` Don't disable the blink message (display it on startup).

```0xFE 0x43 0x01``` Disable the blink message.  In this mode it skips straight to the splash screen.  

#### Related Notes:

The ability to overwrite the splash screen with spaces in the original Adafruit code is intact.  CanyonCasa's revision allows splash delay to be changed so the startup time can also be shortened when splash is not used.

With this current fork it is now possible to start up with a blank screen and nothing is displayed until the user specifically sends something to be displayed.

**Switch Debounce Improvements** This was re-implemented with a variation on a method presented by [Jack Ganssle](http://www.eng.utah.edu/~cs5780/debouncing.pdf), from the heading "**Handling Multiple Inputs**" (*Listing 3*).  The main deviation is to set a timer to avoid re-transmitting the switch condition rapidly on a shakey finger or accidentally pressing 2 buttons nearly simultaneously.  Also from the data Ganssle presented it was evident that only 4 records of the same state on 3ms sampling intervals are likely needed to cover the majority of bounce cases while providing acceptable EMI immunity. This is particularly true as the re-transmit timer provides a function not unlike the latch Ganssle presented in *Figure1: The SR Debouncer*.

My test for behavior was tapping and sliding a wire on and around the switch input pads, and inserting into the hole and wiggling it back and forth.  There was not excessive traffic on the serial port as every transmission represented a valid change of state.  Perhaps in a future revision a command can be added to change the retransmit delay.

## Other Backpack Issues (not fixed)

**TX and RX Lines** [HARDWARE]: Note, pin 3 of CN3 is incorrectly labeled on the board silkscreen as TX when it actually (correctly) connects to RX. This may be confusing when wiring to separate TX and RX pins, which are labeled correctly.

Note from Transmogrifox:  The schematic labels these correctly so this appears as a mismatch between PCB silkscreen text and net names.  It's hard to guess whether this was intentional (one interface considered DTE and the other a DCE) or unintentional.  At the very least the documentation for the product should identify this and define it.

### Command Summary Table

STATUS | SCRIPT NAME | CODE | DESCRIPTION
--- | --- | --- | ---
ALL | SetBaud | 0x39 | Set serial baud rate and writes to EEPROM.
AF | ChangeSplash | 0x40 | Writes splash screen text to EEPROM.
NEW | SaveSplash | 0x40 | Save the display screen to EEPROM as splash screen.
NEW | SplashDelay | 0x41 | Save and save splash screen display time, ~0.1s units.
ALL | DisplayOnTimed | 0x42 | Same as Adafruit DisplayOn command, but supports timeout value.
NEW | DisableConfigBlink | 0x43 | Display Config Data (0x00) or skip straight to start-up splash (0x01).
NEW | DisplayOn | 0x45 | Turn display on (indefinately) with no timeout.
ALL | DisplayOff | 0x46 | Turn display off.
ALL | SetCursor | 0x47 | Move cursor to (COL,ROW).
ALL | GoHome | 0x48 | Move cursor to home position (1,1).
ALL | UnderlineCursorOn | 0x4A | Turn on the underline cursor.
ALL | UnderlineCursorOff | 0x4B | Turn off the underline cursor.
ALL | CursorBack | 0x4C | Move cursor back one space, wrap to end.
ALL | CursorForward | 0x4D | Move cursor forward one space, wrap to beginning.
ALL | CreateCustomCharacter | 0x4E | Create custom character #0-7 with 8 bytes of data direct to character RAM.
CHG | SetContrast | 0x50 | Set backlight color, not saved to EEPROM. Use SaveContrast to write EEPROM.
ALL | AutoscrollOn | 0x51 | Scroll display lines, where new lines appear at bottom.
ALL | AutoscrollOff | 0x52 | Do NOT scroll display, where new lines overwrite at top.
ALL | BlockCursorOn | 0x53 | Turn on the Block cursor.
ALL | BlockCursorOff | 0x54 | Turn off the Block cursor.
NEW | GPIOReadSaveMask | 0x55 | Save the GPIO mask and read the input bits.
ALL | GPIOOff | 0x56 | Set the general purpose pin 1-4 to LOW (0V).
ALL | GPIOOn | 0x57 | Set the general purpose pin 1-4 to HIGH (5V)
ALL | ClearScreen | 0x58 | Clear text on display
NEW | GPIORead | 0x59 | Read the input bits with the saved mask.
ALL | SaveContrast | 0x91 | Set backlight contrast and save to EEPROM.
ALL | SaveBrightness | 0x98 | Set backlight brightness and save to EEPROM.
CHG | SetBrightness | 0x99 | Set backlight brightness, not saved to EEPROM. Use SaveBrightness to write EEPROM.
ALL | LoadCustomCharacters | 0xC0 | Load characters from EEPROM bank, 0-3 into character RAM.
ALL | SaveCustomCharacter | 0xC1 | Create custom character #0-7 with 8 bytes of data saved to EEPROM bank 0-3.
ALL | GPIOStartState | 0xC3 | Sets the "initial" state of the GPIO pin, default "input pullup".
CHG | SaveRGB | 0xD0 | Sets the backlight red, green, and blue levels (0-255) and saves to EEPROM. Same as Adafruit Set Backlight Color.
ALL | SaveSize | 0xD1 | Set display size up to 20x4, saved to EEPROM. Only needed once.
AF | Testbaud | 0xD2 | Test non standard baudrate. (Adafruit only, not supported).
NEW | SaveScrollMode | 0xD3 | Extended scroll modes, saved to EEPROM.
NEW | ScaleRGB | 0xD4 | Scales the maximum LED intensities red, green, and blue levels (0-255, Adafruit: 100, 190, 255).
NEW | SetRGB | 0xD5 | Sets the backlight red, green, and blue levels (0-255), not saved to EEPROM.
NEW | DEBUG | 0xDC | Controls various debug data.
NEW | DumpEEPROM | 0xDD | Dumps/reads (4-byte) pages of EEPROM.
NEW | EditEEPROM | 0xDE | Edits/writes (save) (4-byte) pages of EEPROM.
NEW | TEST | 0xDF | Reserved for new code development testing.
