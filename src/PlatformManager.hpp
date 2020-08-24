#ifndef PLATFORMMANAGER_HPP
#define PLATFORMMANAGER_HPP

#include <ESP8266WiFi.h>
#include "EventLogger.hpp"

#ifdef BUILTIN_LED_ON_WITH_LAMP
#define BUILTIN_LED_ON (_lampState + 1) % 2
#define BUILTIN_LED_OFF _lampState
#else
#define BUILTIN_LED_ON LOW
#define BUILTIN_LED_OFF HIGH
#endif

#define LAMP_ON LOW
#define LAMP_OFF HIGH

typedef uint8_t LampState;

class PlatformManager
{
private:
    LampState _lampState = LAMP_OFF;
    uint8_t _builtinLed; // Its status will be the opposite of _lampState in order to be on when lamp is on
    uint8_t _lampPin;
    EventLogger *const _eventLogger;

public:
    PlatformManager(uint8_t builtinLed, uint8_t lampPin, EventLogger *eventLogger);
    void LampOn();
    void LampOff();
    void BlinkOn();
    void Blink(int repeat = 1, int duration = 50);
};

PlatformManager::PlatformManager(uint8_t builtinLed, uint8_t lampPin, EventLogger *eventLogger)
    : _builtinLed(builtinLed), _lampPin(lampPin), _eventLogger(eventLogger) {}

void PlatformManager::LampOn()
{
    if (_lampState == LAMP_OFF)
        _eventLogger->LogEvent(F("Lamp ON."));

    _lampState = LAMP_ON;
#ifdef BUILTIN_LED_ON_WITH_LAMP
    digitalWrite(_builtinLed, (_lampState + 1) % 2);
#endif
    digitalWrite(_lampPin, _lampState);
}

void PlatformManager::LampOff()
{
    if (_lampState == LAMP_ON)
        _eventLogger->LogEvent(F("Lamp OFF."));

    _lampState = LAMP_OFF;
#ifdef BUILTIN_LED_ON_WITH_LAMP
    digitalWrite(_builtinLed, (_lampState + 1) % 2);
#endif
    digitalWrite(_lampPin, _lampState);
}

void PlatformManager::BlinkOn()
{
    digitalWrite(_builtinLed, BUILTIN_LED_ON);
}

void PlatformManager::Blink(int repeat, int duration)
{
    for (int i = 0; i < repeat; i++)
    {
        digitalWrite(_builtinLed, BUILTIN_LED_ON);
        delay(duration);
        digitalWrite(_builtinLed, BUILTIN_LED_OFF);
        delay(duration);
    }
}
#endif
