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

void setup()
{
  Serial.begin(BAUD_RATE);
  delay(1000);

  Serial.println("\n[STARTUP] LOLIN ESP8266 NodeMCU V3 Temperature Monitor");
  Serial.println("[STARTUP] Initializing modules...\n");

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

  Serial.println("[STARTUP] Ready\n");
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
  if (gsm.checkForIncomingCall())
  {
    Serial.println("[CALL] Incoming call - hanging up and sending status");
    display.displayMessage("CALL!\nSending status...");

    // Hang up the call
    gsm.hangupCall();
    delay(500);

    // Send status SMS with current temperature and state
    gsm.sendStatusSMS(tempSensor.getTemperature(), tempSensor.getState());

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
