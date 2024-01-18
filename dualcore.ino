#include <Adafruit_GFX.h>       // Graphics library
#include <Adafruit_NeoPixel.h>  // NeoPixel library
#include <Adafruit_NeoMatrix.h> // Bridges GFX and NeoPixel
#include <Fonts/TomThumb.h>     // A tiny 3x5 font incl. w/GFX
#include <WiFi.h>               // Include the WiFi library

#define PIN 27

Adafruit_NeoMatrix matrix(5, 5, PIN,
  NEO_MATRIX_TOP  + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE
  );

volatile int networkCount = 0;
volatile bool isNewScanAvailable = false; // Flag for new scan availability
char message[10]; // Buffer for the WiFi count message
int x, y;
uint16_t message_width;

// Task for WiFi Scanning
void scanWiFiTask(void *parameter) {
  for(;;) { // Infinite loop
    networkCount = WiFi.scanNetworks();
    snprintf(message, sizeof(message), "%d", networkCount);
    WiFi.scanDelete();
    isNewScanAvailable = true; // Indicate new scan is available
    delay(10000); // Delay between scans
  }
}

void playAnimation() {
  int width = matrix.width();
  int height = matrix.height();

  while (!isNewScanAvailable) { // Continue animation until a new scan is available
    // Move pixel along the top edge
    for (int x = 0; x < width; ++x) {
      if (isNewScanAvailable) return;
      matrix.fillScreen(0);
      matrix.drawPixel(x, 0, matrix.Color(255, 255, 255));
      matrix.show();
      delay(200);
    }

    // Move pixel down the right edge
    for (int y = 1; y < height; ++y) {
      if (isNewScanAvailable) return;
      matrix.fillScreen(0);
      matrix.drawPixel(width - 1, y, matrix.Color(255, 255, 255));
      matrix.show();
      delay(200);
    }

    // Move pixel along the bottom edge
    for (int x = width - 2; x >= 0; --x) {
      if (isNewScanAvailable) return;
      matrix.fillScreen(0);
      matrix.drawPixel(x, height - 1, matrix.Color(255, 255, 255));
      matrix.show();
      delay(200);
    }

    // Move pixel up the left edge
    for (int y = height - 2; y > 0; --y) {
      if (isNewScanAvailable) return;
      matrix.fillScreen(0);
      matrix.drawPixel(0, y, matrix.Color(255, 255, 255));
      matrix.show();
      delay(200);
    }
  }
}

// Task for Scrolling Message
void scrollMessageTask(void *parameter) {
  for(;;) { // Infinite loop
    if (isNewScanAvailable) {
      x = matrix.width(); // Reset position for new scroll
      isNewScanAvailable = false; // Reset flag

      while(x >= -message_width) {
        matrix.fillScreen(0);
        matrix.setCursor(x, y);
        matrix.print(message);
        matrix.show();
        x--; // Move the text to the left
        delay(100); // Speed of scrolling
      }
      delay(400); // Delay after displaying the number

      // Play animation
      playAnimation();
    }
    delay(100); // Small delay to prevent task from hogging CPU
  }
}


void setup() {
  matrix.begin();
  matrix.setBrightness(40);
  matrix.setFont(&TomThumb);
  matrix.setTextWrap(false);
  matrix.setTextColor(matrix.Color(0, 0, 255));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  x = matrix.width();
  y = matrix.height();

  // Start WiFi Scanning on Core 0
  xTaskCreatePinnedToCore(
    scanWiFiTask, "ScanWiFiTask", 10000, NULL, 1, NULL, 0
  );

  // Start Scrolling Message on Core 1
  xTaskCreatePinnedToCore(
    scrollMessageTask, "ScrollMessageTask", 10000, NULL, 1, NULL, 1
  );
}

void loop() {
  // Empty loop
}
