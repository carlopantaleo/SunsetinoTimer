#ifndef EVENTLOGGER_HPP
#define EVENTLOGGER_HPP

#include <deque>
#include <Arduino.h>
#include <ctime>
#include "NTPClient.hpp"
#include "constants.h"
#include "debug.h"

class EventLogger
{
private:
    NTPClient *const _ntpClient;
    std::deque<String> _events;

public:
    EventLogger(NTPClient *const ntpClient);
    void LogEvent(const String &event);
    String PrintEvents();
};

EventLogger::EventLogger(NTPClient *const ntpClient)
    : _ntpClient(ntpClient)
{
}

void EventLogger::LogEvent(const String &event)
{
    long now = (long) _ntpClient->getEpochTime();
    std::tm dayNow = *std::localtime(&now);
    String log = String(dayNow.tm_mday) + "/" + String(dayNow.tm_mon + 1) + "/" + String(dayNow.tm_year + 1900) + " " +
                 _ntpClient->getFormattedTime() + " " + event;
    _events.push_back(log);
    LOGDEBUGLN(log);

    if (_events.size() > NUM_EVENTS)
        _events.pop_front();
}

String EventLogger::PrintEvents()
{
    String ret = "";
    for (auto it = _events.rbegin(); it != _events.rend(); it++)
    {
        ret += *it + "\n";
    }

    return ret;
}

#endif