#ifndef PLATFORMMANAGER_HPP
#define PLATFORMMANAGER_HPP

#include <ESP8266WiFi.h>
#include "EventLogger.hpp"

typedef uint8_t LampState;

class PlatformManager
{
private:
    LampState _lampState = LOW;
    uint8_t _builtinLed; // Its status will be the opposite of _lampState in order to be on when lamp is on
    uint8_t _lampPin;
    EventLogger *const _eventLogger;

public:
    PlatformManager(uint8_t builtinLed, uint8_t lampPin, EventLogger *eventLogger);
    void LampOn();
    void LampOff();
    void Blink(int repeat = 1, int duration = 50);
};

PlatformManager::PlatformManager(uint8_t builtinLed, uint8_t lampPin, EventLogger *eventLogger)
    : _builtinLed(builtinLed), _lampPin(lampPin), _eventLogger(eventLogger) {}

void PlatformManager::LampOn()
{
    if (_lampState == LOW)
        _eventLogger->LogEvent(F("Lamp ON."));

    _lampState = HIGH;
    digitalWrite(_builtinLed, (_lampState + 1) % 2);
    digitalWrite(_lampPin, _lampState);
}

void PlatformManager::LampOff()
{
    if (_lampState == HIGH)
        _eventLogger->LogEvent(F("Lamp OFF."));

    _lampState = LOW;
    digitalWrite(_builtinLed, (_lampState + 1) % 2);
    digitalWrite(_lampPin, _lampState);
}

void PlatformManager::Blink(int repeat, int duration)
{
    for (int i = 0; i < repeat; i++)
    {
        digitalWrite(_builtinLed, _lampState);
        delay(duration);
        digitalWrite(_builtinLed, (_lampState + 1) % 2);
        delay(duration);
    }
}
#endif
