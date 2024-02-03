#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Adafruit_SHT4x.h"

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_BME280 bme; // I2C
boolean drawDots;
unsigned status;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    // turn on the I2C power by setting pin to opposite of 'rest state'
    pinMode(PIN_I2C_POWER, INPUT);
    delay(1);
    bool polarity = digitalRead(PIN_I2C_POWER);
    pinMode(PIN_I2C_POWER, OUTPUT);
    digitalWrite(PIN_I2C_POWER, !polarity);

    Serial.println("Adafruit SHT4X test");
    if (!sht4.begin())
    {
        Serial.println("Couldn't find SHT4x");
        while (1)
            delay(1);
    }
    Serial.println("Found SHT4x sensor");
    Serial.print("Serial number 0x");
    Serial.println(sht4.readSerial(), HEX);
    // You can have 3 different precisions, higher precision takes longer
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    switch (sht4.getPrecision())
    {
    case SHT4X_HIGH_PRECISION:
        Serial.println("High precision");
        break;
    case SHT4X_MED_PRECISION:
        Serial.println("Med precision");
        break;
    case SHT4X_LOW_PRECISION:
        Serial.println("Low precision");
        break;
    }
    // You can have 6 different heater settings
    // higher heat and longer times uses more power
    // and reads will take longer too!
    sht4.setHeater(SHT4X_NO_HEATER);
    switch (sht4.getHeater())
    {
    case SHT4X_NO_HEATER:
        Serial.println("No heater");
        break;
    case SHT4X_HIGH_HEATER_1S:
        Serial.println("High heat for 1 second");
        break;
    case SHT4X_HIGH_HEATER_100MS:
        Serial.println("High heat for 0.1 second");
        break;
    case SHT4X_MED_HEATER_1S:
        Serial.println("Medium heat for 1 second");
        break;
    case SHT4X_MED_HEATER_100MS:
        Serial.println("Medium heat for 0.1 second");
        break;
    case SHT4X_LOW_HEATER_1S:
        Serial.println("Low heat for 1 second");
        break;
    case SHT4X_LOW_HEATER_100MS:
        Serial.println("Low heat for 0.1 second");
        break;
    }

    matrix.begin(0x70);
    matrix.setBrightness(1);
    status = bme.begin(0x77);
    if (!status)
    {
        matrix.blinkRate(HT16K33_BLINK_1HZ);
        matrix.printError();
        matrix.writeDisplay();
        delay(100);
    }
}

void loop()
{
    if (status)
    {
        readTemp();

        if (drawDots == false)
        {
            drawDots = true;
        }
        else
        {
            drawDots = false;
        }
    }
    else
    {
        matrix.blinkRate(HT16K33_BLINK_1HZ);
        matrix.printError();
        matrix.writeDisplay();
        delay(100);
    }
}

void readTemp()
{
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    float shtc = temp.temperature;
    float shtf = (shtc * 1.8) + 32;

    float c = bme.readTemperature();
    float f = (c * 1.8) + 32;

    float cdiff = shtc - c;
    float fdiff = shtf - f;

    Serial.print("TempC: ");
    Serial.print(shtc);
    Serial.print(" - ");
    Serial.print(c);
    Serial.print(" = ");
    Serial.print(cdiff);
    Serial.println("C");

    Serial.print("TempF: ");
    Serial.print(shtf);
    Serial.print(" - ");
    Serial.print(f);
    Serial.print(" = ");
    Serial.print(fdiff);
    Serial.println("F");

    matrix.clear();
    matrix.printFloat(shtf, 1);
    if (drawDots)
    {
        matrix.writeDigitAscii(0, ' ', drawDots);
    }
    else
    {
        matrix.writeDigitAscii(0, ' ', drawDots);
    }
    matrix.writeDisplay();
    delay(500);
}