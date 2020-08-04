#ifndef PERSISTENTCONFIGURATION_HPP
#define PERSISTENTCONFIGURATION_HPP

#include <ctime>
#include <EEPROM.h>
#include <Arduino.h>

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
    TimerInterval GetTimerInterval(unsigned int num);
    void SetTimerInterval(unsigned int num, const TimerInterval &timerInterval);
    void SaveConfiguration();

private:
    struct Conf
    {
        char ssid[32 + 1];
        char password[64 + 1];
        float latitude;
        float longitude;
        TimerInterval timerIntervals[4];
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

TimerInterval PersistentConfiguration::GetTimerInterval(unsigned int num)
{
    TimerInterval ti;
    if (num > 4)
        ti = {0};
    else
        ti = _conf.timerIntervals[num];

    return ti;
}

void PersistentConfiguration::SetTimerInterval(unsigned int num, const TimerInterval &timerInterval)
{
    if (num > 4)
        return;
    
    _conf.timerIntervals[num] = timerInterval;
}

void PersistentConfiguration::SaveConfiguration()
{
    EEPROM.put(0, _conf);
    EEPROM.commit();
}

#endif
