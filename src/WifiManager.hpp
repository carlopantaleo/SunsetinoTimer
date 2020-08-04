#ifndef WIFIMANAGER_HPP
#define WIFIMANAGER_HPP

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include "PlatformManager.hpp"
#include "PersistentConfiguration.hpp"
#include "debug.h"

class WifiManager
{
private:
    static const int MAX_CONNECTION_ATTEMPTS = 20;
    const IPAddress _apIP;
    const char *_apSSID = "SunsetinoTimer";
    boolean _isSetupMode = false;
    String _ssidList;
    DNSServer _dnsServer;
    ESP8266WebServer *const _webServer;
    PlatformManager *const _platformManager;
    PersistentConfiguration *const _persistentConfiguration;

    boolean RestoreConfig();
    void ConfigureWebServer();
    void SetupMode();
    String MakePage(String title, String contents);
    String UrlDecode(String input);
    void OnSettings();
    void OnSetAp();
    void OnReset();

public:
    WifiManager(ESP8266WebServer *webServer,
                PlatformManager *platformManager,
                PersistentConfiguration *persistentConfiguration);
    ~WifiManager();
    void Setup();
    void HandleClient();
    bool CheckConnection();
    bool IsSetupMode();
};

WifiManager::WifiManager(ESP8266WebServer *webServer,
                         PlatformManager *platformManager,
                         PersistentConfiguration *persistentConfiguration)
    : _apIP(192, 168, 1, 1),
      _webServer(webServer),
      _platformManager(platformManager),
      _persistentConfiguration(persistentConfiguration)
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
        _webServer->on(F("/"), [this]() {
            // TODO: move logic to general configuration component
            _platformManager->Blink();
            String s = F("<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>");
            _webServer->send(200, F("text/html"), MakePage(F("STA mode"), s));
            _platformManager->Blink();
        });
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
        for (int i = 0; i < 4; i++)
        {
            TimerInterval intv = _persistentConfiguration->GetTimerInterval(i);
            String strOn = String(intv.on.tm_hour) + ":" + String(intv.on.tm_min);
            String strOff = String(intv.off.tm_hour) + ":" + String(intv.off.tm_min);

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
        width: 100px;
    }
</style>
<h1>Platform settings</h1>
<form action="http://google.it">
    <h4>Coordinates</h4>
    <p>
        <label for="lat" class="label">Latitude</label>
        <input type="number" name="lng" id="lng" value=")=" +
                   String(lat) + R"=(">
    </p>
    <p>
        <label for="lng" class="label">Longitude</label>
        <input type="number" name="lng" id="lng" value=")=" +
                   String(lng) + R"=(">
    </p>
    <br/>
)=" + intervals + R"=(
    <input type="submit"/>
</form>
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

void WifiManager::OnReset()
{
    _platformManager->Blink();
    for (int i = 0; i < 96; ++i)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
    String s = F("<h1>Wi-Fi settings was reset.</h1><p>The device is going to reboot now.</p>");
    _webServer->send(200, F("text/html"), MakePage(F("Reset Wi-Fi Settings"), s));
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