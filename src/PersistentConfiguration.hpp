#ifndef PERSISTENTCONFIGURATION_HPP
#define PERSISTENTCONFIGURATION_HPP

#include <ctime>
#include <EEPROM.h>
#include <Arduino.h>
#include "constants.h"

enum TimeType 
{
    EXACT,
    SUNRISE,
    SUNSET
};

typedef struct
{
    std::tm on;
    TimeType onType;
    std::tm off;
    TimeType offType;
} TimerInterval;

class PersistentConfiguration
{
public:
    PersistentConfiguration();
    String GetSSID();
    void SetSSID(const String &ssid);
    String GetPassword();
    void SetPassword(const String &password);
    void GetCoordinates(float &latitude, float &longitude);
    void SetCoordinates(const float &latitude, const float &longitude);
    float GetTimezoneOffset();
    void SetTimezoneOffset(const float &tzOffset);
    TimerInterval GetTimerInterval(unsigned int num);
    void SetTimerInterval(unsigned int num, TimerInterval timerInterval);
    void SaveConfiguration();
    void Reset();

private:
    struct Conf
    {
        char ssid[32 + 1];
        char password[64 + 1];
        float latitude;
        float longitude;
        float tzOffset;
        TimerInterval timerIntervals[NUM_INTERVALS];
    } _conf;
};

PersistentConfiguration::PersistentConfiguration()
{
    EEPROM.begin(sizeof(Conf));
    EEPROM.get(0, _conf);
}

String PersistentConfiguration::GetSSID()
{
    return _conf.ssid;
}

void PersistentConfiguration::SetSSID(const String &ssid)
{
    memset(_conf.ssid, 0, sizeof(_conf.ssid));
    strncpy(_conf.ssid, ssid.c_str(), sizeof(_conf.ssid) - 1);
}

String PersistentConfiguration::GetPassword()
{
    return _conf.password;
}

void PersistentConfiguration::SetPassword(const String &password)
{
    memset(_conf.password, 0, sizeof(_conf.password));
    strncpy(_conf.password, password.c_str(), sizeof(_conf.password) - 1);
}

void PersistentConfiguration::GetCoordinates(float &latitude, float &longitude)
{
    latitude = _conf.latitude;
    longitude = _conf.longitude;
}

void PersistentConfiguration::SetCoordinates(const float &latitude, const float &longitude)
{
    _conf.latitude = latitude;
    _conf.longitude = longitude;
}

float PersistentConfiguration::GetTimezoneOffset()
{
    return _conf.tzOffset;
}

void PersistentConfiguration::SetTimezoneOffset(const float &tzOffset)
{
    _conf.tzOffset = tzOffset;
}

TimerInterval PersistentConfiguration::GetTimerInterval(unsigned int num)
{
    TimerInterval ti;
    if (num > NUM_INTERVALS)
        ti = {0};
    else
        ti = _conf.timerIntervals[num];

    return ti;
}

void PersistentConfiguration::SetTimerInterval(unsigned int num, TimerInterval timerInterval)
{
    if (num > NUM_INTERVALS)
        return;
    
    _conf.timerIntervals[num] = timerInterval;
}

void PersistentConfiguration::SaveConfiguration()
{
    EEPROM.put(0, _conf);
    EEPROM.commit();
}

void PersistentConfiguration::Reset()
{
    Conf rstConf = {0};
    EEPROM.put(0, rstConf);
    EEPROM.commit();
}

#endif
