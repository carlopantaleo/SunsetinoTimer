#include "WifiConfigurator.hpp"
#include "SunClock.hpp"
#include <WiFiUdp.h>
#include "NTPClient.hpp"

#define NTP_UPDATE_INTERVAL 60 * 60 * 1000

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
ESP8266WebServer webServer(80);
PlatformManager platformManager(D4, D1);
WifiConfigurator wifiConfigurator(&webServer, &platformManager);

void setup()
{
  pinMode(D4, OUTPUT);
  pinMode(D1, OUTPUT);
  wifiConfigurator.Setup();
  webServer.begin();
  timeClient.setUpdateInterval(NTP_UPDATE_INTERVAL);
  timeClient.begin();
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

void loop()
{
  houseKeeping();

  if (!wifiConfigurator.IsSetupMode()) // Normal operation
  {
    static int day = -1;
    static time_t set, rise;

    // Recalculate rise and set times once a day
    if (timeClient.getDay() != day)
    {
      day = timeClient.getDay();
      setRiseSetTimes(&rise, &set);
    }

    // Turn lamp on or off
    if (timeClient.getEpochTime() > (unsigned)set &&
        timeClient.getEpochTime() < (unsigned)rise)
    {
      platformManager.LampOn();
    }
    else
    {
      platformManager.LampOff();
    }
  }

  delay(10000);
}

void setRiseSetTimes(time_t *rise, time_t *set)
{
  Sunclock sunclock(44.3316998, 7.4774379); // TODO: retrieve location from EEPROM
  *set = sunclock.sunset(timeClient.getEpochTime());
  *rise = sunclock.sunrise(timeClient.getEpochTime() + 60 * 60 * 24); // Next day sunrise
  Serial.print(F("Current time (GMT): "));
  Serial.println(timeClient.getFormattedTime());
  printTime(*set);
  printTime(*rise);
}

void wifiCheck()
{
  if (!wifiConfigurator.CheckConnection())
  {
    platformManager.Blink(10, 50);
  }
  else
  {
    platformManager.Blink(3, 500);
  }
}

void houseKeeping()
{
  wifiConfigurator.HandleClient();
  webServer.handleClient();

  if (wifiConfigurator.IsSetupMode())
  {
    platformManager.Blink(3);
  }
  else if (!timeClient.update())
  {
    wifiCheck();
  }
}

/**
 * Only for debug, will be removed
 */
void printTime(time_t time)
{
  std::tm *ptm = std::localtime(&time);
  char buffer[32];
  memset(buffer, 0, sizeof(buffer) - 1);
  std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S (GMT)", ptm);
  Serial.println(buffer);
}
