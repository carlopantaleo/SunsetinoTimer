#ifndef PLATFORMMANAGER_HPP
#define PLATFORMMANAGER_HPP

#include <ESP8266WiFi.h>

typedef uint8_t LampState;

class PlatformManager
{
private:
    LampState _lampState = LOW;
    uint8_t _builtinLed; // Its status will be the opposite of _lampState in order to be on when lamp is on
    uint8_t _lampPin;

public:
    PlatformManager(uint8_t builtinLed, uint8_t lampPin);
    void LampOn();
    void LampOff();
    void Blink(int repeat = 1, int duration = 50);
};

PlatformManager::PlatformManager(uint8_t builtinLed, uint8_t lampPin)
    : _builtinLed(builtinLed), _lampPin(lampPin) {}

void PlatformManager::LampOn()
{
    _lampState = HIGH;
    digitalWrite(_builtinLed, (_lampState + 1) % 2);
    digitalWrite(_lampPin, _lampState);
}

void PlatformManager::LampOff()
{
    _lampState = LOW;
    digitalWrite(_builtinLed, (_lampState + 1) % 2);
    digitalWrite(_lampPin, _lampState);
}

void PlatformManager::Blink(int repeat = 1, int duration = 50)
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
