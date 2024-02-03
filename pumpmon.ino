#include <WiFiMulti.h>
#include <Wire.h>
#include <driver/adc.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const String SLACK_URL = "https://hooks.slack.com/<servicepath>";

const int led = 13;
boolean drawDots;
boolean pumpOn;
int pumpLowCount;
int pumpRunCount;
int totalPumpRunTime;
int avgPumpRunTime;

Adafruit_7segment matrix = Adafruit_7segment();
WiFiMulti wifiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String lastTime = "";
String currentTime = "";
unsigned long lastTimeEpoch = 0;
unsigned long currentTimeEpoch = 0;
unsigned long startupTimeEpoch = 0;
unsigned long startupDelta = 0;
unsigned long epochDelta = 0;

void setup(void)
{
    pinMode(led, OUTPUT);
    digitalWrite(led, 0);
    Serial.begin(115200);
    Serial.println("");
    matrix.begin(0x70);
    matrix.setBrightness(3);

    pumpOn = false;
    pumpLowCount = 0;
    pumpRunCount = 0;
    epochDelta = 0;
    startupDelta = 0;
    totalPumpRunTime = 0;
    avgPumpRunTime = 0;

    // Configure ADC settings
    int desiredWidth = 4; // 13 bit ADC
    adc_bits_width_t bits_width = (adc_bits_width_t)desiredWidth;
    adc1_config_width(bits_width);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_0); // Adjust the channel and attenuation level as needed

    wifiMulti.addAP("<ssid>", "<wifi_pw>");
    while ((wifiMulti.run() != WL_CONNECTED))
    {
        matrix.clear();
        matrix.println("HTTP");
        matrix.writeDisplay();
        delay(1000);
    }

    matrix.clear();
    matrix.println("REDY");
    matrix.writeDisplay();
    delay(1000);

    if (wifiMulti.run() == WL_CONNECTED)
    {
        Serial.print("\nWiFi connected, SSID: ");
        Serial.print(WiFi.SSID());
        Serial.print(", IP address: ");
        Serial.println(WiFi.localIP());
    }

    matrix.clear();
    matrix.println("one");
    matrix.writeDisplay();
    delay(1000);

    timeClient.begin();

    matrix.clear();
    matrix.println("two");
    matrix.writeDisplay();
    delay(1000);
    timeClient.update();

    matrix.clear();
    matrix.println("3");
    matrix.writeDisplay();
    delay(1000);
    lastTime = timeClient.getFormattedTime();
    currentTime = timeClient.getFormattedTime();
    lastTimeEpoch = timeClient.getEpochTime();
    currentTimeEpoch = lastTimeEpoch;
    startupTimeEpoch = lastTimeEpoch;

    // Send a message to Pushbullet / Slack
    String message = currentTime + " \\n Pump Monitor On!";
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

void loop(void)
{
    int num_samples = 30; // Number of samples to take
    uint32_t adc_value = 0;

    for (int i = 0; i < num_samples; i++)
    {
        adc_value += adc1_get_raw(ADC1_CHANNEL_7); // Read the ADC value
        delay(10);                                 // Delay between samples if necessary
    }

    if (drawDots == false)
    {
        drawDots = true;
    }
    else
    {
        drawDots = false;
    }
    adc_value /= num_samples; // Calculate the average value

    // Convert ADC value to voltage
    float voltage = (adc_value / 8191.0) * 4.94; // 8191 is the maximum value for 13-bit ADC, 3.3V is the reference voltage
    float mapped_voltage = voltage;
    float current = mapped_voltage / 0.05; // every 1V is equal to 20A

    matrix.clear();

    if (current > 1.7)
    {
        timeClient.update();
        currentTime = timeClient.getFormattedTime();
        currentTimeEpoch = timeClient.getEpochTime();
        if (currentTimeEpoch >= lastTimeEpoch)
        {
            epochDelta = currentTimeEpoch - lastTimeEpoch;
        }
        else
        {
            epochDelta = 0;
        }
        String pbmsg = "Last Update:  " + lastTime + "\\n";
        pbmsg += "seconds ago: " + String(static_cast<int>(epochDelta)) + "\\n";
        pbmsg += "Current Time: " + currentTime + "\\n";

        if (current > 6.0)
        {
            // send one alert when pump is on
            if (!pumpOn || pumpLowCount > 1)
            {
                pumpLowCount = 0;
                lastTime = currentTime;
                lastTimeEpoch = currentTimeEpoch;
                pumpOn = true;
            }
        }
        else
        {
            // send alerts well dry
            pumpLowCount++;
            if (pumpOn && pumpLowCount > 1)
            {
                sendSlackMessage(pbmsg + "\\n pumpLowCount: " + String(pumpLowCount) + "\\n LOW CURRENT < 6.0A: " + String(current));
                lastTime = currentTime;
                lastTimeEpoch = currentTimeEpoch;
            }
            pumpOn = true;
        }
        matrix.printFloat(current, 2);
    }
    else
    {
        if (drawDots)
        {
            matrix.writeDigitAscii(0, ' ', drawDots);
        }
        else
        {
            matrix.writeDigitAscii(0, ' ', drawDots);
        }
        if (pumpOn)
        {
            timeClient.update();
            currentTime = timeClient.getFormattedTime();
            currentTimeEpoch = timeClient.getEpochTime();
            if (currentTimeEpoch >= lastTimeEpoch)
            {
                epochDelta = currentTimeEpoch - lastTimeEpoch;
            }
            else
            {
                epochDelta = 0;
            }
            // send alert that pump is now off
            pumpLowCount = 0;
            pumpRunCount++;
            totalPumpRunTime += static_cast<int>(epochDelta);
            avgPumpRunTime = totalPumpRunTime / pumpRunCount;
            pumpOn = false;

            String pbmsg = "Pump OFF \\n";
            pbmsg += "this run: " + String(static_cast<int>(epochDelta)) + "\\n";
            pbmsg += "24h runs: " + String(pumpRunCount) + "\\n";
            pbmsg += "24h total: " + String(totalPumpRunTime) + "\\n";
            pbmsg += "24h avg: " + String(avgPumpRunTime);
            sendSlackMessage(pbmsg);

            if (currentTimeEpoch >= startupTimeEpoch)
            {
                startupDelta = currentTimeEpoch - startupTimeEpoch;
                if (startupDelta >= 21600)
                {
                    // new day
                    avgPumpRunTime = totalPumpRunTime / pumpRunCount;
                    String pbmsg = "Daily Update: " + currentTime + "\\n";
                    pbmsg += "Pump Runs in past 24 hours: " + String(pumpRunCount) + "\\n";
                    pbmsg += "Pump Low  in past 24 hours: " + String(pumpLowCount) + "\\n";
                    pbmsg += "Pump Avg Run Time in past 24 hours (seconds): " + String(avgPumpRunTime);
                    sendSlackMessage(pbmsg);
                    pumpRunCount = 0;
                    totalPumpRunTime = 0;
                    startupTimeEpoch = currentTimeEpoch;
                }
            }
            lastTime = currentTime;
            lastTimeEpoch = currentTimeEpoch;
        }
    }
    matrix.writeDisplay();
    delay(1000);
}