#include "WifiManager.hpp"
#include "SunClock.hpp"
#include <WiFiUdp.h>
#include "NTPClient.hpp"
#include "EventLogger.hpp"
#include "debug.h"
#include "constants.h"

// PINS - D4: builtin led, D1: lamp relay, D3: WiFi on interrupt

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
ESP8266WebServer webServer(80);
EventLogger eventLogger(&timeClient);
PlatformManager platformManager(D4, D1, &eventLogger);
PersistentConfiguration persistentConfiguration;
WifiManager wifiManager(&webServer, &platformManager, &persistentConfiguration, &timeClient, &eventLogger);

void setup()
{
  Serial.begin(9600);
  pinMode(D4, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D3, INPUT_PULLUP);
  wifiManager.Setup();
  webServer.begin();
  timeClient.setUpdateInterval(NTP_UPDATE_INTERVAL);
  timeClient.setTimeOffset((int) persistentConfiguration.GetTimezoneOffset() * 60 * 60);
  timeClient.begin();
  attachInterrupt(digitalPinToInterrupt(D3), wifiOnISR, FALLING);
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

  delay(wifiManager.IsWifiOn() ? 500 : 10000);
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
  int tm1 = time1.tm_hour * 60 + time1.tm_min,
      tm2 = time2.tm_hour * 60 + time2.tm_min;

  if (tm1 > tm2)
    return 1;
  else if (tm1 < tm2)
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

    // Adjust intervals intersecting midnight (valid only for exact times)
    if (((ti.onType == EXACT && ti.offType == EXACT) ||
         (ti.onType == SUNSET && ti.offType == EXACT)) &&
        compareTimes(on, off) > 0)
    {
      if (compareTimes(now, on) >= 0)
      {
        off.tm_min = 59;
        off.tm_hour = 23;
      }
      else
        on = {0};
    }

    // Turn light on or off
    if (compareTimes(now, on) >= 0 && compareTimes(now, off) <= 0)
    {
      platformManager.LampOn();
      break;
    }
    else
      platformManager.LampOff();
  }
}

void setRiseSetTimes(time_t &rise, time_t &set)
{
  float lat, lng; // lat: 44.3316998, lng: 7.4774379
  persistentConfiguration.GetCoordinates(lat, lng);
  Sunclock sunclock(lat, lng, persistentConfiguration.GetTimezoneOffset());
  set = sunclock.sunset(timeClient.getEpochTime());
  rise = sunclock.sunrise(timeClient.getEpochTime());
  LOGDEBUG(F("Current time (GMT): "));
  LOGDEBUGLN(timeClient.getFormattedTime());
  printTime(set);
  printTime(rise);
}

void houseKeeping()
{
  wifiManager.HandleClient();
  webServer.handleClient();

  if (wifiManager.IsSetupMode())
  {
    platformManager.Blink(1, 500);
  }
  else
  {
    switch (timeClient.update())
    {
    case 0:
      platformManager.Blink(3, 500);
      wifiManager.TurnWifiOn();
      break;
    case 2:
      eventLogger.LogEvent(F("RTC synced."));
      break;
    default:
      platformManager.Blink();
      break;
    }
  }

  wifiManager.WifiHousekeeping();
}

ICACHE_RAM_ATTR void wifiOnISR()
{
  // Debounce
  static unsigned long lastInterruptTime = 0;
  if (millis() - lastInterruptTime < 10000)
    return;
  lastInterruptTime = millis();

  // Do the thing
  if (!wifiManager.IsWifiOn())
  {
    wifiManager.TurnWifiOn();
    platformManager.BlinkOn();
    eventLogger.LogEvent(F("Requested WiFi ON."));
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
