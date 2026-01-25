#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

class DisplayModule
{
private:
  Adafruit_SSD1306 display;
  bool isOn;
  bool isInitialized;
  unsigned long displayOnTime;
  unsigned long lastUpdateTime;

public:
  DisplayModule() : display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, -1), isOn(true), isInitialized(false), displayOnTime(millis()), lastUpdateTime(0) {}

  void begin()
  {
    // Initialize I2C on custom pins
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS))
    {
      isInitialized = false;
      return;
    }

    isInitialized = true;
    isOn = true;
    displayOnTime = millis();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("[DISPLAY] Ready");
    display.display();
  }

  void update(float temperature, int tempState, int signalQuality, String operatorName = "")
  {
    if (!isInitialized) return;

    unsigned long now = millis();

    // Auto-off after timeout
    if (isOn && (now - displayOnTime >= DISPLAY_TIMEOUT))
    {
      turnOff();
    }

    // Update display at regular interval when on
    if (isOn && (now - lastUpdateTime >= DISPLAY_UPDATE_INTERVAL))
    {
      lastUpdateTime = now;
      drawScreen(temperature, tempState, signalQuality, operatorName);
    }
  }

  void toggleDisplay()
  {
    if (isOn)
    {
      turnOff();
    }
    else
    {
      turnOn();
    }
  }

  void turnOn()
  {
    if (!isInitialized) return;

    if (!isOn)
    {
      isOn = true;
      displayOnTime = millis();
      display.ssd1306_command(SSD1306_DISPLAYON);
    }
    else
    {
      // Display already on, just reset timeout
      displayOnTime = millis();
    }
  }

  void turnOff()
  {
    if (!isInitialized) return;

    if (isOn)
    {
      isOn = false;
      display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
  }

  bool isDisplayOn() const { return isOn; }

  void displayMessage(const char *message)
  {
    if (!isInitialized) return;

    // Turn on display and reset timeout when showing messages
    turnOn();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(message);
    display.display();
  }

private:
  void drawScreen(float temperature, int tempState, int signalQuality, String operatorName = "")
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Yellow section (top 16 pixels): Temperature + Status
    display.setCursor(0, 4);
    display.print("TEMP:");
    display.setTextSize(2);
    display.setCursor(40, 0);
    display.print(temperature, 1);
    display.setTextSize(1);
    display.setCursor(95, 4);
    display.print("C");

    // Status indicator in yellow section
    display.setCursor(105, 4);
    if (tempState == TEMP_STATE_CRITICAL)
      display.print("!");
    else if (tempState == TEMP_STATE_WARNING)
      display.print("*");
    else
      display.print("");

    // Blue section starts at Y=16
    // Draw separator line
    display.drawLine(0, 16, 127, 16, SSD1306_WHITE);

    // Carrier/Operator (if available)
    if (operatorName.length() > 0)
    {
      display.setCursor(0, 20);
      display.print("Carrier: ");
      // Truncate operator name if too long
      String shortOp = operatorName.substring(0, min((int)operatorName.length(), 10));
      display.println(shortOp);
    }

    // Signal strength with bar
    display.setCursor(0, 30);
    display.print("Signal:");
    display.setCursor(50, 30);
    display.print(signalQuality);
    display.print("/31");

    // Signal bar (larger and more visible)
    drawSignalBar(signalQuality);

    // Status text
    display.setCursor(0, 52);
    if (tempState == TEMP_STATE_CRITICAL)
      display.print("STATUS: CRITICAL!");
    else if (tempState == TEMP_STATE_WARNING)
      display.print("STATUS: WARNING");
    else
      display.print("STATUS: NORMAL");

    display.display();
  }

  void drawSignalBar(int quality)
  {
    // Draw signal strength indicator (0-31) at Y=40
    // Make it larger and more visible
    int barWidth = (quality * DISPLAY_WIDTH) / 31;

    // Outer rectangle
    display.drawRect(0, 40, DISPLAY_WIDTH, 10, SSD1306_WHITE);

    // Fill based on signal strength
    if (barWidth > 0)
    {
      display.fillRect(1, 41, barWidth - 1, 8, SSD1306_WHITE);
    }
  }
};

#endif // DISPLAY_MODULE_H
