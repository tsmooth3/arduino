#include "Adafruit_SHT4x.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define NUMPIXELS 1
const String TEMP_MON_LOCATION = "<ubidots_variable>";
const float COLD_ALERT = 65.0;
const float HOT_ALERT = 80.0;
const unsigned int MSG_INTERVAL = 32767;
const String UBIDOTS_TOKEN = "<device_token>";
const String UBIDOTS_HOOK = "https://industrial.api.ubidots.com/api/v1.6/devices/<device-label>/";
bool alertFired = true;
int alertCount = 0;
const String SLACK_URL = "https://hooks.slack.com/<slack-hook>";
unsigned int pixelHue = 0;
int pulseDirection = 1;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
WiFiMulti wifiMulti;

void setup()
{
    Serial.begin(115200);
    Serial.println("START");
    Wire1.setPins(SDA1, SCL1);
    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);
    pixels.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.setBrightness(10); // not so bright
    blinkPix();

    Serial.println("Adafruit SHT4X test");
    if (!sht4.begin(&Wire1))
    {
        Serial.println("Couldn't find SHT4x");
        while (1)
            delay(1);
    }
    Serial.println("Found SHT4x sensor");
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);

    wifiMulti.addAP("<ssid>", "<wifi_pw>");
    while ((wifiMulti.run() != WL_CONNECTED))
    {
        Serial.println("HTTP");
        delay(1000);
    }

    if (wifiMulti.run() == WL_CONNECTED)
    {
        Serial.print("\nWiFi connected, SSID: ");
        Serial.print(WiFi.SSID());
        Serial.print(", IP address: ");
        Serial.println(WiFi.localIP());
    }

    // Send a message to Slack
    String message = TEMP_MON_LOCATION + " : Temp Monitor On!";
    Serial.println(message);
    sendSlackMessage(message);
}

void sendSlackMessage(String pushMessage)
{
    if ((wifiMulti.run() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(SLACK_URL);
        http.addHeader("Content-Type", "application/json");
        String jsonPayload = "{\"text\":\"" + pushMessage + "\"}";
        int httpCode = http.POST(jsonPayload);
        if (httpCode > 0)
        {
            Serial.print("Slack response code: ");
        }
        else
        {
            Serial.print("Error sending. HTTP code: ");
        }
        Serial.println(httpCode);
        http.end();
    }
}

void loop()
{
    readTemp();
    rainbow(MSG_INTERVAL);
}

void rainbow(int wait)
{
    int multiplier = 20;
    int rate = 50;
    for (int i = 0; i < wait; i = i + rate)
    {
        if (pixelHue > 65535)
        {
            pixelHue = 65535;
            pulseDirection = 0;
        }
        if (pixelHue < 0)
        {
            pixelHue = 0;
            pulseDirection = 1;
        }
        if (pulseDirection == 1)
        {
            pixelHue = pixelHue + (rate * multiplier);
        }
        if (pulseDirection == 0)
        {
            pixelHue = pixelHue - (rate * multiplier);
        }
        pixels.setPixelColor(0, pixels.gamma32(pixels.ColorHSV(pixelHue)));
        pixels.show();
        delay(rate);
    }
}

void readTemp()
{
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    float shtc = temp.temperature;
    float shtf = (shtc * 1.8) + 32;
    Serial.print("TempC: ");
    Serial.print(shtc);
    Serial.println("C");
    Serial.print("TempF: ");
    Serial.print(shtf);
    Serial.println("F");
    logTemp(shtf);

    if (alertFired)
    {
        if (shtf > COLD_ALERT && shtf < HOT_ALERT)
        {
            alertFired = false;
            alertCount = 0;
        }
    }
    else
    {
        if (shtf > HOT_ALERT || shtf < COLD_ALERT)
        {
            alertCount++;
        }
        else
        {
            alertCount = 0;
        }
        if (alertCount > 2)
        {
            alertFired = true;
            String alertmsg = TEMP_MON_LOCATION + " : Temp ALERT : " + shtf + " F";
            sendSlackMessage(alertmsg);
        }
    }
}

void blinkPix()
{
    for (int i = 0; i < 3; i++)
    {
        pixels.setPixelColor(0, pixels.Color(155, 155, 155));
        pixels.show();
        delay(100);
        pixels.clear();
        pixels.show();
        delay(100);
    }
}

void logTemp(float t)
{
    if ((wifiMulti.run() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(UBIDOTS_HOOK);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("X-Auth-Token", UBIDOTS_TOKEN);
        String jsonPayload = "{\"" + TEMP_MON_LOCATION + "\":\"" + t + "\"}";
        int httpCode = http.POST(jsonPayload);
        http.end();
        if (httpCode > 204)
        {
            Serial.print("Error sending. HTTP code: ");
            Serial.println(httpCode);
            blinkPix();
        }
    }
}
