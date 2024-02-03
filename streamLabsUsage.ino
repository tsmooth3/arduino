#include <Arduino.h>
#include <ArduinoJson.h> //v6
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_7segment matrix = Adafruit_7segment();
WiFiMulti wifiMulti;
boolean drawDots;

void setup()
{
    matrix.begin(0x70);
    matrix.setBrightness(1);
    for (uint8_t t = 4; t > 0; t--)
    {
        if (t == 4)
        {
            matrix.clear();
            matrix.writeDigitNum(0, t);
        }
        if (t == 3)
        {
            matrix.clear();
            matrix.writeDigitNum(1, t);
        }
        if (t == 2)
        {
            matrix.clear();
            matrix.writeDigitNum(3, t);
        }
        if (t == 1)
        {
            matrix.clear();
            matrix.writeDigitNum(4, t);
        }
        matrix.writeDisplay();
        delay(1000);
    }
    matrix.println("0000");
    matrix.writeDisplay();
    wifiMulti.addAP("<ssid>", "<wifi_pw>");
    drawDots = false;
    matrix.clear();
    matrix.println("HTTP");
    matrix.writeDisplay();
}

void loop()
{
    // wait for WiFi connection
    if ((wifiMulti.run() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin("https://api.streamlabswater.com/v1/locations/<streamlabslocation>/readings/water-usage/summary");
        http.addHeader("Authorization", "Bearer <token>");
        int httpCode = http.GET();

        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK)
            {
                DynamicJsonDocument doc(2048);
                deserializeJson(doc, http.getStream());
                int todayGal = doc["today"];
                todayGal += 1;
                matrix.clear();
                matrix.writeDigitNum(1, (todayGal / 100) % 10, false);
                matrix.writeDigitNum(3, (todayGal / 10) % 10, false);
                matrix.writeDigitNum(4, todayGal % 10, drawDots);
                matrix.writeDisplay();
                delay(30000);
            }
        }
        else
        {
            matrix.clear();
            matrix.printError();
            matrix.writeDisplay();
            delay(180000);
        }

        http.end();
        if (drawDots == false)
        {
            drawDots = true;
        }
        else
        {
            drawDots = false;
        }
    }
    delay(5000);
}