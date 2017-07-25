/***********************************
Complete rewrite by CanyonCasa of Enchanted Engineering
to provide I/O and a number of other enhancements.
See BACKPACK_PLUS_README.md for details

Code formatting and addition of config splash enable/disable function
by The Transmogrifox, 2017.

Derived from

Matrix-orbitalish compatible LCD driver with USB and Serial
For use with Teensy 1.0 core on AT90USB162 chips

---> http://www.adafruit.com/category/63_96

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/

// NOTE: Compiler reports 15,872 (0x3E00) bytes available, but loader fails for >12,288 (0x3000)

// external libraries...
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <util/delay.h>
#include <stdarg.h>

// code parameters
#define VERSION                     "+17G1"          // '+'+YEAR+ALPHA_MONTH+VERSION
#define RELATIVE                    true
#define ABSOLUTE                    false
#define SCROLLBEFORE                0
#define SCROLLAUTO                  1
#define SCROLLAFTER                 2
#define SCROLLMODE                  2
#define EEPROM_PAGE_SIZE            4               // EEPROM page size used by EXTENDED_DUMP_EEPROM and EXTENDED_EDIT_EEPROM commands
#define EEPROM_SIZE                 512             // EEPROM size
#define MAXCOLS                     20              // MAXIMUM DISPLAY SIZE, NOTE: AT90USB162 could handle 40x4 displays
#define MAXROWS                     4
#define DEFAULTCOLS                 16              // DEFAULT DISPLAY SIZE
#define DEFAULTROWS                 2
#define MAXCHARS                    MAXCOLS*MAXROWS // 20*4=80 bytes
#define CUSTOM_CHARACTER_BANKS      4               // 4*64=256 bytes
#define SERIAL_TIMEOUT              255             // Serial blocking read timeout milliseconds
#define USB                         Serial          // Rename Serial for code clarity
#define UART                        Serial1         // Hardware Serial already defines Serial1 instance
#define DEBUGTERM                   Serial          // Select which terminal used for debug
#define CR                          13              // Carriage Return character
#define LF                          10              // Line Feed character
#define BS                          8               // Backspace character
#define LOOPDELAY                   10              // loop delay in milliseconds, debounce and display on time
#define delayMS                     _delay_ms
// Debug flags...
#define ECHOCHARS                   1               // echo received characters
#define DUMPENABLE                  4               // enable debug dump of virtual display data
#define DUMPVD                      64              // Dump virtual display data immediately

// I/O pin declarations
// LCD I/O pins
#define D4                          1               // PD1
#define D5                          4               // PD4
#define D6                          5               // PD5
#define D7                          6               // PD6
#define RS                          12              // PB4
#define RW                          13              // PB5
#define EN                          14              // PB6
// display backlight controls - analog output (PWM) pins!
#define REDLITE                     0               // D0
#define GREENLITE                   18              // C5
#define BLUELITE                    17              // C6
#define CONTRASTPIN                 15              // B7
// GPIO pins 1-4, PB0 PC2 PC4 PC7, respectively, values PB0 PC2 PC4 PC7 already defined
#define GPIO0                       8
#define GPIO1                       20
#define GPIO2                       19
#define GPIO3                       16

// matrix orbital commands (we dont support -all-) see a list here...
//http://www.matrixorbital.ca/manuals/LCDVFD_Series/LCD2041/LCD2041.pdf
#define MATRIX_BAUDRATE             0x39            // 1 arg: baudrate
#define MATRIX_CHANGESPLASH         0x40            // writes virtual display to EEPROM
#define MATRIX_SPLASH_DELAY         0x41            // 1 arg: delay in ~1/4 seconds
#define MATRIX_DISPLAY_ON_TIMED     0x42            // 1 argument minutes
#define MATRIX_DISPLAY_ON           0x45
#define MATRIX_DISPLAY_OFF          0x46
#define MATRIX_SETCURSOR_POSITION   0x47            // 2 args: col, row
#define MATRIX_HOME                 0x48
#define MATRIX_UNDERLINECURSOR_ON   0x4A
#define MATRIX_UNDERLINECURSOR_OFF  0x4B
#define MATRIX_MOVECURSOR_BACK      0x4C
#define MATRIX_MOVECURSOR_FORWARD   0x4D
#define MATRIX_CUSTOM_CHARACTER     0x4E            // 9 args: char #, 8 bytes data
#define MATRIX_SET_CONTRAST         0x50            // 1 arg: scale
#define MATRIX_AUTOSCROLL_ON        0x51
#define MATRIX_AUTOSCROLL_OFF       0x52
#define MATRIX_BLOCKCURSOR_ON       0x53
#define MATRIX_BLOCKCURSOR_OFF      0x54
#define MATRIX_GPIO_READANDMASK     0x55            // 1 arg: mask; low nibble ands to select bits, high nibble XORs with low result
#define MATRIX_GPIO_OFF             0x56            // 1 arg: pin
#define MATRIX_GPIO_ON              0x57            // 1 arg: pin
#define MATRIX_CLEAR                0x58
#define MATRIX_GPIO_READ            0x59
#define MATRIX_SETSAVE_CONTRAST     0x91            // 1 arg: scale
#define MATRIX_SETSAVE_BRIGHTNESS   0x98            // 1 arg: scale
#define MATRIX_SET_BRIGHTNESS       0x99            // 1 arg: scale
#define MATRIX_LOADCUSTOMCHARBANK   0xC0            // 1 arg: bank
#define MATRIX_SAVECUSTOMCHAR       0xC1            // 10 args: bank, char #, 8 bytes data
#define MATRIX_GPIO_START_STATE     0xC3            // 2 args: pin, state
#define EXTENDED_DISABLE_BAUD_SPLSH 0x43            // 1 arg: (0) Display baud rate on startup, (1) disable baud rate display
#define EXTENDED_AUDIO_LEVEL_METER  0xD6            // 2 args: [Channel 0-7, or >7 disabled] [Level (0 to ROWS)]
                                                    // Level > ROWS flashes backlight red (clipping) but restores orig color scheme.
#define EXTENDED_RGBBACKLIGHTSAVE   0xD0            // 3 args: R G B
#define EXTENDED_SETSIZE            0xD1            // 2 args: Cols, Rows
#define EXTENDED_TESTBAUD           0xD2            // Adafruit Backpack
#define EXTENDED_SCROLLMODE         0xD3            // 1 arg: mode, 0: off, 1: on before, 2: off, 3: on after
#define EXTENDED_SCALERGBBACKLIGHT  0xD4            // 3 args: R G B
#define EXTENDED_RGBBACKLIGHT       0xD5            // 3 args: R G B
#define EXTENDED_DUMP_EEPROM        0xDD            // 2 args: start_page, end_page;
#define EXTENDED_EDIT_EEPROM        0xDE            // 5 args: page#, byte1, byte2, byte3, byte4
#define EXTENDED_CODE_TEST          0xDF            // reserved for code testing
#define MATRIX_COMMAND_PREFIX       0xFE            // prefix for all commands

// EEPROM storage of the current state - unset EEPROM cells reads as 255
#define COLS_ADDR                   0
#define ROWS_ADDR                   1
#define CONTRAST_ADDR               2
#define BACKLIGHT_BRIGHTNESS_ADDR   3
#define BACKLIGHT_R_ADDR            4
#define BACKLIGHT_G_ADDR            5
#define BACKLIGHT_B_ADDR            6
#define SCROLLMODE_ADDR             7
#define BAUDRATE_ADDR               8               // and 9, 10
#define GPIO_MASK_ADDR              11
#define GPIO_START_STATE_ADDR       12              // and 13 14 15
#define SCALERED_ADDR               16
#define SCALEGREEN_ADDR             17
#define SCALEBLUE_ADDR              18
#define SPLASH_DELAY_ADDR           19              // units: 0.1 seconds
#define BAUD_SPLASH_DISABLE         20

// open 17-175
// space for MAXCHARS (80) splash screen characters
#define SPLASH_ADDR EEPROM_SIZE-(64*CUSTOM_CHARACTER_BANKS)-MAXCHARS
// 256 bytes to end or EEPROM for CUSTOM_CHARACTER_BANKS (4) worth of characters!
#define CUSTOMCHARBANKS_ADDR EEPROM_SIZE-(64*CUSTOM_CHARACTER_BANKS)

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

// Arduino .ino script define and prototype declaration insertions
#include "Arduino.h"
void setup();
void loop();
void parseCommand();
void dumpEEPROM(uint8_t firstPage, uint8_t lastPage);
void loadCustomCharBank(uint8_t bank);
void defineCustomChar(uint8_t loc, uint8_t bank);
uint32_t getBaud();
void setBaud(uint32_t baudrate);
int serialAvailable();
uint8_t serialBlockingRead();
uint8_t serialBlockingRead();
void setSize(uint8_t c, uint8_t r);
void display(uint8_t state);
void virtualClear();
void virtualCursorPosition(int8_t col, int8_t row, bool positioning);
void virtualScroll(uint8_t mode);
void virtualWrite(char ch);
//void dumpVirtualDisplay(uint8_t dump);
void gpioPinMode(uint8_t bit, uint8_t mode);
uint8_t gpioRead();
void gpioSend(uint8_t p);
void eeSave(uint16_t addr, uint8_t value);

// variables declaration...
LiquidCrystal lcd(RS, RW, EN, D4, D5, D6, D7);    // LCD interface
// general purpose i/o data
uint8_t GPIO[] = {GPIO0, GPIO1, GPIO2, GPIO3};    // GPIO bits
uint8_t gpioPort = 0;                             // last GPIO pin states
uint8_t gpioMask = 0xFF;                          // read mask default
uint8_t gpioDebounceBuf[4];                       // Circular buf
uint8_t gpioDBBfIndex = 0;                        // Circular buf index
// display definition...
uint8_t COLS = EEPROM.read(COLS_ADDR);
uint8_t ROWS = EEPROM.read(ROWS_ADDR);
// create a virtual display that mimics LCD...
// Enables scrolling, backspace, splash screen save, debug dump
// calls to virtual display change both the virtual display content and the LCD
// while calls direct to lcd will not track virtual display
// only handles calls that effect displayed text
char virtualDisplay[MAXROWS][MAXCOLS];  // virtual display made maxsize of 40x4
// NOTE: display cursor positions zero-based, whereas COLS, ROWS are 1-based!!!
// organizing by [row][col] vs [col][rows] facilitates printing
uint8_t vX, vY;                   // virtual cursor position
uint8_t vLastNL=0;                // last character, used in handling new lines
uint8_t vScroll = 0;              // set when overflow occurs with autoscrolling set ON
// hold display parameters in RAM to reduce EEPROM wear, defaults defined here
uint8_t red = 0xFF;
uint8_t green = 0xFF;
uint8_t blue = 0xFF;
uint8_t brightness = 0xFF;
uint8_t contrast = 0xC0;
uint16_t onTime = 0;  // seconds
elapsedMillis since = 0;
elapsedMillis dbounce_timer = 0;
uint8_t retransmit_timer = 0;
uint8_t cmdFlags = 0x00;                          // command mode debug flags

//
// Audio level meter
//
#define METER_STATE_BOTH             0
#define METER_STATE_TOP              1
#define METER_STATE_BOTM             2
uint8_t active_audio_channels = 0; //deactivated by default
uint8_t audio_level_meter_active = 0;
uint8_t level_detector_slow[8];
elapsedMillis audio_meter_timer = 0;

void audioLevelMeter(uint8_t ch);
void audioLevelMeterInitCustomChars();

// follow Arduino style setup and loop functions
void setup()
{
    // setup display hardware
    setBaud((getBaud()>115200) ? 9600 : getBaud());
    setSize((COLS>MAXCOLS)?DEFAULTCOLS:COLS,(ROWS>MAXROWS)?DEFAULTROWS:ROWS);  // default display size
    pinMode(CONTRASTPIN, OUTPUT);
    pinMode(REDLITE, OUTPUT);
    pinMode(GREENLITE, OUTPUT);
    pinMode(BLUELITE, OUTPUT);
    // configure all GPIO as defined in EEPROM, default "safe" INPUT_PULLUP
    for(uint8_t i=0; i<4; i++)
    {
        gpioPinMode(i,EEPROM.read(GPIO_START_STATE_ADDR+i));
    }
    gpioMask = EEPROM.read(GPIO_MASK_ADDR);
    gpioPort = gpioRead(); // get intial state after setting up pins

    gpioDBBfIndex = 0;
    for(uint8_t i=0; i<4; i++)
    {
        gpioDebounceBuf[i] = gpioPort;
    }

    // for the initial 'screen flash' we want to use default settings:
    lcd.begin(COLS,ROWS);
    loadCustomCharBank(0);
    display(1);

    // configuration message to screen
    uint8_t dsplsh = EEPROM.read(BAUD_SPLASH_DISABLE);
    if(dsplsh != 1)
    {
        if (dsplsh != 0) // if it's not a 1, then it's garbage and this EEPROM
                         // address should be set to default
            eeSave(BAUD_SPLASH_DISABLE, 0);
        lcd.clear();
        lcd.spf(F("USB/Ser RGB Bkpk"));
        lcd.setCursor(0,1);
        lcd.spf(F("%4i00/8N1 %s"),(int)(getBaud()/100),VERSION);
        delayMS(500);
    }

    // now setup as defined in EEPROM
    red = EEPROM.read(BACKLIGHT_R_ADDR);
    green = EEPROM.read(BACKLIGHT_G_ADDR);
    blue = EEPROM.read(BACKLIGHT_B_ADDR);
    brightness = EEPROM.read(BACKLIGHT_BRIGHTNESS_ADDR);
    contrast = EEPROM.read(CONTRAST_ADDR);
    display(1);

    // do splash screen
    if (EEPROM.read(SPLASH_ADDR) == 0xFF)   // default
    {
        lcd.clear();
        lcd.spf(F("USB/Ser RGB Bpk+"));
        lcd.setCursor(0,1);
        lcd.spf(F("Transmogrifox"));
    }
    else
    {
        lcd.clear();
        for (uint8_t i=0; i<ROWS; i++)
        {
            virtualCursorPosition(1,i+1,ABSOLUTE);
            for (uint8_t j=0; j<COLS; j++)
            {
                // manual write since virtualWrite may scroll
                lcd.write(EEPROM.read(SPLASH_ADDR + i*COLS + j));
            };
        };
    };
    for (uint8_t d=0; d<EEPROM.read(SPLASH_DELAY_ADDR); d++)
        delayMS(100);
    since = 0;
    dbounce_timer = 0;
    retransmit_timer = 0;

    for(uint8_t i = 0; i < 8; i++)
        level_detector_slow[i] = i;
    audio_level_meter_active = 0;
    active_audio_channels = 0;

    virtualClear();
}

// basic loop operation:
//    look for serial character
//      for character parse/execute command or write to LCD
//    poll GPIO for changes
//    check for timedown of display timer
void loop()
{
    uint8_t tmp;
    // look for serial data input
    if (serialAvailable())
    {
        tmp = serialBlockingRead();
        if (tmp != MATRIX_COMMAND_PREFIX)
        {
            virtualWrite(tmp);  // not a command, just print the text!
            audio_level_meter_active = 0;
        }
        else
        {
            parseCommand();   // it marks a command, parse it!
        }
    };

    // check display timeout
    if (onTime)   // only execute if timed display
    {
        if (since >= 1000)   // 1000ms = 1 second elapsed
        {
            since = 0;
            onTime--;
            if (onTime == 0)
                display(0);
        };
    };

    //Tick audio level meter
    if(audio_level_meter_active == 1)
    {
        if(audio_meter_timer >= 30) //close to 30 fps
        {
            // Save current cursor positions
            uint8_t tmpX = vX;
            uint8_t tmpY = vY;
            audio_meter_timer = 0;
            for(uint8_t i = 0; i < 8; i++)
            {
                tmp = active_audio_channels;
                tmp = (tmp >> i);
                if(tmp & 0x01)
                    audioLevelMeter(i);
            }
            // Restore cursor positions
            virtualCursorPosition(tmpX,tmpY,ABSOLUTE);
        }
    }

    // when not waiting on serial, poll for input port changes
    // debouce -- 4 consecutive reads with identical state
    if(dbounce_timer >= 2)
    {
        dbounce_timer = 0;
        gpioDebounceBuf[gpioDBBfIndex] = gpioRead();
        tmp = 0;
        for(uint8_t i=0; i<4; i++)
        {
            if(gpioDebounceBuf[gpioDBBfIndex] != gpioDebounceBuf[i])
                tmp = 1;
        }

        if(retransmit_timer == 0)
        {
            if ((tmp == 0) && (gpioPort != gpioDebounceBuf[gpioDBBfIndex]))
            {
                //Toggle state
                gpioPort = gpioDebounceBuf[gpioDBBfIndex];
                //Ship it
                gpioSend(gpioPort);
                retransmit_timer = 20;
            }
        }
        else
            retransmit_timer--;

        //cycle circ buf
        if( ++gpioDBBfIndex >= 4)
            gpioDBBfIndex = 0;
    }


}

//parse Serial command, triggered by receiving comand prefix character, 0xFE
void parseCommand()
{
    uint8_t a, b;
    uint8_t cmd = serialBlockingRead(); // read the command byte
    switch (cmd)   // get the command byte and process
    {
    case MATRIX_DISPLAY_ON_TIMED:
        a = serialBlockingRead();
        onTime = (a&0x80) ? a&0x7F : a*60;  // MSB==0: 0-127 minutes, MSB==0: 1-27 seconds
        since = 0;
    case MATRIX_DISPLAY_ON:
        display(1);
        break;
    case MATRIX_DISPLAY_OFF:
        display(0);
        break;
    case MATRIX_SETSAVE_BRIGHTNESS:
    case MATRIX_SET_BRIGHTNESS:
        brightness = serialBlockingRead();
        if (cmd==MATRIX_SETSAVE_BRIGHTNESS)
            eeSave(BACKLIGHT_BRIGHTNESS_ADDR,brightness);
        display(1);
        break;
    case MATRIX_SETSAVE_CONTRAST:
    case MATRIX_SET_CONTRAST:
        contrast = serialBlockingRead();
        if (cmd==MATRIX_SETSAVE_CONTRAST)
            eeSave(CONTRAST_ADDR,contrast);
        display(1);
        break;
    case MATRIX_HOME:
        virtualCursorPosition(0,0,ABSOLUTE);
        break;
    case MATRIX_CLEAR:
        virtualClear();
        break;
    case MATRIX_AUTOSCROLL_OFF: // even value
    case MATRIX_AUTOSCROLL_ON:  // odd value
        // autoscroll flag is just LSB that happens to corrolate with cmd LSB
        if ((cmd&1)!=(EEPROM.read(SCROLLMODE_ADDR)&1))
            EEPROM.write(SCROLLMODE_ADDR, EEPROM.read(SCROLLMODE_ADDR)^1); // toggle LSB
        break;
    case MATRIX_SETCURSOR_POSITION:
        a = serialBlockingRead();
        a = (a>COLS) ? COLS : a;
        b = serialBlockingRead();
        b = (b>ROWS) ? ROWS : b;
        virtualCursorPosition(a,b,ABSOLUTE);
        audio_level_meter_active = 0;
        break;
    case MATRIX_MOVECURSOR_BACK:
        virtualCursorPosition(-1,0,RELATIVE);
        audio_level_meter_active = 0;
        break;
    case MATRIX_MOVECURSOR_FORWARD:
        virtualCursorPosition(1,0,RELATIVE);
        audio_level_meter_active = 0;
        break;
    case MATRIX_UNDERLINECURSOR_ON:
        lcd.cursor();
        break;
    case MATRIX_BLOCKCURSOR_OFF:
    case MATRIX_UNDERLINECURSOR_OFF:
        lcd.noCursor();
        lcd.noBlink();
        break;
    case MATRIX_BLOCKCURSOR_ON:
        lcd.blink();
        break;
    case MATRIX_CHANGESPLASH:
        // assume Splash Screen has been written to virtual display (LCD), save to EEPROM, wait ~ 0.5sec
        for (uint8_t y=0; y<ROWS; y++)
        {
            for (uint8_t x=0; x<COLS; x++)
            {
                eeSave(SPLASH_ADDR + y*COLS + x, virtualDisplay[y][x]);
            };
        };
        break;
    case MATRIX_SPLASH_DELAY:
        a = serialBlockingRead();
        eeSave(SPLASH_DELAY_ADDR,a);
        break;
    case MATRIX_BAUDRATE:
        a = serialBlockingRead();
        switch (a)
        {
        case 0x53:
            setBaud(1200);
            break;
        case 0x29:
            setBaud(2400);
            break;
        case 0xCf:
            setBaud(4800);
            break;
        case 0x67:
            setBaud(9600);
            break;
        case 0x33:
            setBaud(19200);
            break;
        case 0x22:
            setBaud(28800);
            break;
        case 0x19:
            setBaud(38400);
            break;
        case 0x10:
            setBaud(57600);
            break;
        case 0x08:
            setBaud(115200);
            break;
        default:
            setBaud(9600);
        };
        break;
    case MATRIX_CUSTOM_CHARACTER:
        a = serialBlockingRead();
        defineCustomChar(a, 255);
        break;
    case MATRIX_SAVECUSTOMCHAR:
        a = serialBlockingRead();
        b = serialBlockingRead();
        defineCustomChar(b, a);
        break;
    case MATRIX_LOADCUSTOMCHARBANK:
        a = serialBlockingRead();
        loadCustomCharBank(a);
        break;
    case MATRIX_GPIO_READANDMASK:
        gpioMask = serialBlockingRead();
        eeSave(GPIO_MASK_ADDR,gpioMask);
    case MATRIX_GPIO_READ:
        gpioPort = gpioRead();  // update, otherwise a mask change may cause an output on next check
        gpioSend(gpioPort);     // return current state
        break;
    case MATRIX_GPIO_OFF:
        a = serialBlockingRead()-1;
        if (a > 3) return;
        digitalWrite(GPIO[a], LOW);
        break;
    case MATRIX_GPIO_ON:
        a = serialBlockingRead()-1;
        if (a > 3) return;
        digitalWrite(GPIO[a], HIGH);
        break;
    case MATRIX_GPIO_START_STATE:
        a = serialBlockingRead()-1;
        b = serialBlockingRead();
        if (a > 3) return;
        gpioPinMode(a,b); // set current pinMode and save to EEPROM
        break;
    case EXTENDED_DISABLE_BAUD_SPLSH:
        a = serialBlockingRead();
        switch(a)
        {
            case 0:
            case 1:
                eeSave(BAUD_SPLASH_DISABLE, a);
                break;
            default:
                break;  // anything else will have no effect
        }
        break;
    case EXTENDED_AUDIO_LEVEL_METER:
        a = (uint8_t) serialBlockingRead();
        if(a < 8) // enables if valid number of channels, else disabled
        {
            if(!audio_level_meter_active) //change to special chars
                audioLevelMeterInitCustomChars();
            b = 1;
            b = (b << a);
            active_audio_channels |= b;
            b = (uint8_t) serialBlockingRead();
            if(b > level_detector_slow[a])
                level_detector_slow[a] = b;
            audio_level_meter_active = 1;
        } else // get it out of the buffer but don't do anything with it
        {
            b = (uint8_t) serialBlockingRead();
            if(audio_level_meter_active)
                loadCustomCharBank(0); // restore to default
            audio_level_meter_active = 0; //disabled
            active_audio_channels = 0;
        }
        break;
    case EXTENDED_RGBBACKLIGHTSAVE:
    case EXTENDED_RGBBACKLIGHT:
        red = serialBlockingRead();
        green = serialBlockingRead();
        blue = serialBlockingRead();
        if (cmd==EXTENDED_RGBBACKLIGHTSAVE)
        {
            eeSave(BACKLIGHT_R_ADDR,red);
            eeSave(BACKLIGHT_G_ADDR,green);
            eeSave(BACKLIGHT_B_ADDR,blue);
        };
        display(1);
        break;
    case EXTENDED_SETSIZE:
        a = serialBlockingRead();
        b = serialBlockingRead();
        setSize(a, b);
        break;
    case EXTENDED_SCROLLMODE:
        a = serialBlockingRead();  // bit 0: autoscroll, bit 1: scroll after
        eeSave(SCROLLMODE_ADDR,a);
        break;
    case EXTENDED_SCALERGBBACKLIGHT:  // onetime display type calibrate
        eeSave(SCALERED_ADDR,serialBlockingRead());
        eeSave(SCALEGREEN_ADDR,serialBlockingRead());
        eeSave(SCALEBLUE_ADDR,serialBlockingRead());
        display(1);
        break;
    case EXTENDED_DUMP_EEPROM:
        a = serialBlockingRead(); // first page
        b = serialBlockingRead(); // last page
        dumpEEPROM(a,b);
        break;
    case EXTENDED_EDIT_EEPROM:
        a = serialBlockingRead(); // page
        DEBUGTERM.spf(F("EEPROM Edit..."));
        dumpEEPROM(a,a);
        for (uint8_t j=0; j<EEPROM_PAGE_SIZE; j++)
            eeSave(a*EEPROM_PAGE_SIZE+j,serialBlockingRead()); // data
        DEBUGTERM.spf(F("EEPROM Verify..."));
        dumpEEPROM(a,a);
        break;
    case EXTENDED_CODE_TEST:
        break;
    };
}

void dumpEEPROM(uint8_t firstPage, uint8_t lastPage)
{
    for (uint8_t i=firstPage; i<=lastPage; i++)
    {
        DEBUGTERM.spf(F("EEPROM[0x%04X]: "),i);
        for (uint8_t j=0; j<EEPROM_PAGE_SIZE; j++)
        {
            DEBUGTERM.spf(F(" 0x%02X"),EEPROM.read(i*EEPROM_PAGE_SIZE+j));
        };
        DEBUGTERM.spf(F("\n"));
    };
}

// loads a character bank from EEPROM into RAM
void loadCustomCharBank(uint8_t bank)
{
    uint8_t newChar[8];
    if (bank > CUSTOM_CHARACTER_BANKS-1) return;
    int16_t addr = CUSTOMCHARBANKS_ADDR + (bank<<6);
    for (uint8_t loc = 0; loc < 8; loc++)
    {
        for (uint8_t i=0; i<8; i++)
            newChar[i] = EEPROM.read(addr+(loc<<3)+i);
        lcd.createChar(loc, newChar);
    }
}

// creates a custom character at loc (0-7) in bank (0-3) or in RAM (bank=255)
void defineCustomChar(uint8_t loc, uint8_t bank)
{
    uint8_t newChar[8];
    for (uint8_t i=0; i<8; i++)
    {
        newChar[i] = serialBlockingRead();
    }
    if (bank > 3)
    {
        lcd.createChar(loc, newChar);   // save in RAM
    }
    else    // save it to EEPROM
    {
        if (loc > 7)
            return;
        int16_t addr = CUSTOMCHARBANKS_ADDR + (bank<<6) + (loc<<3);
        for (uint8_t i=0; i<8; i++)
            eeSave(addr+i,newChar[i]);
    }
}

uint32_t getBaud()
{
    uint32_t b;
    b = EEPROM.read(BAUDRATE_ADDR);     //MS byte
    b <<= 8;
    b |= EEPROM.read(BAUDRATE_ADDR+1);
    b <<= 8;
    b |= EEPROM.read(BAUDRATE_ADDR+2);  // LS byte
    return b;
}

void setBaud(uint32_t baudrate)
{
    if (getBaud()!=baudrate)
    {
        eeSave(BAUDRATE_ADDR, baudrate >> 16);
        eeSave(BAUDRATE_ADDR+1, baudrate >> 8);
        eeSave(BAUDRATE_ADDR+2, baudrate & 0xFF);
    }
    USB.begin(baudrate);
    UART.begin(baudrate);
    pinMode(PD2,INPUT_PULLUP);  // the UART is a little noisy without a pullup
}

int serialAvailable()
{
    return max(USB.available(), UART.available());
}

uint8_t serialBlockingRead()
{
    uint8_t timeout = EEPROM.read(SERIAL_TIMEOUT);
    uint8_t s = 0;
    if (timeout)
    {
        while (!serialAvailable())        // only wait until timeout
        {
            delayMS(1);
            if (timeout--==0) return 0;
        };
    }
    else
    {
        while (!serialAvailable());       // wait indefinitely
    };
    uint8_t c = 0;
    if (USB.available())
    {
        c = USB.read();
    }
    else if (UART.available())
    {
        c = UART.read();
        s = 1;
    };
    if (cmdFlags&ECHOCHARS)
        DEBUGTERM.spf(F("%s RX: '%c' [%d] (0x%02X)\n"),(s)?"UART":"USB",(c<32)?' ':c,c,c);
    return c;
}

// sets the virtual display size to match lcd hardware
void setSize(uint8_t c, uint8_t r)
{
    eeSave(ROWS_ADDR, r);
    eeSave(COLS_ADDR, c);
    COLS = c;
    ROWS = r;
}

// turns LCD display on or off, call to refresh too
void display(uint8_t state)
{
    if (state)
    {
        // turn on, which means updating analog control values
        analogWrite(CONTRASTPIN, 255-contrast); // contrast behavior is inverted
        // normalize the LEDs brightnesses
        uint8_t r = map(red, 0, 255, 0, EEPROM.read(SCALERED_ADDR));
        uint8_t g = map(green, 0, 255, 0, EEPROM.read(SCALEGREEN_ADDR));
        uint8_t b = map(blue, 0, 255, 0, EEPROM.read(SCALEBLUE_ADDR));
        r = map(r, 0, 255, 0, brightness);
        g = map(g, 0, 255, 0, brightness);
        b = map(b, 0, 255, 0, brightness);
        // set the backlight
        analogWrite(REDLITE, r);
        analogWrite(GREENLITE, g);
        analogWrite(BLUELITE, b);
    }
    else
    {
        // turn backlights off
        analogWrite(REDLITE, 0);
        analogWrite(GREENLITE, 0);
        analogWrite(BLUELITE, 0);
    }
}

// virtual display clear...
void virtualClear()
{
    for (uint8_t y=0; y <ROWS; y++)
    {
        for (uint8_t x=0; x<COLS; x++)
        {
            virtualDisplay[y][x] = ' ';
        }
        lcd.clear();  // includes 'home'
        delayMS(2);   // give it a little more time
        virtualCursorPosition(1,1,ABSOLUTE); // clear includes home, but sync virtual
        vScroll = 0;
    }
}

// virtual display positioning...
void virtualCursorPosition(int8_t col, int8_t row, bool positioning)
{
    if (positioning==RELATIVE)
    {
        // assumes only moves +/-1 row or column at a time, i.e. by character
        vX += col;
        vY += row;   // used for backspace, scrolling
    }
    else
    {
        // c,r: one-based; x,y: zero-based
        vX = col-1;
        vY = row-1; // absolute positioning
    };
    if (vX < 0)
    {
        vX =+ COLS;
        vY -= 1;
    };
    if (vY < 0)
    {
        vY += ROWS;
    };
    if (vX >= COLS)
    {
        vX = 0;
        vY += 1;
    };
    if (vY >= ROWS)
    {
        vY = 0;
        vScroll = EEPROM.read(SCROLLMODE_ADDR); // can only be set here
    };
    lcd.setCursor(vX,vY);  // physical positioning
}

// virtual display scroll...
void virtualScroll(uint8_t mode)
{
    uint8_t c;
    // autoscroll on AND (mode==(vScroll&2); mode:0->before, mode:2->after
    if ((vScroll&SCROLLAUTO) && (mode==(vScroll&SCROLLMODE)))
    {
        if (cmdFlags&DUMPENABLE) //debug
            DEBUGTERM.spf(F("SCROLLED\n"));
        // scroll the virtualDisplay and LCD...
        for (uint8_t y=0; y<ROWS; y++)
        {
            virtualCursorPosition(1,y+1,ABSOLUTE);  // needed for autowrap off
            for (uint8_t x=0; x<COLS; x++)
            {
                // move characters up a row, fill last row with ' '
                c = (y!=(ROWS-1)) ? virtualDisplay[y+1][x] : ' ';
                virtualDisplay[y][x] = c;
                lcd.write(c);
            };
        };
        virtualCursorPosition(1,ROWS,ABSOLUTE);
        vScroll = 0; // scroll flag is only cleared here
    };
}

// virtual display output...
void virtualWrite(char ch)
{
    if (ch==LF || ch==CR)
    {
        // special exception for new lines either \r or \n, fill to end of line with spaces
        if (!(vLastNL==CR && ch==LF))     // ignore \n when preceded by \r
        {
            do
            {
                virtualWrite(' ');
            }
            while (vX != 0);
            vLastNL = ch;
            return;
        };
    }
    else if (ch == BS)    // backspace
    {
        virtualCursorPosition(-1,0,RELATIVE);
        virtualWrite(' ');
        virtualCursorPosition(-1,0,RELATIVE);
    }
    else
    {
        // its a plain character to write to the LCD...
        // scrolling before write rather than after preserves all lines of scrolling display
        virtualScroll(SCROLLBEFORE);
        virtualDisplay[vY][vX] = ch;
        lcd.write(ch);
        virtualCursorPosition(1,0,RELATIVE);
        // scrolling after write rather than before preserves cursor behavior (Adafruit default)
        virtualScroll(SCROLLAFTER);
    }
    //dumpVirtualDisplay(cmdFlags&DUMPENABLE); //debug
}


// Audio Level meter
// Valid channels are up to 2xROWS.  1/2 a row is left, other 1/2 is right
// Level max is 0 to (COLS)
// Any level larger than level max will indicate clipping
void audioLevelMeterInitCustomChars()
{
    uint8_t newChar[8];
    for (uint8_t i=0; i<8; i++)
    {
        newChar[i] = 0xFF;
    }
    newChar[3] = 0x00;
    newChar[4] = 0x00;
    lcd.createChar(METER_STATE_BOTH, newChar);   // save in RAM

    for (uint8_t i=0; i<3; i++)
    {
        newChar[i] = 0xFF;
    }
    for (uint8_t i=3; i<8; i++)
    {
        newChar[i] = 0x00;
    }
    lcd.createChar(METER_STATE_TOP, newChar);   // save in RAM

    for (uint8_t i=0; i<5; i++)
    {
        newChar[i] = 0x00;
    }
    for (uint8_t i=5; i<8; i++)
    {
        newChar[i] = 0xFF;
    }
    lcd.createChar(METER_STATE_BOTM, newChar);   // save in RAM

}

void audioLevelMeter(uint8_t ch)
{
    uint8_t ldrow = 1 + (ch >> 1);  // which row to write
    uint8_t pk = level_detector_slow[ch];
    uint8_t dpk = 0;
    virtualCursorPosition(1,ldrow,ABSOLUTE);

    //
    // Clipping detection
    //

    //Temporarily save color scheme
    uint8_t r, g, b;
    r = red;
    g = green;
    b = blue;
    //flash red if clipping
    if(level_detector_slow[ch] >= COLS)
    {
        red = 255;
        green = 0;
        blue = 0;
    }
    if(pk > 0x1F)
        pk = 0x1F;
    display(1);
    //Restore the color
    red = r;
    green = g;
    blue = b;

    // peak decay
    dpk = (pk << 3);
    dpk -= pk;
    pk = (dpk >> 3);

    level_detector_slow[ch] = pk;  //Then save it for the next time around

    //next work out what custom characters need to be created
    uint8_t ldch = (ch & (0xFE));
    uint8_t pkl = level_detector_slow[ldch];
    uint8_t pkh = level_detector_slow[ldch+1];
    uint8_t meter_state = ' ';
    virtualCursorPosition(1,ldrow,ABSOLUTE);

    for(uint8_t k = 0; k < COLS; k++)
    {
        //determine meter state
        if((pkl > 0) && (pkh > 0))
        {
            meter_state = METER_STATE_BOTH;
        }
        else if ((pkl == 0) && (pkh == 0))
        {
            meter_state = ' ';
        }
        else if ((pkl < 1) && (pkh > 0))
        {
            meter_state = METER_STATE_TOP;
        }
        else if ((pkl > 0) && (pkh < 1))
        {
            meter_state = METER_STATE_BOTM;
        }
        else
        {
            meter_state = ' ';
        }

        if(pkl > 0) pkl--;
        if(pkh > 0) pkh--;

        //if(k == 1) virtualCursorPosition(1,ldrow,ABSOLUTE);
        virtualWrite(meter_state);
    }

}

// additional support for GPIO
// sets the pinMode of a GPIO and updates EEPROM
// bit: 0-3, GPIO_1-GPIO_4, respectively
// mode: 0x00:OUT LOW, 0x01:OUT HIGH, 0x10: INPUT, 0x10: INPUT_PULLUP
void gpioPinMode(uint8_t bit, uint8_t mode)
{
    switch (mode)
    {
    case 0x00:
        pinMode(GPIO[bit],OUTPUT);
        digitalWrite(GPIO[bit],0);
        break;
    case 0x01:
        pinMode(GPIO[bit],OUTPUT);
        digitalWrite(GPIO[bit],1);
        break;
    case 0x10:
        pinMode(GPIO[bit],INPUT);
        break;
    default:
        pinMode(GPIO[bit],INPUT_PULLUP);
        mode = 0x11;
    }
    eeSave(GPIO_START_STATE_ADDR+bit,mode); // update EEPROM
}

// read all the GPIO pins as a port
uint8_t gpioRead()
{
    uint8_t p = 0;
    p = digitalRead(GPIO[3]);
    p = p<<1 | digitalRead(GPIO[2]);
    p = p<<1 | digitalRead(GPIO[1]);
    p = p<<1 | digitalRead(GPIO[0]);
    p = (p&gpioMask)^(gpioMask>>4);
    return p;
}

// report GPIO port state to USB and UART
void gpioSend(uint8_t p)
{
    USB.spf(F("#0x%02X\n"),p);
    UART.spf(F("#0x%02X\n"),p);
}

// all EEPORM write checked to reduce wear
void eeSave(uint16_t addr, uint8_t value)
{
    if (value != EEPROM.read(addr))
        EEPROM.write(addr, value);
}
