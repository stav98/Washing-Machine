#include <pgmspace.h>
#include "display_CG.h"

#define  DISP_DATA    15
#define  DISP_CLK     2

// 74HC595 LCD control pin layout
#define LCD_CS1    0b00100000
#define LCD_CS2    0b01000000
#define LCD_RESET  0b00000000
#define LCD_DATA   0b00010000
#define LCD_READ   0b00000000
#define LCD_ENABLE 0b00001000

// Commands sent when LCD in "instruction" mode (LCD_DATA bit set to 0)
#define LCD_ON          0x3F
#define LCD_OFF         0x3E
#define LCD_SET_ADD     0x40   // plus X address (0 to 63)
#define LCD_SET_PAGE    0xB8   // plus Y address (0 to 7)
#define LCD_DISP_START  0xC0   // plus X address (0 to 63) - for scrolling

//#define clockPulse()  {digitalWrite(_clkPin, HIGH); digitalWrite(_clkPin, LOW);} //GENERIC
//#define clockPulse()  {PORTD |= _BV(PD3); PORTD &= ~_BV(PD3);} //AVR
#define clockPulse()  {GPIO.out_w1ts = (1 << DISP_CLK); GPIO.out_w1tc = (1 << DISP_CLK);} //ESP32
//#define bitOut(val)   {if (val) digitalWrite(_dataPin, HIGH); else digitalWrite(_dataPin, LOW);} //GENERIC
//#define bitOut(val)    {if (val) PORTD |= _BV(PD2); else PORTD &= ~_BV(PD2);} //AVR
#define bitOut(val)   {val ? GPIO.out_w1ts = (1 << DISP_DATA) : GPIO.out_w1tc = (1 << DISP_DATA);} //ESP32
#define sendBit(val)  {bitOut(val); clockPulse();}

//Για να εκτυπώνει από PROGMEM με κλήση lcd.print(PGMSTR(msg1)); δηλαδή όπως η F("....") για Serial.print
#define PGMSTR(x)   (__FlashStringHelper*)(x)

/* //Παρόμοιο με το παραπάνω με κλήση lcd_print_P(msg1);
void lcd_print_P(const void *str)
    {
     size_t n = 0;
     char c;
     while ((c = pgm_read_byte ((const uint8_t *)(str) + n))) 
           {
            lcd.print ((char) c);
            n++;
           }
    }
*/

const byte *font;

class Serial_Graphical_LCD_display : public Print
{
 private:
  byte _chipSelect;  // currently-selected chip (LCD_CS1 or LCD_CS2)
  byte _lcdx;        // current x position (0 - 127)
  byte _lcdy;        // current y position (0 - 63)
  byte _cache [64 * 128 / 8];
  short  _cacheOffset;
  boolean _delayedWrite;
  byte  _leftX;
  byte  _rightX;
  byte  _topY;
  byte  _bottomY;
  uint8_t _highByte;
  uint8_t _lowByte;
  uint8_t _clkPin;
  uint8_t _dataPin;
  boolean _invmode;
  byte _textsize;
   
  void sendByte(const uint8_t val)
   {
    uint8_t t, i;
    t = val;
    sendBit(t & 0x01);    // Unrolled loop for speed
    t >>= 1;
    sendBit(t & 0x01)
    t >>= 1;
    sendBit(t & 0x01)
    t >>= 1;
    sendBit(t & 0x01)
    t >>= 1;
    sendBit(t & 0x01)
    t >>= 1;
    sendBit(t & 0x01)
    t >>= 1;
    sendBit(t & 0x01)
    t >>= 1;
    sendBit(t & 0x01)
   }

  void sendXL595(const byte data, const byte lowFlags, const byte highFlags)
   {
    _lowByte = ((data << 5) & 0xe0) | lowFlags | 0x01;   // Combine bits 0-2, DI and EN, latch enable
    _highByte = ((data >> 3) & 0x1f) | highFlags | 0x80;        // Bits 3-7, chipselects, latch command
    //digitalWrite(7, LOW);   //Αν βάλω Latch En κάνει 11ms λιγότερο. 170ms από 220 που έκανε με αντίσταση - δίοδο
    //-PORTD &= ~_BV(PD7); //150ms Πολύ καλύτερα αν γράφω απευθείας στη πόρτα
    sendByte(_lowByte);     // Load the shift registers
    sendByte(_highByte);    // (latch gets set when leading 1 clocks into IC2 QH)
    //digitalWrite(7, HIGH);
    //-PORTD |= _BV(PD7); //150ms
    bitOut(0);
    clockPulse();         // (Unrolled loop for speed)
    clockPulse();         // This clears the whole shift register,
    clockPulse();         // ensuring that the latch bit stays low
    clockPulse();         // while the next 16-bit word is clocked in.
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
    clockPulse();
   }
 
  // read the byte corresponding to the selected x,y position
  byte readData ()
   {
    return _cache [_cacheOffset];
   }
  
  // Initialize for delayed writes
  void startDelayedWrite()
   {
    if (_delayedWrite)
        endDelayedWrite();
    _delayedWrite = true;
    _leftX = 127;
    _rightX = 0;
    _topY = 63;
    _bottomY = 0;
   }

  // Finish up delayed write by actually writing data from cache
  void endDelayedWrite()
   {
    int cOffset;
    int width;
    byte y;
    int i;
    byte savLcdx = _chipSelect == LCD_CS2 ? _lcdx + 64 : _lcdx;
    byte savLcdy = _lcdy;
    if (_rightX > 127)
        _rightX = 127;
    if (_bottomY > 63)
        _bottomY = 63;
    if (_delayedWrite && (_leftX <= _rightX) && (_topY <= _bottomY))
       {
        for (y = (_topY & 0xF1); y <= _bottomY; y += 8)
            {
             gotoxy(_leftX, y);
             cOffset = (_leftX << 3) | (y >> 3);
             width = _rightX - _leftX + 1;
             for (i = 0; i < width; i++)
                 {
                  writeData(_cache[cOffset]);
                  cOffset += 8;
                 }
            }
        gotoxy(savLcdx, savLcdy);
       }
    _delayedWrite = false;
   }
   
  // Update limits of delayed write based on current x-y position
  void updateDelayedWrite(const byte x, const byte y)
   {
    if (_delayedWrite)
       {
        if (x < _leftX)
            _leftX = x;
        if (x > _rightX)
            _rightX = x;
        if (y < _topY)
            _topY = y;
        if (y > _bottomY)
            _bottomY = y;
       }
   }

 public:
  // constructor
  // Approx time to run: 600 ms on Arduino Uno
  void begin (const byte data = 2, const byte clk = 3)
   {
    _dataPin = data;        // reuse first parameter as data pin
    _clkPin = clk;   // reuse second parameter as clock pin
    pinMode(_dataPin, OUTPUT);
    pinMode(_clkPin, OUTPUT);
    digitalWrite(_dataPin, LOW);
    digitalWrite(_clkPin, LOW);
    sendXL595(0, 0, 0);
    delay(1);

    // turn LCD chip 1 on
    _chipSelect = LCD_CS1;
    cmd (LCD_ON);
    // turn LCD chip 2 on
    _chipSelect = LCD_CS2;
    cmd (LCD_ON);
    // init text size
    textSize();
    // clear entire LCD display
    clear ();
    // and put the cursor in the top-left corner
    gotoxy (0, 0);
    // ensure scroll is set to zero
    scroll (0);
   }  // end of graphical_LCD_display (constructor)
  
  // send command to LCD display (chip 1 or 2 as in chipSelect variable)
  // for example, setting page (Y) or address (X)
  void cmd (const byte data)
   {
    sendXL595(data, LCD_ENABLE, _chipSelect);
    sendXL595(data, 0, _chipSelect);
   }
    
  // set our "cursor" to the x/y position
  // works out whether this refers to chip 1 or chip 2 and sets chipSelect appropriately

  // Approx time to run: 33 ms on Arduino Uno
  void gotoxy (byte x, byte y)
   {
    if (x > 127)
        x = 0;
    if (y > 63)
        y = 0;
    _cacheOffset = 0;
    // work out which chip
    if (x >= 64)
       {
        x -= 64;
        _chipSelect = LCD_CS2;
        _cacheOffset = 64 * 64 / 8;  // half-way through cache
       }
    else
       _chipSelect = LCD_CS1;
    // remember for incrementing later
    _lcdx = x;
    _lcdy = y;
    // command LCD to the correct page and address
    cmd (LCD_SET_PAGE | (y >> 3) );  // 8 pixels to a page
    cmd (LCD_SET_ADD  | x );
    _cacheOffset += (x << 3) | y >> 3;
   }

  // write a byte to the LCD display at the selected x,y position
  // if inv true, invert the data
  // writing advances the cursor 1 pixel to the right
  // it wraps to the next "line" if necessary (a line is 8 pixels deep)
  void writeData (byte data, const boolean inv)
   {
    // invert data to be written if wanted
    if (inv)
        data ^= 0xFF;
    sendXL595(data, LCD_DATA | LCD_ENABLE, _chipSelect);
    sendXL595(data, LCD_DATA, _chipSelect);
    _cache [_cacheOffset] = data;
    // we have now moved right one pixel (in the LCD hardware)
    _lcdx++;
    // see if we moved from chip 1 to chip 2, or wrapped at end of line
    if (_lcdx >= 64)
       {
        if (_chipSelect == LCD_CS1)  // on chip 1, move to chip 2
            gotoxy (64, _lcdy);
        else
            gotoxy (0, _lcdy + 8);  // go back to chip 1, down one line
       }  // if >= 64
    else
       {
        _cacheOffset += 8;
       }
   }
  void writeData (byte data) { writeData(data, _invmode);}

  // write one letter (space to 0x7F), inverted or normal
  // Approx time to run: 4 ms on Arduino Uno
  void putch (byte c, const boolean inv)
   {
    byte cg_offset = 7;
    byte font_width = pgm_read_byte (&font[2]);
    byte font_height = pgm_read_byte (&font[3]);
    byte first_char = pgm_read_byte (&font[4]);
    byte char_count = pgm_read_byte (&font[5]);
    byte x_padding = pgm_read_byte (&font[6]);
    
    byte x0 = (_chipSelect != LCD_CS2) ? _lcdx : _lcdx + 64;
    byte y0 = _lcdy;
    if (c == '\r')
        gotoxy(0, _lcdy);
    if (c == '\n')
        gotoxy(x0, _lcdy + font_height * _textsize); //8
    if (c < 0x20)
        return;
    //if (c > 0x7F)
    //  c = 0x7F;  // unknown glyph
    if (c < first_char)
        return;
    if (c > (first_char + char_count)) //Αν έφτασε στο τέλος του CG τότε δείξε το τελευταίο
        c = first_char + char_count;  //άγνωστος χαρακτήρας
    c -= 0x20; //ο 1ος χαρακτήρας είναι το κενό 0x20 ή 32
    // Αν δεν υπάρχει χώρος για ολόκληρο χαρακτήρα, τότε άλλαξε γραμμή
    // letters are 5 wide, so once we are past 59, there isn't room before we hit 63
    //if (_lcdx + (_textsize * 6) > 63 && _chipSelect == LCD_CS2) 
    if (_lcdx + (_textsize * (font_width)) > 63 && _chipSelect == LCD_CS2) //(font_width+1)
       {
        y0 = _lcdy + (font_height * _textsize);  //8
        if(y0 > (64 - (font_height * _textsize))) //8
           y0 = 0;
        gotoxy (0, y0);
        x0 = 0;
       }

    if (_textsize == 1)      // Αν το μέγεθος είναι 1
       {
        // font data is in PROGMEM memory (firmware)
        for (byte x = 0; x < font_width; x++) //5
             //writeData (pgm_read_byte (&font [c] [x]), inv);
             writeData (pgm_read_byte (&font[c * font_width + x + cg_offset]), inv);
        writeData (0, inv);  // one-pixel gap between letters
       } 
    else //Μεγαλύτερο μέγεθος
       {
        // For larger text, we will scale up the character from the font table.
        startDelayedWrite();          // set flag for deferred write
        updateDelayedWrite(x0, y0);   // set top left corner of delayed area
        //updateDelayedWrite(x0 + (6 * _textsize), y0 + (8 * _textsize)); // set bottom right corner
        updateDelayedWrite(x0 + (font_width * _textsize), y0 + (8 * _textsize)); //6 set bottom right corner
        for (byte x = 0; x < font_width; x++)   //5 columns per character in font table
            {
             //byte b = pgm_read_byte(&font[c][x]);    // Get a font column
             byte b = pgm_read_byte(&font[c * font_width + x + cg_offset]);    // Get a font column
             byte bx = (x * _textsize);      // Where does this column start in scaled character?
             if (inv)                         // Invert bits if requested
                 b ^= 0xff;
             for (byte bi = 0; bi < font_height; bi++)  //8 For each pixel in font column
                 {
                  byte yy = bi * _textsize;         // starting Y position of scaled pixel
                  for (byte y = 0; y < _textsize; y++) // for height of scaled pixel
                      {
                       for (byte i = bx; i < bx + _textsize; i++) // for width of scaled pixel
                           {
                            if (b & 0x01)   // set or clear scaled pixel
                                _cache[_cacheOffset + ((i << 3) | ((yy + y) >> 3))] |= (1<<((yy+y) & 7));
                            else
                                _cache[_cacheOffset + ((i << 3) | ((yy + y) >> 3))] &= ~(1<<((yy+y) & 7));
                           }
                      }
                      b >>= 1;      // Next pixel in column
                 }
            }
        // Now generate the scaled spacing column - could be all black or all white
        byte sp = (inv) ? 1 : 0;
        for (byte x = 0; x < _textsize; x++) 
            {
             for (byte y = 0; y < _textsize; y++) 
                  //_cache[_cacheOffset + (((x + (5 * _textsize)) << 3) | (y >> 3))] = sp;
                  _cache[_cacheOffset + (((x + (font_width * _textsize)) << 3) | (y >> 3))] = sp;   //5 
            }
        endDelayedWrite();    // Refresh display from writethrough buffer
       }
    //gotoxy(x0 + 6*_textsize, y0); 
    gotoxy(x0 + (font_width + x_padding) * _textsize, y0); //6
   }

  //Πολυμορφική χρήση με μία παράμετρο
  void putch (byte c) 
   {
    putch(c, _invmode);
   }

  // write an entire null-terminated string to the LCD: inverted or normal
  void string (const char * s, const boolean inv)
   {
    char c;
    while ((c = *s++))
           putch (c, inv);
   }
   
  void string (const char * s) {string(s, _invmode);}

  //Αντιγράφει μια σειρά από bytes από την PROGMEM στην οθόνη. Προσοχή ύψος μέχρι 8 pixels
  //Κλήση lcd.blit (picture, sizeof picture); και η εικόνα δηλώνεται ως const byte picture [] PROGMEM = {0x1C, 0x22, 0x49, 0xA1, 0xA1, 0x49, 0x22, 0x1C};
  void blit (const byte * pic, const unsigned short size)
   {
    for (unsigned short x = 0; x < size; x++, pic++)
         writeData (pgm_read_byte (pic));
   }

  //Εμφανίζει μια εικόνα διαστάσεων X * Y στην οθόνη στις θέσεις posx, posy
  //Περίπου 10μsec / byte για esp32
  void img (const byte * pic, byte posx, byte posy, boolean inv = 0)
   {
    byte img_width = pgm_read_byte(pic++);
    byte img_height = (byte)(pgm_read_byte(pic++) / 8);
    byte data;
    for (byte y = 0; y < img_height; y++)
        {
         gotoxy(posx, (posy + (y * 8)));
         for (byte x = 0; x < img_width; x++, pic++)
             {
              data = pgm_read_byte (pic);
              writeData(data, inv);
             }
        }
   }

  // clear rectangle x1,y1,x2,y2 (inclusive) to val (eg. 0x00 for black, 0xFF for white)
  // default is entire screen to black
  // rectangle is forced to nearest (lower) 8 pixels vertically
  // this if faster than lcd_fill_rect because it doesn't read from the display
  // Approx time to run: 120 ms on Arduino Uno for 20 x 50 pixel rectangle
  void clear (const byte x1 = 0, const byte y1 = 0, const byte x2 = 127, /*end pixel*/ const byte y2 = 63, const byte val = 0) // what to fill with 
   {
    scroll();
    _delayedWrite = false;
    for (byte y = y1; y <= y2; y += 8)
        {
         gotoxy (x1, y);
         for (byte x = x1; x <= x2; x++)
              writeData (val);
        } // end of for y
    gotoxy (x1, y1); 
   }
  
  // set or clear a pixel at x,y
  // warning: this is slow because we have to read the existing pixel in from the LCD display
  // so we can change a single bit in it
  //void setPixel (const byte x, const byte y, const byte val = 1)
  void setPixel (const byte x, const byte y, const byte val)
   {
    short readOffset = 0;
    byte rx = 0, ry = 0;
    updateDelayedWrite(x, y);
    if (x < 128)
        rx = x;
    if (y < 64)
        ry = y;
    // work out which chip
    if (rx >= 64)
       {
        rx -= 64;
        readOffset = 64 * 64 / 8;  // half-way through cache
        if (_delayedWrite)
            _chipSelect = LCD_CS2;
       }
    else
        if (_delayedWrite)
            _chipSelect = LCD_CS1;
    readOffset += (rx << 3) | ry >> 3;
    if (_delayedWrite)
       {
        _lcdx = rx;
        _lcdy = ry;
        _cacheOffset = readOffset;
       }
    byte c = _cache[readOffset];
    // toggle or clear this particular one as required
    if (val)
        c |=   1 << (y & 7);    // set pixel
    else
        c &= ~(1 << (y & 7));   // clear pixel
    if (_delayedWrite)
       {
        _cache[_cacheOffset] = c;
        // we have now moved right one pixel (in the LCD hardware)
        _lcdx++;
        // see if we moved from chip 1 to chip 2, or wrapped at end of line
        if (_lcdx >= 64)
           {
            if (_chipSelect == LCD_CS1)  // on chip 1, move to chip 2
               {
                _chipSelect = LCD_CS2;
                _lcdx = 0;
               }
            else
               {
                _chipSelect = LCD_CS1;
                _lcdx = 0;
                _lcdy += 8;
                if (_lcdy >= 64)
                    _lcdy = 0;
               }
            _cacheOffset = (64 * 64 / 8) + ((_lcdx << 3) | (_lcdy >> 3));
           }  // if >= 64
        else
            _cacheOffset += 8;
       }
    else
       {
        gotoxy(x, y);
        writeData(c);
       }
   }
  
  //void fillRect (const byte x1 = 0, /*start pixel*/ const byte y1 = 0, const byte x2 = 127, /*end pixel*/ const byte y2 = 63, const byte val = 1);  // what to draw (0 = white, 1 = black)
  // fill the rectangle x1,y1,x2,y2 (inclusive) with black (1) or white (0)
  // if possible use lcd_clear instead because it is much faster
  // however lcd_clear clears batches of 8 vertical pixels

  // Approx time to run: 5230 ms on Arduino Uno for 20 x 50 pixel rectangle
  //    (Yep, that's over 5 seconds!)
  void fillRect (const byte x1, /*start pixel*/ const byte y1, const byte x2, /*end pixel*/ const byte y2, const byte val)  // what to draw (0 = white, 1 = black)
   {
    startDelayedWrite();
    for (byte y = y1; y <= y2; y++)
         for (byte x = x1; x <= x2; x++)
              setPixel (x, y, val);
    endDelayedWrite();
   }
  
  /*void frameRect (const byte x1 = 0,    // start pixel
                 const byte y1 = 0,
                 const byte x2 = 127, // end pixel
                 const byte y2 = 63,
                 const byte val = 1,    // what to draw (0 = white, 1 = black)
                 const byte width = 1);*/
  // frame the rectangle x1,y1,x2,y2 (inclusive) with black (1) or white (0)
  // width is width of frame, frames grow inwards

  // Approx time to run:  730 ms on Arduino Uno for 20 x 50 pixel rectangle with 1-pixel wide border
  //             1430 ms on Arduino Uno for 20 x 50 pixel rectangle with 2-pixel wide border
  void frameRect (const byte x1, // start pixel
                  const byte y1,
                  const byte x2, // end pixel
                  const byte y2,
                  const byte val,    // what to draw (0 = white, 1 = black)
                  const byte width)
   {
    byte x, y, i;
    startDelayedWrite();
    // top and bottom line
    for (x = x1; x <= x2; x++)
         for (i = 0; i < width; i++)
             {
              setPixel (x, y1 + i, val);
              setPixel (x, y2 - i, val);
             }
    // left and right line
    for (y = y1; y <= y2; y++)
         for (i = 0; i < width; i++)
             {
              setPixel (x1 + i, y, val);
              setPixel (x2 - i, y, val);
             }
    endDelayedWrite();
   }
  
  /*void line  (const byte x1 = 0,    // start pixel
              const byte y1 = 0,
              const byte x2 = 127,  // end pixel
              const byte y2 = 63,
              const byte val = 1);  // what to draw (0 = white, 1 = black)*/

  // draw a line from x1,y1 to x2,y2 (inclusive) with black (1) or white (0)
  // Warning: fairly slow, as is anything that draws individual pixels
  void line (const byte x1,  // start pixel
             const byte y1,
             const byte x2,  // end pixel
             const byte y2,
             const byte val)  // what to draw (0 = white, 1 = black)
   {
    byte x, y;
    // vertical line? do quick way
    if (x1 == x2)
       {
        for (y = y1; y <= y2; y++)
             setPixel (x1, y, val);
       } 
    else if (y1 == y2) // horizontal line? do quick way
       {
        for (x = x1; x <= x2; x++)
             setPixel (x, y1, val);
       } 
    else  // Not vertical or horizontal - there will be slope calculations
       {
        short x_diff = x2 - x1,
        y_diff = y2 - y1;
        // if x difference > y difference, draw every x pixels
        if (abs(x_diff) > abs (y_diff))
           {
            short x_inc = 1;
            short y_inc = (y_diff << 8) / x_diff;
            short y_temp = y1 << 8;
            if (x_diff < 0)
            x_inc = -1;
            for (x = x1; x != x2; x += x_inc)
                {
                 setPixel (x, y_temp >> 8, val);
                 y_temp += y_inc;
                }
           } 
        else // otherwise draw every y pixels  
           {
            short x_inc = (x_diff << 8) / y_diff;
            short y_inc = 1;
            if (y_diff < 0)
            y_inc = -1;
            short x_temp = x1 << 8;
            for (y = y1; y != y2; y += y_inc)
                {
                 setPixel (x_temp >> 8, y, val);
                 x_temp += x_inc;
                }
           }
       }
   }
  /*void circle (const byte x = 64,   // Center of circle
               const byte y = 32,
               const byte r = 30,   // Radius
               const byte val = 1); // what to draw (0 = white, 1 = black)*/
  // Draw a circle
  void circle (const byte x0, // x coordinate of center
               const byte y0, // y coordinate of center
               const byte radius,  // radius
               const byte val)   // what to draw (0 = white, 1 = black)
   {
    short x = radius-1;
    short y = 0;
    short dx = 1;
    short dy = 1;
    short err = dx - (radius << 1);
    startDelayedWrite();
    while (x >= y)
          {
           setPixel(x0 + x, y0 + y, val);
           setPixel(x0 + y, y0 + x, val);
           setPixel(x0 - y, y0 + x, val);
           setPixel(x0 - x, y0 + y, val);
           setPixel(x0 - x, y0 - y, val);
           setPixel(x0 - y, y0 - x, val);
           setPixel(x0 + y, y0 - x, val);
           setPixel(x0 + x, y0 - y, val);
           if (err <= 0)
              {
               y++;
               err += dy;
               dy += 2;
              }
           if (err > 0)
              {
               x--;
               dx += 2;
               err += (-radius << 1) + dx;
              }
          }
   endDelayedWrite();
  }
      
  /*void fillCircle (const byte x = 64,   // Center of circle
                   const byte y = 32,
                   const byte r = 30,   // Radius
                   const byte val = 1); // what to draw (0 = white, 1 = black) */
  // Draw a filled circle
  void fillCircle (const byte x0,        // x coordinate of center
                   const byte y0,        // y coordinate of center
                   const byte radius,    // radius
                   const byte val)     // what to draw (0 = white, 1 = black)
   {
    short x = radius-1;
    short y = 0;
    short dx = 1;
    short dy = 1;
    short err = dx - (radius << 1);
    short q;
    startDelayedWrite();
    while (y < radius)
          {
           if (err <= 0)
              {
               for (q = (x0 - x); q <= (x0 + x); q++)
                   {
                    setPixel(q, y0 + y, val);
                    setPixel(q, y0 - y, val);
                   }
               y++;
               err += dy;
               dy += 2;
              }
           else // if (err > 0)
              {
               x--;
               dx += 2;
               err += (-radius << 1) + dx;
              }
          }
    endDelayedWrite();
   }
  
  // set scroll position to y
  void scroll (const byte y = 0)
   {
    byte old_cs = _chipSelect;
    _chipSelect = LCD_CS1;
    cmd (LCD_DISP_START | (y & 0x3F) );  // set scroll position
    _chipSelect = LCD_CS2;
    cmd (LCD_DISP_START | (y & 0x3F) );  // set scroll position
    _chipSelect = old_cs;
   }
  
  size_t write(uint8_t c) {putch(c, _invmode); return 1; }

  void setInv(boolean inv) 
   {
    _invmode = inv; // set inverse mode state true == inverse
   }
   
  void textSize(const byte s = 1) 
   {
    _textsize = (s > 0 && s < 5) ? s : _textsize;
   }

  void selectFont(const byte * f)
   {
    font = f;
   }
};
