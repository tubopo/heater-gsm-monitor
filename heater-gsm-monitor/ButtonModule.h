#ifndef BUTTON_MODULE_H
#define BUTTON_MODULE_H

#include "config.h"

class ButtonModule
{
private:
  int buttonPin;
  bool lastState;
  bool currentState;
  unsigned long lastDebounceTime;
  const unsigned long debounceDelay = 50; // 50ms debounce
  bool (*onPressCallback)() = nullptr;

public:
  ButtonModule() : buttonPin(BUTTON_PIN), lastState(HIGH), currentState(HIGH), lastDebounceTime(0) {}

  void begin()
  {
    pinMode(buttonPin, INPUT_PULLUP);
  }

  void update()
  {
    int reading = digitalRead(buttonPin);

    // Debounce logic
    if (reading != lastState)
    {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
      if (reading != currentState)
      {
        currentState = reading;

        // Trigger on button press (LOW = pressed for most touch buttons)
        if (currentState == LOW)
        {
          onPressed();
        }
      }
    }

    lastState = reading;
  }

  void setOnPressCallback(bool (*callback)())
  {
    onPressCallback = callback;
  }

  bool isPressed() const { return currentState == LOW; }

private:
  void onPressed()
  {
    if (onPressCallback)
    {
      onPressCallback();
    }
  }
};

#endif // BUTTON_MODULE_H
