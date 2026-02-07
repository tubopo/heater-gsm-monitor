#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "DisplayModule.h"

// Forward declaration of global display instance
extern DisplayModule display;

class TemperatureSensor
{
private:
  OneWire oneWire;
  DallasTemperature sensor;
  float currentTemp;
  int currentState;
  int lastState;
  unsigned long lastReadTime;
  unsigned long lastRetryTime;
  bool sensorInitialized;
  int consecutiveErrors;

public:
  TemperatureSensor() : oneWire(TEMP_SENSOR_PIN), sensor(&oneWire), currentTemp(0.0), currentState(TEMP_STATE_NORMAL), lastState(-1), lastReadTime(0), lastRetryTime(0), sensorInitialized(false), consecutiveErrors(0) {}

  void begin()
  {
    sensor.begin();

    if (sensor.getDeviceCount() > 0)
    {
      sensor.setResolution(12);

      // DS18B20 returns 85°C as power-on default
      // Do first conversion and discard to clear power-on register value
      sensor.requestTemperatures();
      delay(750); // Wait for 12-bit conversion (750ms max)
      sensor.getTempCByIndex(0); // Discard first reading

      // Now do a second conversion to get real initial temperature
      // If it's still 85°C after this, it's either real or sensor is faulty
      sensor.requestTemperatures();
      delay(750);
      float initialReading = sensor.getTempCByIndex(0);

      // Validate initial reading (but accept 85°C if it persists - might be real)
      if (initialReading != DEVICE_DISCONNECTED_C &&
          initialReading >= TEMP_VALID_MIN &&
          initialReading <= TEMP_VALID_MAX)
      {
        currentTemp = initialReading;
        sensorInitialized = true;
        consecutiveErrors = 0;
      }
      else
      {
        // Sensor not responding properly
        currentTemp = 0.0;
        sensorInitialized = false;
      }
    }
    else
    {
      sensorInitialized = false;
      currentTemp = 0.0;
    }
  }

  void update()
  {
    unsigned long now = millis();

    // Handle millis() overflow (every 49 days)
    if (now < lastReadTime)
    {
      lastReadTime = now;
    }
    if (now < lastRetryTime)
    {
      lastRetryTime = now;
    }

    // Try to reinitialize sensor if disconnected (silent retries for autonomous operation)
    if (!sensorInitialized && (now - lastRetryTime >= SENSOR_RETRY_INTERVAL))
    {
      lastRetryTime = now;
      begin(); // Retry initialization silently
      if (sensorInitialized)
      {
        // Show success message only when reconnected
        display.displayMessage("Sensor:\nReconnected!");
        delay(1500);
      }
    }

    // Read temperature at regular interval
    if (now - lastReadTime >= SENSOR_READ_INTERVAL)
    {
      lastReadTime = now;
      readTemperature();
    }
  }

  float getTemperature() const { return currentTemp; }
  int getState() const { return currentState; }
  int getLastState() const { return lastState; }
  bool isDisconnected() const { return currentState == TEMP_STATE_DISCONNECTED; }
  bool isInitialized() const { return sensorInitialized; }

  bool hasStateChanged() const { return currentState != lastState; }

  // Force immediate temperature reading (e.g., for incoming call status)
  // If last read was recent (within 5 minutes), skip to avoid blocking
  void forceRead()
  {
    unsigned long now = millis();
    unsigned long timeSinceLastRead = now - lastReadTime;

    // Handle millis() overflow
    if (now < lastReadTime)
    {
      readTemperature();
      return;
    }

    // Only force read if last reading is older than 5 minutes
    // This avoids 750ms blocking when recent data is available
    if (timeSinceLastRead > 300000) // 5 minutes
    {
      readTemperature();
    }
    // Otherwise, current temperature is fresh enough
  }

private:
  void readTemperature()
  {
    if (!sensorInitialized)
    {
      lastState = currentState;
      currentState = TEMP_STATE_DISCONNECTED;
      currentTemp = DEVICE_DISCONNECTED_C;
      return;
    }

    sensor.requestTemperatures();
    delay(750); // DS18B20 12-bit conversion takes up to 750ms
    float reading = sensor.getTempCByIndex(0);

    // Check for sensor disconnection or invalid reading
    if (reading == DEVICE_DISCONNECTED_C || reading < TEMP_VALID_MIN || reading > TEMP_VALID_MAX)
    {
      consecutiveErrors++;

      // Show message only on first disconnection
      if (lastState != TEMP_STATE_DISCONNECTED)
      {
        display.displayMessage("Sensor:\nDISCONNECTED!");
        delay(2000);
      }

      lastState = currentState;
      currentState = TEMP_STATE_DISCONNECTED;
      currentTemp = DEVICE_DISCONNECTED_C;
      sensorInitialized = false;  // Mark for retry
      return;
    }

    // Valid reading - reset error counter
    if (consecutiveErrors > 0)
    {
      consecutiveErrors = 0;
    }

    currentTemp = reading;
    lastState = currentState;

    // Evaluate temperature state
    if (currentTemp <= TEMP_CRITICAL_LOW || currentTemp >= TEMP_CRITICAL_HIGH)
    {
      currentState = TEMP_STATE_CRITICAL;
    }
    else if (currentTemp < TEMP_MIN || currentTemp > TEMP_MAX)
    {
      currentState = TEMP_STATE_WARNING;
    }
    else
    {
      currentState = TEMP_STATE_NORMAL;
    }
  }
};

#endif // TEMPERATURE_SENSOR_H
