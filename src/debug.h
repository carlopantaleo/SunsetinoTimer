#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define LOGDEBUGLN(message) Serial.println(message);
#define LOGDEBUG(message) Serial.print(message);
#else
#define LOGDEBUGLN(message)
#define LOGDEBUG(message)
#endif

#endif