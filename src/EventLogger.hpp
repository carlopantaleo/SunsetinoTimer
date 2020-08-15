#ifndef EVENTLOGGER_HPP
#define EVENTLOGGER_HPP

#include <deque>
#include <Arduino.h>
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
    String log = _ntpClient->getFormattedTime() + " " + event;
    _events.push_back(log);
    LOGDEBUGLN(log);

    if (_events.size() > NUM_EVENTS)
        _events.pop_back();
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