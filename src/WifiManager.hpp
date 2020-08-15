#ifndef WIFIMANAGER_HPP
#define WIFIMANAGER_HPP

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <string>
#include "PlatformManager.hpp"
#include "PersistentConfiguration.hpp"
#include "NTPClient.hpp"
#include "EventLogger.hpp"
#include "debug.h"

class WifiManager
{
private:
    static const int MAX_CONNECTION_ATTEMPTS = 20;
    const IPAddress _apIP;
    const char *_apSSID = "SunsetinoTimer";
    boolean _isSetupMode = false;
    unsigned long _lastConnection = 0;
    String _ssidList;
    DNSServer _dnsServer;
    ESP8266WebServer *const _webServer;
    PlatformManager *const _platformManager;
    PersistentConfiguration *const _persistentConfiguration;
    NTPClient *const _timeClient;
    EventLogger *const _eventLogger;

    boolean RestoreConfig();
    void ConfigureWebServer();
    void SetupMode();
    String MakePage(String title, String contents);
    String UrlDecode(String input);
    void OnSettings();
    void OnSaveSettings();
    void OnSetAp();
    void OnReset();

public:
    WifiManager(ESP8266WebServer *webServer,
                PlatformManager *platformManager,
                PersistentConfiguration *persistentConfiguration,
                NTPClient *timeClient,
                EventLogger *eventLogger);
    ~WifiManager();
    void Setup();
    void HandleClient();
    bool CheckConnection();
    bool IsSetupMode();
    boolean IsWifiOn();
    void WifiHousekeeping(bool forceReset = false);
};

WifiManager::WifiManager(ESP8266WebServer *webServer,
                         PlatformManager *platformManager,
                         PersistentConfiguration *persistentConfiguration,
                         NTPClient *timeClient,
                         EventLogger *eventLogger)
    : _apIP(192, 168, 1, 1),
      _webServer(webServer),
      _platformManager(platformManager),
      _persistentConfiguration(persistentConfiguration),
      _timeClient(timeClient),
      _eventLogger(eventLogger)
{
}

WifiManager::~WifiManager()
{
}

void WifiManager::Setup()
{
    EEPROM.begin(512);
    WiFi.mode(WIFI_STA);
    delay(10);
    if (!(RestoreConfig() && CheckConnection()))
    {
        LOGDEBUGLN(F("Running in setup mode"));
        SetupMode();
    }

    ConfigureWebServer();
}

void WifiManager::HandleClient()
{
    if (_isSetupMode)
    {
        _dnsServer.processNextRequest();
    }
}

boolean WifiManager::RestoreConfig()
{
    LOGDEBUGLN(F("\nReading EEPROM..."));
    String ssid = _persistentConfiguration->GetSSID();
    String pass = _persistentConfiguration->GetPassword();
    if (!ssid.isEmpty())
    {
        LOGDEBUG(F("SSID: "));
        LOGDEBUGLN(ssid);
        LOGDEBUG(F("Password: "));
        LOGDEBUGLN(pass);
        WiFi.begin(ssid.c_str(), pass.c_str());
        return true;
    }
    else
    {
        LOGDEBUGLN(F("Config not found."));
        return false;
    }
}

bool WifiManager::CheckConnection()
{
    int numAttempts;
    for (numAttempts = 0;
         WiFi.status() != WL_CONNECTED && numAttempts < MAX_CONNECTION_ATTEMPTS;
         numAttempts++)
    {
        delay(250);
        _platformManager->Blink();
        LOGDEBUG(F("."));
    }

    return numAttempts < MAX_CONNECTION_ATTEMPTS;
}

bool WifiManager::IsSetupMode()
{
    return _isSetupMode;
}

void WifiManager::ConfigureWebServer()
{
    if (_isSetupMode)
    {
        _webServer->on(F("/settings"), [this]() { OnSettings(); });
        _webServer->on(F("/set-ap"), [this]() { OnSetAp(); });
    }
    else
    {
        _webServer->on(F("/save-settings"), [this]() { OnSaveSettings(); });
        _webServer->on(F("/reset"), [this]() { OnReset(); });
    }

    _webServer->onNotFound([this]() { OnSettings(); });
}

void WifiManager::OnSettings()
{
    _platformManager->Blink();
    if (_isSetupMode)
    {
        String s = F("<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>");
        s += F("<form action=\"set-ap\"><label>SSID: </label><select name=\"ssid\">");
        s += _ssidList;
        s += F("</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>");
        _webServer->send(200, F("text/html"), MakePage(F("Wi-Fi Settings"), s));
    }
    else
    {
        String intervals = "";
        for (int i = 0; i < NUM_INTERVALS; i++)
        {
            TimerInterval intv = _persistentConfiguration->GetTimerInterval(i);
            String strOn = (intv.on.tm_hour < 10 ? "0" : "") + String(intv.on.tm_hour) + ":" +
                           (intv.on.tm_min < 10 ? "0" : "") + String(intv.on.tm_min);
            String strOff = (intv.off.tm_hour < 10 ? "0" : "") + String(intv.off.tm_hour) + ":" +
                            (intv.off.tm_min < 10 ? "0" : "") + String(intv.off.tm_min);

            intervals += "<h4>Interval " + String(i+1) + "</h4>"
                         "<p>"
                         "<span class='label'>On</span>"
                         "<select id='onType" + String(i) + "' name='onType" + String(i) + "'>"
                         "<option value='0' " + (intv.onType == 0 ? "selected" : "") + ">Specific time</option>"
                         "<option value='1' " + (intv.onType == 1 ? "selected" : "") + ">Sunrise</option>"
                         "<option value='2' " + (intv.onType == 2 ? "selected" : "") + ">Sunset</option>"
                         "</select>"
                         "<input type='time' id='onTime" + String(i) + "' name='onTime" + String(i) + "' value='" + strOn + "'/>"
                         "<br/>"
                         "<span class='label'>Off</span>"
                         "<select id='offType" + String(i) + "' name='offType" + String(i) + "'>"
                         "<option value='0' " + (intv.offType == 0 ? "selected" : "") + ">Specific time</option>"
                         "<option value='1' " + (intv.offType == 1 ? "selected" : "") + ">Sunrise</option>"
                         "<option value='2' " + (intv.offType == 2 ? "selected" : "") + ">Sunset</option>"
                         "</select>"
                         "<input type='time' id='offTime" + String(i) + "' name='offTime" + String(i) + "' value='" + strOff + "'/>"
                         "</p>"
                         "<br/>";
        }

        float lat, lng;
        _persistentConfiguration->GetCoordinates(lat, lng);
        String s = R"=(
<style>
    .label {
        display: inline-block;
        width: 150px;
    }
</style>
<h1>Platform settings</h1>
<p>Current time: )=" + _timeClient->getFormattedTime() + R"=(</p>
<form action="save-settings">
    <h4>Coordinates</h4>
    <p>
        <label for="lat" class="label">Latitude</label>
        <input type="number" step="any" name="lat" id="lat" value=")=" +
                   String(lat, 7) + R"=(">
    </p>
    <p>
        <label for="lng" class="label">Longitude</label>
        <input type="number" step="any" name="lng" id="lng" value=")=" +
                   String(lng, 7) + R"=(">
    </p>
    <p>
        <label for="tzoff" class="label">Timezone Offset</label>
        <input type="number" step="0.5" name="tzoff" id="tzoff" value=")=" +
                   String(_persistentConfiguration->GetTimezoneOffset(), 1) + R"=(">
    </p>
    <br/>
)=" + intervals + R"=(
    <input type="submit"/>
</form>
<h4>Events</h4>
<pre>)=" + _eventLogger->PrintEvents() + R"=(</pre>
 )=";
        _webServer->send(200, F("text/html"), MakePage(F("Platform Settings"), s));
    }
    _platformManager->Blink();
}

void WifiManager::OnSetAp()
{
    _platformManager->Blink();
    String ssid = UrlDecode(_webServer->arg(F("ssid")));
    LOGDEBUG(F("SSID: "));
    LOGDEBUGLN(ssid);
    String pass = UrlDecode(_webServer->arg(F("pass")));
    LOGDEBUG(F("Password: "));
    LOGDEBUGLN(pass);
    LOGDEBUGLN(F("Saving configuration..."));
    _persistentConfiguration->SetSSID(ssid);
    _persistentConfiguration->SetPassword(pass);
    _persistentConfiguration->SaveConfiguration();
    String s = F("<h1>Setup complete.</h1><p>The device will reboot now and will be connected to \"");
    s += ssid;
    s += F("\" after the restart.</p>");
    _webServer->send(200, F("text/html"), MakePage(F("Wi-Fi Settings"), s));
    _platformManager->Blink();
    ESP.restart();
}

void WifiManager::OnSaveSettings()
{
    _platformManager->Blink();
    _persistentConfiguration->SetCoordinates(
        atof(UrlDecode(_webServer->arg(F("lat"))).c_str()), atof(UrlDecode(_webServer->arg(F("lng"))).c_str()));
    #ifdef DEBUG
    float lat, lng;
    _persistentConfiguration->GetCoordinates(lat, lng);
    LOGDEBUGLN("Lat: " + String(lat, 7) + "; Lng: " + String(lng, 7))
    #endif

    float tzOffset = atof(UrlDecode(_webServer->arg(F("tzoff"))).c_str());
    _persistentConfiguration->SetTimezoneOffset(tzOffset);
    LOGDEBUGLN("Timezone offset: " + String(tzOffset, 1));

    for (int i = 0; i < NUM_INTERVALS; i++)
    {
        TimerInterval ti = {0};

        // On
        String strOn = UrlDecode(_webServer->arg("onTime" + String(i)));
        TimeType ttOn = static_cast<TimeType>(atoi(UrlDecode(_webServer->arg("onType" + String(i))).c_str()));
        LOGDEBUGLN("onTime" + String(i) + ": " + strOn)
        LOGDEBUGLN("onType" + String(i) + ": " + ttOn)
        if (!strOn.isEmpty() && ttOn == 0)
        {
            ti.on.tm_hour = atoi(strOn.substring(0, 2).c_str());
            ti.on.tm_min = atoi(strOn.substring(3, 5).c_str());
        }
        ti.onType = ttOn;

        // Off
        String strOff = UrlDecode(_webServer->arg("offTime" + String(i)));
        TimeType ttOff = static_cast<TimeType>(atoi(UrlDecode(_webServer->arg("offType" + String(i))).c_str()));
        LOGDEBUGLN("offTime" + String(i) + ": " + strOff)
        LOGDEBUGLN("offType" + String(i) + ": " + ttOff)
        if (!strOff.isEmpty() && ttOff == 0)
        {
            ti.off.tm_hour = atoi(strOff.substring(0, 2).c_str());
            ti.off.tm_min = atoi(strOff.substring(3, 5).c_str());
        }
        ti.offType = ttOff;

        _persistentConfiguration->SetTimerInterval(i, ti);
    }
    _persistentConfiguration->SaveConfiguration();
    String s = F("<h1>Configuration saved.</h1><p><a href=\"/\">Go back to settings.</a></p>");
    _webServer->send(200, F("text/html"), MakePage(F("Configuration saved"), s));
    _eventLogger->LogEvent(F("Configuration changed."));
    _platformManager->Blink();
}

void WifiManager::OnReset()
{
    _platformManager->Blink();
    _persistentConfiguration->Reset();
    String s = F("<h1>Platform reset.</h1><p>The device is going to reboot now.</p>");
    _webServer->send(200, F("text/html"), MakePage(F("Platform reset"), s));
    _platformManager->Blink();
    ESP.restart();
}

void WifiManager::SetupMode()
{
    _isSetupMode = true;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    _platformManager->Blink(50, 10);
    int n = WiFi.scanNetworks();
    delay(100);
    _platformManager->Blink(50, 10);
    LOGDEBUGLN("");
    for (int i = 0; i < n; ++i)
    {
        _ssidList += F("<option value=\"");
        _ssidList += WiFi.SSID(i);
        _ssidList += F("\">");
        _ssidList += WiFi.SSID(i);
        _ssidList += F("</option>");
    }
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(_apIP, _apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(_apSSID);
    _dnsServer.start(53, "*", _apIP);
    LOGDEBUG(F("Starting Access Point at \""));
    LOGDEBUG(_apSSID);
    LOGDEBUGLN("\"");
}

void WifiManager::WifiHousekeeping(bool forceReset)
{
    if (forceReset)
        _lastConnection = millis(); // Force reconnect

    if (IsWifiOn())
    {
        // Awake WiFi if it was sleeping
        if (WiFi.getMode() == WIFI_OFF)
        {
            _eventLogger->LogEvent(F("WiFi active."));
            WiFi.forceSleepWake();
            delay(1);
            WiFi.mode(WIFI_STA);
            WiFi.begin();
            _lastConnection = millis();
        }

        if (!CheckConnection())
        {
            _platformManager->Blink(10, 50);
            _lastConnection = millis(); // Reset last connection timer
        }
        else
        {
            _platformManager->Blink(5);
        }
    }
    else
    {
        // Put WiFi to sleep only if in STA mode
        if (WiFi.getMode() == WIFI_STA)
        {
            _eventLogger->LogEvent(F("WiFi inactive."));
            WiFi.mode(WIFI_OFF);
            WiFi.forceSleepBegin();
            delay(1);
        }
    }
}

boolean WifiManager::IsWifiOn()
{
    // Connections are kept alive for 10 minutes, then WiFi will be turned off for power saving.
    return millis() - _lastConnection < 10 * 60 * 1000;
}

String WifiManager::MakePage(String title, String contents)
{
    String s = F("<!DOCTYPE html><html><head>");
    s += F("<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">");
    s += F("<title>");
    s += title;
    s += F("</title></head><body>");
    s += contents;
    s += F("</body></html>");
    return s;
}

String WifiManager::UrlDecode(String input)
{
    String s = input;
    s.replace(F("%20"), F(" "));
    s.replace(F("+"), F(" "));
    s.replace(F("%21"), F("!"));
    s.replace(F("%22"), F("\""));
    s.replace(F("%23"), F("#"));
    s.replace(F("%24"), F("$"));
    s.replace(F("%25"), F("%"));
    s.replace(F("%26"), F("&"));
    s.replace(F("%27"), F("\'"));
    s.replace(F("%28"), F("("));
    s.replace(F("%29"), F(")"));
    s.replace(F("%30"), F("*"));
    s.replace(F("%31"), F("+"));
    s.replace(F("%2C"), F(","));
    s.replace(F("%2E"), F("."));
    s.replace(F("%2F"), F("/"));
    s.replace(F("%2C"), F(","));
    s.replace(F("%3A"), F(":"));
    s.replace(F("%3A"), F(";"));
    s.replace(F("%3C"), F("<"));
    s.replace(F("%3D"), F("="));
    s.replace(F("%3E"), F(">"));
    s.replace(F("%3F"), F("?"));
    s.replace(F("%40"), F("@"));
    s.replace(F("%5B"), F("["));
    s.replace(F("%5C"), F("\\"));
    s.replace(F("%5D"), F("]"));
    s.replace(F("%5E"), F("^"));
    s.replace(F("%5F"), F("-"));
    s.replace(F("%60"), F("`"));
    return s;
}

#endif