#include "WifiManager.hpp"
#include "SunClock.hpp"
#include <WiFiUdp.h>
#include "NTPClient.hpp"
#include "debug.h"
#include "constants.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
ESP8266WebServer webServer(80);
PlatformManager platformManager(D4, D1);
PersistentConfiguration persistentConfiguration;
WifiManager wifiManager(&webServer, &platformManager, &persistentConfiguration, &timeClient);

void setup()
{
  Serial.begin(115200);
  pinMode(D4, OUTPUT);
  pinMode(D1, OUTPUT);
  wifiManager.Setup();
  webServer.begin();
  timeClient.setUpdateInterval(NTP_UPDATE_INTERVAL);
  timeClient.begin();
}

void loop()
{
  houseKeeping();

  if (wifiManager.IsSetupMode())
  {
    delay(500);
    return;
  }

  // Normal operation
  static int day = -1;
  static time_t set, rise;

  // Recalculate rise and set times once a day
  if (timeClient.getDay() != day)
  {
    day = timeClient.getDay();
    setRiseSetTimes(rise, set);
  }

  manageLamp(rise, set);

  delay(10000);
}

std::tm getTime(const TimeType &type, const time_t &rise, const time_t &set, const std::tm &exactTime)
{
  switch (type)
  {
  case SUNRISE:
    return *std::localtime(&rise);
  case SUNSET:
    return *std::localtime(&set);
  default:
    return exactTime;
  }
}

int compareTimes(const std::tm &time1, const std::tm &time2)
{
  if (time1.tm_hour > time2.tm_hour && time1.tm_min > time2.tm_min)
    return 1;
  else if (time1.tm_hour < time2.tm_hour && time1.tm_min < time2.tm_min)
    return -1;
  else
    return 0;
}

void manageLamp(time_t &rise, time_t &set)
{
  for (int i = 0; i < NUM_INTERVALS; i++)
  {
    TimerInterval ti = persistentConfiguration.GetTimerInterval(i);
    std::tm on = getTime(ti.onType, rise, set, ti.on);
    std::tm off = getTime(ti.offType, rise, set, ti.off);
    std::tm now;
    now.tm_min = timeClient.getMinutes();
    now.tm_hour = timeClient.getHours();

    // Adjust intervals intersecting midnight (valid only for exact times and other few exceptions)
    if (((ti.onType == EXACT && ti.offType == EXACT) ||
         (ti.onType == SUNSET && ti.offType == EXACT) ||
         (ti.onType == EXACT && ti.offType == SUNSET)) &&
        compareTimes(on, off) < 0)
    {
      if (compareTimes(now, on) >= 0)
      {
        off.tm_min = 59;
        off.tm_hour = 23;
      }
      else if (compareTimes(now, off) <= 0)
        on = {0};
    }

    // Turn light on or off
    if (compareTimes(now, on) >= 0 && compareTimes(now, off) <= 0)
      platformManager.LampOn();
    else
      platformManager.LampOff();
  }
}

void setRiseSetTimes(time_t &rise, time_t &set)
{
  Sunclock sunclock(44.3316998, 7.4774379); // TODO: retrieve location from EEPROM
  set = sunclock.sunset(timeClient.getEpochTime());
  rise = sunclock.sunrise(timeClient.getEpochTime());
  LOGDEBUG(F("Current time (GMT): "));
  LOGDEBUGLN(timeClient.getFormattedTime());
  printTime(set);
  printTime(rise);
}

void wifiHousekeeping(bool forceReset = false)
{
  static unsigned long lastConnection = 0;
  if (forceReset)
    lastConnection = millis(); // Force reconnect

  // Connections will be kept alive for 10 minutes, then WiFi will be turned off for power saving.
  if (millis() - lastConnection < 10 * 60 * 1000)
  {
    // Awake WiFi if it was sleeping
    if (WiFi.getMode() == WIFI_OFF)
    {
      LOGDEBUGLN(F("Waking WiFi up"));
      WiFi.forceSleepWake();
      delay(1);
      WiFi.mode(WIFI_STA);
      WiFi.begin();
      lastConnection = millis();
    }

    if (!wifiManager.CheckConnection())
    {
      platformManager.Blink(10, 50);
      lastConnection = millis(); // Reset last connection timer
    }
    else
    {
      platformManager.Blink(5);
    }
  }
  else
  {
    // Put WiFi to sleep only if in STA mode
    if (WiFi.getMode() == WIFI_STA)
    {
      LOGDEBUGLN(F("Putting WiFi to sleep."));
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
      delay(1);
    }
  }
}

void houseKeeping()
{
  wifiManager.HandleClient();
  webServer.handleClient();

  if (wifiManager.IsSetupMode())
  {
    platformManager.Blink(1, 500);
  }
  else if (!timeClient.update())
  {
    platformManager.Blink(3, 500);
    wifiHousekeeping(true);
  }
  else
  {
    wifiHousekeeping();
    platformManager.Blink();
  }
}

/**
 * Only for debug, will be removed
 */
void printTime(time_t time)
{
#ifdef DEBUG
  std::tm *ptm = std::localtime(&time);
  char buffer[32];
  memset(buffer, 0, sizeof(buffer) - 1);
  std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S (GMT)", ptm);
  Serial.println(buffer);
#endif
}
