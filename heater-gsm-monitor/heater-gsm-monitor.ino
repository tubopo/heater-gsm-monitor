/*
 * LOLIN ESP8266 NodeMCU V3 GSM Temperature Monitor
 * ESP8266 + SIM800L + Dallas DS18B20 + SSD1306 Display + TTP223 Button
 * SMS alerts on temperature threshold violations
 * Display can be toggled with button, auto-off after 30 seconds
 */

#include "config.h"
#include "TemperatureSensor.h"
#include "GSMModule.h"
#include "DisplayModule.h"
#include "ButtonModule.h"

// Module instances
TemperatureSensor tempSensor;
GSMModule gsm;
DisplayModule display;
ButtonModule button;

// State tracking
unsigned long lastAlertTime = 0;
int lastAlertState = -1;
bool callBeingHandled = false;

void setup()
{
  Serial.begin(BAUD_RATE);
  delay(1000);

  // Initialize status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);

  // Initialize all modules
  tempSensor.begin();
  display.begin();
  button.begin();

  // Set button callback to toggle display
  button.setOnPressCallback([]()
                            { display.toggleDisplay();  return true; });

  // Initialize GSM modem
  gsm.begin();

  digitalWrite(STATUS_LED_PIN, LOW);
}

void loop()
{
  // Update all modules
  tempSensor.update();
  gsm.update();
  button.update();

  // Update display with current sensor data
  display.update(tempSensor.getTemperature(), tempSensor.getState(), gsm.getSignalQuality(), gsm.getOperator());

  // Check for incoming calls - drop and send status SMS
  bool callDetected = gsm.checkForIncomingCall();

  if (callDetected && !callBeingHandled)
  {
    display.displayMessage("CALL!\nSending status...");

    // Hang up the call
    gsm.hangupCall();
    delay(1000); // Wait for hangup to complete

    // Send status SMS with current temperature and state
    gsm.sendStatusSMS(tempSensor.getTemperature(), tempSensor.getState());
    // Mark that we're handling this call to prevent multiple SMS
    callBeingHandled = true;

    // Flash LED to indicate call was handled
    for (int i = 0; i < 3; i++)
    {
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(100);
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(100);
    }

    delay(2000); // Show message for 2 seconds
  }
  else if (!callDetected && callBeingHandled)
  {
    // Call has cleared, reset the flag
    callBeingHandled = false;
  }

  // Check if temperature state changed and send alert if needed
  if (tempSensor.hasStateChanged())
  {
    int currentState = tempSensor.getState();

    // Only send alert on non-normal states
    if (currentState != TEMP_STATE_NORMAL)
    {
      unsigned long timeSinceLastAlert = millis() - lastAlertTime;

      // Respect cooldown period
      if (timeSinceLastAlert >= ALERT_COOLDOWN || lastAlertState == -1)
      {
        gsm.sendAlert(tempSensor.getTemperature(), currentState);
        lastAlertTime = millis();
        lastAlertState = currentState;

        // Flash LED on alert
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(200);
        digitalWrite(STATUS_LED_PIN, LOW);
      }
    }
    else
    {
      lastAlertState = TEMP_STATE_NORMAL;
    }
  }

  delay(50); // Small delay to prevent CPU hogging
}
