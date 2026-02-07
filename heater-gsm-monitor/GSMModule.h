#ifndef GSM_MODULE_H
#define GSM_MODULE_H

#include <TinyGsmClient.h>
#include <SoftwareSerial.h>
#include "config.h"
#include "DisplayModule.h"

// Forward declaration of global display instance
extern DisplayModule display;

class GSMModule
{
private:
  SoftwareSerial gsmSerial;
  TinyGsm modem;
  int signalQuality;
  unsigned long lastSignalCheckTime;
  bool isConnected;
  String operatorName;

public:
  GSMModule() : gsmSerial(SIM800_RX_PIN, SIM800_TX_PIN), modem(gsmSerial), signalQuality(0), lastSignalCheckTime(0), isConnected(false), operatorName("") {}

  void begin()
  {
    display.displayMessage("GSM: Init...");

    // Wait for modem to auto-power-on (most SIM800L modules auto-power when VCC applied)
    display.displayMessage("GSM: Waiting boot\n(5 sec)");
    delay(5000); // Give modem time to boot up

    // Try autobaud detection - test multiple baud rates
    display.displayMessage("GSM: Auto-detect\nbaud rate...");

    bool modemResponding = false;
    unsigned long baudRates[] = {9600, 115200, 57600, 38400, 19200, 4800};
    int numBaudRates = 6;

    for (int b = 0; b < numBaudRates && !modemResponding; b++)
    {
      unsigned long testBaud = baudRates[b];

      char msg[32];
      snprintf(msg, sizeof(msg), "GSM: Testing\n%lu baud", testBaud);
      display.displayMessage(msg);

      gsmSerial.begin(testBaud);
      delay(100);

      // Try AT command 3 times at this baud rate
      for (int i = 0; i < 3; i++)
      {
        modem.sendAT("");
        if (modem.waitResponse(1000) == 1)
        {
          snprintf(msg, sizeof(msg), "GSM: Found!\n%lu baud", testBaud);
          display.displayMessage(msg);
          delay(2000);

          modemResponding = true;
          break;
        }
        delay(100);
      }
    }

    if (!modemResponding)
    {
      display.displayMessage("GSM: FAILED\nNo response!");
      delay(5000);
      isConnected = false;
      return;
    }

    // Initialize modem (this is important!)
    display.displayMessage("GSM: Modem init");
    if (!modem.init())
    {
      display.displayMessage("GSM: Init FAILED");
      delay(3000);
      isConnected = false;
      return;
    }

    // Get modem info
    String modemInfo = modem.getModemInfo();
    if (!modemInfo.isEmpty())
    {
      char msg[48];
      snprintf(msg, sizeof(msg), "GSM: %.32s", modemInfo.c_str());
      display.displayMessage(msg);
      delay(2000);
    }

    // Disable modem echo
    modem.sendAT(GF("E0"));
    modem.waitResponse();

    // Check SIM status
    display.displayMessage("GSM: Check SIM");
    int simStatus = modem.getSimStatus();
    if (simStatus != SIM_READY)
    {
      display.displayMessage("GSM: Unlock SIM");
      if (!modem.simUnlock(SIM_PIN))
      {
        display.displayMessage("GSM: SIM UNLOCK\nFAILED!");
        delay(3000);
      }
      delay(2000);
    }

    // Wait for network registration with detailed feedback
    display.displayMessage("GSM: Wait network\n30-60 sec...");

    if (!modem.waitForNetwork(60000L))
    {
      display.displayMessage("GSM: NO NETWORK\nCheck SIM/antenna");
      delay(5000);
      isConnected = false;
      return;
    }

    display.displayMessage("GSM: Registered!");
    delay(1000);
    isConnected = true;

    // Get operator name
    operatorName = modem.getOperator();
    if (operatorName.length() > 0)
    {
      char opMsg[32];
      snprintf(opMsg, sizeof(opMsg), "GSM: %.20s", operatorName.c_str());
      display.displayMessage(opMsg);
      delay(2000);
    }

    // Get initial signal quality
    checkSignal();

    // Show signal on display
    char signalMsg[32];
    snprintf(signalMsg, sizeof(signalMsg), "GSM: Signal %d/31", signalQuality);
    display.displayMessage(signalMsg);
    delay(2000);

    if (signalQuality == 99)
    {
      display.displayMessage("GSM: NO SIGNAL!\nCheck antenna");
      delay(3000);
    }

    // Final success message
    display.displayMessage("GSM: Ready!");
    delay(1000);
  }

  void update()
  {
    unsigned long now = millis();
    if (now - lastSignalCheckTime >= GSM_CHECK_INTERVAL)
    {
      lastSignalCheckTime = now;
      checkSignal();
    }
  }

  bool checkForIncomingCall()
  {
    // Check for incoming call using AT+CLCC (List Current Calls)
    modem.sendAT(GF("+CLCC"));
    String response = "";
    if (modem.waitResponse(1000, response) == 1)
    {
      // Response format: +CLCC: <id>,<dir>,<stat>,<mode>,<mpty>
      // dir=1 means mobile terminated (incoming)
      // stat=4 means incoming call (not yet answered)
      if (response.indexOf("+CLCC:") >= 0 && response.indexOf(",1,4,") >= 0)
      {
        return true;
      }
    }
    return false;
  }

  void hangupCall()
  {
    modem.sendAT(GF("ATH")); // ATH = Hang up command
    modem.waitResponse(1000);
    delay(500);
    // Also send CHUP as backup
    modem.sendAT(GF("+CHUP")); // Alternative hang up command
    modem.waitResponse(1000);
  }

  void checkSignal()
  {
    signalQuality = modem.getSignalQuality();
  }

  bool sendAlert(float temperature, int tempState)
  {
    if (!isConnected)
    {
      return false;
    }

    // Use char buffer to avoid String memory fragmentation
    char message[80];

    if (tempState == TEMP_STATE_DISCONNECTED)
    {
      snprintf(message, sizeof(message), "ALERT: SENSOR DISCONNECTED");
    }
    else if (tempState == TEMP_STATE_CRITICAL)
    {
      const char* alertType = (temperature <= TEMP_CRITICAL_LOW) ? "CRITICAL LOW" : "CRITICAL HIGH";
      snprintf(message, sizeof(message), "ALERT: %s %.1fC", alertType, temperature);
    }
    else
    {
      const char* alertType = (temperature < TEMP_MIN) ? "WARNING LOW" : "WARNING HIGH";
      snprintf(message, sizeof(message), "ALERT: %s %.1fC", alertType, temperature);
    }

    return modem.sendSMS(PHONE_NUMBER, message);
  }

  bool sendStatusSMS(float temperature, int tempState)
  {
    if (!isConnected)
    {
      return false;
    }

    // Use char buffer to avoid String memory fragmentation
    char message[100];
    const char* statusText;

    if (tempState == TEMP_STATE_DISCONNECTED)
      statusText = "DISCONNECTED";
    else if (tempState == TEMP_STATE_CRITICAL)
      statusText = "CRITICAL";
    else if (tempState == TEMP_STATE_WARNING)
      statusText = "WARNING";
    else
      statusText = "NORMAL";

    if (tempState == TEMP_STATE_DISCONNECTED)
    {
      snprintf(message, sizeof(message), "Temp: SENSOR ERROR\nStatus: %s\nSignal: %d/31", statusText, signalQuality);
    }
    else
    {
      snprintf(message, sizeof(message), "Temp: %.1fC\nStatus: %s\nSignal: %d/31", temperature, statusText, signalQuality);
    }

    return modem.sendSMS(PHONE_NUMBER, message);
  }

  int getSignalQuality() const { return signalQuality; }
  bool isModemConnected() const { return isConnected; }
  String getOperator() const { return operatorName; }
};

#endif // GSM_MODULE_H
