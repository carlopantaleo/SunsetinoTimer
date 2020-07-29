#include "WifiConfigurator.hpp"

ESP8266WebServer webServer(80);
PlatformManager platformManager(D4, D1);
WifiConfigurator wifiConfigurator(&webServer, &platformManager);

void setup()
{
  pinMode(D4, OUTPUT);
  pinMode(D1, OUTPUT);
  wifiConfigurator.Setup();
  webServer.begin();
}

void loop()
{
  if (!wifiConfigurator.CheckConnection())
  {
    platformManager.Blink(10, 20);
  }
  wifiConfigurator.HandleClient();
  webServer.handleClient();
  delay(2000);
}
