#include "WifiConfigurator.hpp"
#include "SunClock.hpp"
#include <WiFiUdp.h>
#include <NTPClient.h>

#define NTP_UPDATE_INTERVAL 60*60*1000

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

  delay(2000);
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

void wifiHousekeeping(bool forceReset = false)
{
  static unsigned long lastConnection = 0;
  if (forceReset)
    lastConnection = millis(); // Force reconnect

  // Connections will be kept alive for 5 minutes, then WiFi will be turned off for power saving.
  if (millis() - lastConnection < 5 * 60 * 1000)
  {
    // Awake WiFi if it was sleeping
    if (WiFi.getMode() == WIFI_OFF)
    {
      WiFi.forceSleepWake();
      delay(1);
      WiFi.mode(WIFI_STA);
      WiFi.begin();
      lastConnection = millis();
    }

    if (!wifiConfigurator.CheckConnection())
    {
      platformManager.Blink(10, 20);
      lastConnection = millis(); // Reset last connection timer
    }
    else
    {
      platformManager.Blink();
    }
  }
  else
  {
    // Put WiFi to sleep only if in STA mode
    if (WiFi.getMode() == WIFI_STA)
    {
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
      delay(1);
    }
  }
}

void houseKeeping()
{
  wifiHousekeeping();
  wifiConfigurator.HandleClient();
  webServer.handleClient();

  if (wifiConfigurator.IsSetupMode())
  {
    platformManager.Blink(3);
  }
  else if (!timeClient.update())
  {
    platformManager.Blink(3, 500);
    wifiHousekeeping(true);
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
