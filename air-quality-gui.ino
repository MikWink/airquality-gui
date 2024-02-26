// IMPORTANT: Adafruit_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h FOR SETUP.

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <string.h>
#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>

// The used commands use up to 48 bytes. On some Arduino's the default buffer
// space is not large enough
#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) &&                 \
     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
    (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

SensirionI2CSen5x sen5x;

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define DARKBLUE 0x4AF1

#define USE_ADAFRUIT_SHIELD_PINOUT  true

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
// If using the shield, all control and data lines are fixed, and
// a simpler declaration can optionally be used:
// Adafruit_TFTLCD tft;


// Get the display dimensions
uint16_t screenWidth = tft.height();
uint16_t screenHeight = tft.width();

uint16_t sidebarWidth = screenWidth / 5;

uint16_t gridWidth = screenWidth - sidebarWidth;

// Define the number of rows and columns in the grid
const int rows = 2;
const int cols = 2;

// Define the number of rows in the sidebar
const int sidebarRows = 1;

uint16_t sidebarCell = screenHeight / sidebarRows;

// Calculate the size of each grid cell
uint16_t cellWidth = gridWidth / cols;
uint16_t cellHeight = screenHeight / rows;

// Set font size for values and labels
uint8_t fontSizeValues = 2;  // Adjust the value based on your preference
uint8_t fontSizeLabels = 3;  // Adjust the value based on your preference

uint8_t scaleTop = 0;
uint8_t scaleBot = 0;

uint8_t barSize = 0;


uint8_t tempArr[50];


void setup(void) {
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));

#ifdef USE_ADAFRUIT_SHIELD_PINOUT
  Serial.println(F("Using Adafruit 2.8\" TFT Arduino Shield Pinout"));
#else
  Serial.println(F("Using Adafruit 2.8\" TFT Breakout Board Pinout"));
#endif

  tft.reset();

  uint16_t identifier = tft.readID();
  identifier = 0x9341;

  if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    return;
  }

  Serial.print("width: ");
  Serial.print(screenWidth);
  Serial.print(", height: ");
  Serial.println(screenHeight);

  tft.begin(identifier);

  tft.fillScreen(BLACK);
  tft.setRotation(-1);
  tft.setCursor(0,0);

  drawAirQualityGrid();
  drawColorBar();
  drawColorScale(scaleTop, scaleBot);
  for(int i = 0; i < 50; i++){
    tempArr[i] = i;
  }
}

int count = 0;

void loop() {
  // Your main code goes here
  float temp = randomFloat(18.0, 24.0);
  int humid = random(50, 70);
  int co2 = random(5, 18);
  float voc = randomFloat(280.0, 330.0);
  int aq = random(60, 90);

  /*for(int i = 0; i < 50; i++){
    Serial.print(tempArr[i]);
    Serial.print(", ");
  }
  Serial.println();*/
  
  //addToFront(temp);
  

  if(count % 1500 == 0){
    updateValues(temp, humid, co2, voc);
    drawColorBar();
    updateMarker(aq);
  }

  count++;


  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) { 
    int y = p.y; 
    // scale from 0->1023 to tft.width
    p.y = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
    p.x = map(y, TS_MAXX, TS_MINX, 0, tft.width());
    

    if(p.x > 140 && p.x < 250 && p.y > 140){
        drawVOCGraph();      
    }else if(p.x < 140 && p.y > 140){
      drawCO2Graph();
    }else if(p.x < 140 && p.y < 140){
      drawTEMPGraph();
    }else if(p.x > 140 && p.x < 250 && p.y < 140){
      drawHUMIDGraph();
    }
    
  }


  delay(0);
}

void addToFront(int value) {
  // Shift existing elements to make room for the new element
  for (int i = 50; i > 0; --i) {
    tempArr[i] = tempArr[i - 1];
  }

  // Add the new element at the front
  tempArr[0] = value;

 
}

void drawVOCGraph(){
  tft.fillScreen(BLACK);
  tft.setCursor(10, 10);
  tft.print("VOC");
  delay(5000);
  tft.fillScreen(BLACK);
  drawAirQualityGrid();
  drawColorBar();
  drawColorScale(scaleTop, scaleBot);  
}

void drawCO2Graph(){
  tft.fillScreen(BLACK);
  tft.setCursor(10, 10);
  tft.print("CO2");
  delay(5000);
  tft.fillScreen(BLACK);
  drawAirQualityGrid();
  drawColorBar();
  drawColorScale(scaleTop, scaleBot);  
}

void drawHUMIDGraph(){
  tft.fillScreen(BLACK);
  tft.setCursor(10, 10);
  tft.print("Humidity");
  delay(5000);
  tft.fillScreen(BLACK);
  drawAirQualityGrid();
  drawColorBar();
  drawColorScale(scaleTop, scaleBot);  
}

void drawTEMPGraph(){
  tft.fillScreen(BLACK);
  tft.setCursor(10, 10);
  tft.print("Temperature");
  delay(5000);
  tft.fillScreen(BLACK);
  drawAirQualityGrid();
  drawColorBar();
  drawColorScale(scaleTop, scaleBot);  
}

void updateMarker(int aq){
  int y = map(aq, 0, 100, barSize, 0) + 10;
  int x0 = gridWidth + sidebarWidth / 2;
  int x1 = gridWidth + sidebarWidth / 2 + 20;

  tft.drawLine(x0, y, x1, y, WHITE);

}

void drawColorScale(int top, int bot){
  int x0 = gridWidth + sidebarWidth / 2 - 3;
  int x1 = gridWidth + sidebarWidth / 2 - 8;
  int y = bot;
  int loopEnd = bot - top;
  int loopEndRounded = loopEnd / 10;
  int label = 0;
  int count = 0;
  Serial.print("bot: ");
  Serial.print(bot);
  Serial.print(", top: ");
  Serial.print(top);
  Serial.print(", barsize: ");
  Serial.println(loopEnd);
  barSize = loopEnd;

  for(int i = 0; i < loopEnd + 1; i++){
    
    if(i % loopEndRounded == 0){
      if((float(loopEnd) / 10.0) * float(count) >= float(loopEnd / 10) * float(count) + 0.5){
        y++;
        
      }
      tft.drawLine(x0, y, x1, y, WHITE);
      tft.setTextSize(0.5);
      tft.setCursor(x1 - 19, y - 3);
      tft.print(label);
      label += 10;
      count++;
    }
    y--;
  }
}

void drawColorBar() {
  int r = 255;
  int g = 0;
  int x0 = gridWidth + sidebarWidth / 2;
  int x1 = gridWidth + sidebarWidth / 2 + 20;
  int y0 = screenHeight - 10;

  // Draw the color scale from bottom to top
  for (int i = 0; i < screenHeight - 19; i++) {
    uint16_t currentColor = tft.color565(
      r,
      g,
      50
    );
    r--;
    g++;

    // Draw a line at the current position with the calculated color
    tft.drawLine(x0, y0, x1, y0, currentColor);

    // Set top and bottom positions for scale
    if(i == 0){
      scaleBot = y0;
    }
    scaleTop = y0;
    
    y0--;
  }
}

void writeGridCell(String value, int row, int col, int xOffset, int yOffset, int size){
  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  
  int xPos = (cellWidth * col) + cellWidth / 2 + xOffset;
  int yPos = (cellHeight * row) + cellHeight / 2 + yOffset;
  tft.setTextSize(size);
  tft.getTextBounds(value, xPos, yPos, &x1, &y1, &textWidth, &textHeight);

  tft.setCursor(xPos - textWidth / 2, yPos - textHeight / 2);
  tft.print(value);  
}

void ereaseCell(int row, int col){
  int xPos = cellWidth * col;
  int yPos = cellHeight * row;
  // Erase old data
  tft.fillRect(xPos + 5, yPos + 20, cellWidth - 10, cellHeight - 40, BLACK);
}

void updateValues(float temp, int humid, int co2, float voc){

  String airQualityValues[][2] = {{String(temp), String(humid) + "%"}, {String(co2) + "%", String(voc)}};
  int cellCount = 0;
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      ereaseCell(row, col);
      writeGridCell(airQualityValues[row][col], row, col, 0, 0, 3);
      cellCount++;
    }
  }

}

void drawAirQualityGrid() {

  String airQualityValues[][2] = {{"Temperature", "Humidity"}, {"CO2", "VOC"}};
  int cellCount = 0;
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      // Calculate the position of the current grid cell
      uint16_t xPos = col * cellWidth;
      uint16_t yPos = row * cellHeight;

      // Draw a rectangle for each cell
      tft.drawRect(xPos, yPos, cellWidth, cellHeight, tft.color565(255, 255, 255));

      writeGridCell(airQualityValues[row][col], row, col, 0, cellHeight/2 - 10, 1);

      cellCount++;
    }
  }


  // Draw the sidebar
  for(int sidebarRow = 0; sidebarRow < sidebarRows; sidebarRow++){
    uint16_t xPos = screenWidth - sidebarWidth;
    uint16_t yPos = sidebarCell * sidebarRow;

    tft.drawRect(xPos, yPos, sidebarWidth, sidebarCell, tft.color565(255, 255, 255));
  }
}

// Function to generate a random float with one decimal place within a specified range [min, max]
float randomFloat(float min, float max) {
    // Generate a random integer within the specified range
    int randomInt = random(int(min * 10), int(max * 10));

    // Convert the random integer to a float with one decimal place
    float randomValue = float(randomInt) / 10.0;

    return randomValue;
}

