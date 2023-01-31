#include "functions_display.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#define TFT_CS 49  // Yellow
#define TFT_DC 48  // Green
#define TFT_RST 47 // Blue

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setupDisplay() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
}

void tftUpdateDisplay(int engineTemp, int oilTemp, int fanDuty, int vehiclespeed, int engineRpm, float best0To100,
                      float best80To120, int electronicstemp) {
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.print("Eng Temp ");
  tft.print(engineTemp);
  tft.print(char(247));
  tft.println("C");
  tft.print("Oil Temp ");
  tft.print(oilTemp);
  tft.print(char(247));
  tft.println("C");
  tft.print("Fan Duty ");
  tft.print(fanDuty);
  tft.println("%  ");
  tft.print("\nSpeed ");
  tft.print(vehiclespeed);
  tft.println(" kph    ");
  tft.print("RPM ");
  tft.print(engineRpm);
  tft.println("     ");
  tft.print("ECU Temp ");
  tft.print(electronicstemp);
  tft.print(char(247));
  tft.println("C");
  tft.setTextSize(1);
  tft.print("Best 0-100 time ");
  tft.print(best0To100 / 1000);
  tft.println("s    ");
  tft.print("Best 80-120 time ");
  tft.print(best80To120 / 1000);
  tft.println("s    ");
}
