#ifndef GLOBALS_H
#define GLOBALS_H

#include <WiFi.h>

// shared state
extern int seconds;
extern unsigned long lastSecondTick;
extern bool sendingEnabled;
extern int displayMode;

// sensor values
extern int currentTemp;
extern int currentHum;

// web
void handleClient();

#endif
