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
int alertCount = 0; // Track number of alerts sent for current problem
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
    // Get fresh temperature if needed (skips if reading is < 5 minutes old)
    tempSensor.forceRead();

    display.displayMessage("CALL!\nSending status...");

    // Hang up the call
    gsm.hangupCall();
    delay(500); // Wait for hangup to complete

    // Send status SMS with current temperature and state
    gsm.sendStatusSMS(tempSensor.getTemperature(), tempSensor.getState());
    callBeingHandled = true;

    // Flash LED to indicate call was handled
    for (int i = 0; i < 3; i++)
    {
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(100);
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(100);
    }

    delay(1500); // Show message briefly
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

      // Handle millis() overflow
      if (millis() < lastAlertTime)
      {
        lastAlertTime = millis();
        timeSinceLastAlert = ALERT_COOLDOWN;
      }

      // DISCONNECTED state: send one alert, then wait for recovery
      if (currentState == TEMP_STATE_DISCONNECTED)
      {
        // Only send alert on first disconnection detection
        if (lastAlertState != TEMP_STATE_DISCONNECTED && (timeSinceLastAlert >= ALERT_COOLDOWN || lastAlertState == -1))
        {
          gsm.sendAlert(tempSensor.getTemperature(), currentState);
          lastAlertTime = millis();
          lastAlertState = currentState;

          // Flash LED rapidly for sensor error
          for (int i = 0; i < 5; i++)
          {
            digitalWrite(STATUS_LED_PIN, HIGH);
            delay(100);
            digitalWrite(STATUS_LED_PIN, LOW);
            delay(100);
          }
        }
        else
        {
          lastAlertState = currentState;
        }
      }
      // WARNING/CRITICAL states: normal alert logic with max count
      else if (alertCount < MAX_ALERTS_PER_EVENT && (timeSinceLastAlert >= ALERT_COOLDOWN || lastAlertState == -1))
      {
        gsm.sendAlert(tempSensor.getTemperature(), currentState);
        lastAlertTime = millis();
        lastAlertState = currentState;
        alertCount++;

        // Flash LED on alert
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(200);
        digitalWrite(STATUS_LED_PIN, LOW);
      }
      else
      {
        // Just update state tracking without sending alert
        lastAlertState = currentState;
      }
    }
    else
    {
      // Temperature/sensor returned to normal - reset counters
      lastAlertState = TEMP_STATE_NORMAL;
      alertCount = 0;
    }
  }

  delay(50); // Small delay to prevent CPU hogging
}
