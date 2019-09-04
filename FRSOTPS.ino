//https://create.arduino.cc/projecthub/SurtrTech/display-bmp-pictures-from-sd-card-on-tft-lcd-shield-f3074c

#include <Adafruit_GFX.h>
#include <Adafruit_TFTLCD.h>
#include <SD.h>
#include <SPI.h>
#include <MCUFRIEND_kbv.h>
#include <Fonts/FGTTest.h>
#include <SoftwareSerial.h>
#include <stdlib.h>
#include "VdoTemperatureSender.h"
#include "VdoPressureSender.h"

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define SD_CS 53 // Adafruit SD shields and modules: pin 10
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define WHITE 0xFFFF
#define BUFFPIXEL 80
#define BUFFPIXEL_X3  240

MCUFRIEND_kbv tft(A3, A2, A1, A0, A4);

void* operator new(size_t size) {
  return malloc(size);
}
void operator delete(void* ptr) {
  free(ptr);
}

// Oil Pressure Table
unsigned int vdoPressure[] =
{
  61,
  160,
  231,
  280,
  320,
  349,
  374,
  394,
  411,
  425,
  437
};

// Oil Temperature Table
/**
   Ref: 60ºC - 430 Ohms |  150ºC - 33 Ohms
*/
unsigned int vdoTemp[] =
{
  644,
  628,
  599,
  560,
  509,
  450,
  385,
  322,
  261,
  210,
  168,
  133,
  104,
  81,
  68,
  58
};

// Water Temperature Table
/**
   Each line is a 10ºC increase, starting from 0ºC to 150ºC
*/
unsigned int agressiveTemp[] =
{
  656,
  647,
  630,
  609,
  575,
  537,
  489,
  421,
  345,
  319,
  283,
  263,
  252,
  246,
  243,
  242
};



VdoPressureSender *oilPressureSensor;
VdoTemperatureSender *waterTemperatureSensor;
VdoTemperatureSender *oilTemperatureSensor;


void setup(void) {
  Serial.begin(9600);
  tft.reset();
  uint16_t identifier = tft.readID();
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  tft.begin(identifier);
  tft.setRotation(3);
  if (!SD.begin(SD_CS)) {
    progmemPrintln(PSTR("failed!"));
    return;
  }

  bmpDraw("01.bmp", 0, 0);
  // Initialize car sensors
  oilPressureSensor = new VdoPressureSender(A15, vdoPressure); //green
  waterTemperatureSensor = new VdoTemperatureSender(A13, agressiveTemp); //blue
  oilTemperatureSensor = new VdoTemperatureSender(A14, vdoTemp); //yellow

}

void loop(void) {


  tft.setTextColor(WHITE);
  tft.setFont(&c__users_Xcon_Desktop_UnitedSansReg_Medium15pt7b);
  //oil pres
  double oilPressure = oilPressureSensor->getPressure();
  tft.setCursor(325, 245);
  tft.println(oilPressure);
  //oil temp
  int oilTemperature = oilTemperatureSensor->getTemperature();
  tft.setCursor(325, 195);
  tft.println(oilTemperature);
  //coolant temp
  int waterTemperature = waterTemperatureSensor->getTemperature();
  tft.setCursor(325, 145);
  tft.println(waterTemperature);

  tft.setCursor(10, 30);
  tft.println("Build No. 1506");

}


void bmpDraw(char *filename, int x, int y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel in buffer (R+G+B per pixel)
  uint16_t lcdbuffer[BUFFPIXEL];  // pixel out buffer (16-bit per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t  lcdidx = 0;
  boolean  first = true;

  if ((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  progmemPrint(PSTR("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');
  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    progmemPrintln(PSTR("File not found"));
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    progmemPrint(PSTR("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    progmemPrint(PSTR("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    progmemPrint(PSTR("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      progmemPrint(PSTR("Bit Depth: ")); Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        progmemPrint(PSTR("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);

        for (row = 0; row < h; row++) { // For each scanline...
          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col = 0; col < w; col++) { // For each column...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              // Push LCD buffer to the display first
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first  = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r, g, b);
          } // end pixel
        } // end scanline
        // Write any remaining data to LCD
        if (lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        }
        progmemPrint(PSTR("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) progmemPrintln(PSTR("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// Copy string from flash to serial port
// Source string MUST be inside a PSTR() declaration!
void progmemPrint(const char *str) {
  char c;
  while (c = pgm_read_byte(str++)) Serial.print(c);
}

// Same as above, with trailing newline
void progmemPrintln(const char *str) {
  progmemPrint(str);
  Serial.println();
}
