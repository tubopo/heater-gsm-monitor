#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

class TemperatureSensor
{
private:
  OneWire oneWire;
  DallasTemperature sensor;
  float currentTemp;
  int currentState;
  int lastState;
  unsigned long lastReadTime;

public:
  TemperatureSensor() : oneWire(TEMP_SENSOR_PIN), sensor(&oneWire), currentTemp(0.0), currentState(TEMP_STATE_NORMAL), lastState(-1), lastReadTime(0) {}

  void begin()
  {
    Serial.println("[SENSOR] Initializing DS18B20...");
    sensor.begin();

    if (sensor.getDeviceCount() > 0)
    {
      sensor.setResolution(12);
      Serial.println("[SENSOR] Ready");
    }
    else
    {
      Serial.println("[SENSOR] ERROR: Not found!");
    }
  }

  void update()
  {
    unsigned long now = millis();
    if (now - lastReadTime >= SENSOR_READ_INTERVAL)
    {
      lastReadTime = now;
      readTemperature();
    }
  }

  float getTemperature() const { return currentTemp; }
  int getState() const { return currentState; }
  int getLastState() const { return lastState; }

  bool hasStateChanged() const { return currentState != lastState; }

private:
  void readTemperature()
  {
    sensor.requestTemperatures();
    delay(50); // Shorter delay for DS18B20
    currentTemp = sensor.getTempCByIndex(0);

    if (currentTemp == DEVICE_DISCONNECTED_C)
    {
      Serial.println("[TEMP] ERROR: Sensor disconnected");
      currentState = TEMP_STATE_CRITICAL;
      return;
    }

    lastState = currentState;

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

    // Debug output
    Serial.print("[TEMP] ");
    Serial.print(currentTemp, 1);
    Serial.print("C ");

    if (currentState == TEMP_STATE_CRITICAL)
      Serial.println("(CRITICAL)");
    else if (currentState == TEMP_STATE_WARNING)
      Serial.println("(WARNING)");
    else
      Serial.println("(NORMAL)");
  }
};

#endif // TEMPERATURE_SENSOR_H
