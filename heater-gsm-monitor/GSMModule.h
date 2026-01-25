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

      String msg = "GSM: Testing\n" + String(testBaud) + " baud";
      display.displayMessage(msg.c_str());

      gsmSerial.begin(testBaud);
      delay(100);

      // Try AT command 3 times at this baud rate
      for (int i = 0; i < 3; i++)
      {
        modem.sendAT("");
        if (modem.waitResponse(1000) == 1)
        {
          String successMsg = "GSM: Found!\n" + String(testBaud) + " baud";
          display.displayMessage(successMsg.c_str());
          delay(2000);

          modemResponding = true;
          break;
        }
        delay(100);
      }
    }

    if (!modemResponding)
    {
      Serial.println("[GSM] ERROR: No baud rate worked!");
      display.displayMessage("GSM: FAILED\nAll baud rates\ntried!");
      delay(5000);
      isConnected = false;
      return;
    }

    // Initialize modem (this is important!)
    display.displayMessage("GSM: Modem init");
    if (!modem.init())
    {
      Serial.println("[GSM] ERROR: Modem init failed!");
      display.displayMessage("GSM: Init FAILED");
      delay(3000);
      isConnected = false;
      return;
    }

    // Get modem info
    String modemInfo = modem.getModemInfo();
    if (!modemInfo.isEmpty())
    {
      String msg = "GSM: " + modemInfo;
      display.displayMessage(msg.c_str());
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
        Serial.println("[GSM] ERROR: SIM unlock failed!");
        display.displayMessage("GSM: SIM UNLOCK\nFAILED!");
        delay(3000);
      }
      delay(2000);
    }

    // Wait for network registration with detailed feedback
    display.displayMessage("GSM: Wait network\n30-60 sec...");

    if (!modem.waitForNetwork(60000L))
    {
      Serial.println("[GSM] ERROR: Network registration failed!");
      display.displayMessage("GSM: NO NETWORK\nCheck SIM/2G");
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
      String opMsg = "GSM: " + operatorName;
      display.displayMessage(opMsg.c_str());
      delay(2000);
    }

    // Get initial signal quality
    checkSignal();

    // Show signal on display
    String signalMsg = "GSM: Signal ";
    signalMsg += signalQuality;
    signalMsg += "/31";
    display.displayMessage(signalMsg.c_str());
    delay(2000);

    if (signalQuality == 99)
    {
      Serial.println("[GSM] WARNING: No signal detected!");
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

    String message = "ALERT: ";

    if (tempState == TEMP_STATE_CRITICAL)
    {
      message += (temperature <= TEMP_CRITICAL_LOW) ? "CRITICAL LOW" : "CRITICAL HIGH";
    }
    else
    {
      message += (temperature < TEMP_MIN) ? "WARNING LOW" : "WARNING HIGH";
    }

    message += " ";
    message += temperature;
    message += "C";

    return modem.sendSMS(PHONE_NUMBER, message);
  }

  bool sendStatusSMS(float temperature, int tempState)
  {
    if (!isConnected)
    {
      return false;
    }

    // Build status message
    String message = "Temp: ";
    message += temperature;
    message += "C\n";

    message += "Status: ";
    if (tempState == TEMP_STATE_CRITICAL)
      message += "CRITICAL";
    else if (tempState == TEMP_STATE_WARNING)
      message += "WARNING";
    else
      message += "NORMAL";

    message += "\nSignal: ";
    message += signalQuality;
    message += "/31";

    return modem.sendSMS(PHONE_NUMBER, message);
  }

  int getSignalQuality() const { return signalQuality; }
  bool isModemConnected() const { return isConnected; }
  String getOperator() const { return operatorName; }
};

#endif // GSM_MODULE_H
