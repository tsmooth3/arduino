#include <FastLED.h>
#define NUM_LEDS_PER_DIGIT 64
#define NUM_LEDS 139
#define DATA_PIN 6

CRGB leds[NUM_LEDS];

// 7-Segment display
//    /-A-\
//    F   B
//    --G-/
//    E   C
//    \-D-/

//    /-B-\
//    G   A
//    --F-/
//    E   C
//    \-D-/

// 9 LEDS per segment
// bitwise operations are MSB - counted from right and padding with 0s up to 64 bits
// e.g.
// 0b0 ccccccccc dddddddddeeeeeeeeefffffffffaaaaaaaaabbbbbbbbb 0 ggggggggg

const uint64_t digits0[10] = {
    // 0bccccc0ddddddddddeeeeeeeeee0fffffffffaaaaaaaaa0bbbbbbbb0ggggggggg 000
    //  0b1111101111111111111111111100000000001111111110111111110111111111, // 0
    0b0000000000000000000000000000000000000000000000000000000000000000, // 0
    0b1111100000000000000000000000000000001111111110000000000000000000, // 1
    0b0000001111111111111111111101111111111111111110111111110000000000, // 2
    0b1111101111111111000000000001111111111111111110111111110000000000, // 3
    0b1111100000000000000000000001111111111111111110000000000111111111, // 4
    0b1111101111111111000000000001111111110000000000111111110111111111, // 5
    0b1111101111111111111111111101111111110000000000111111110111111111, // 6
    0b1111100000000000000000000000000000001111111110111111110000000000, // 7
    0b1111101111111111111111111101111111111111111110111111110111111111, // 8
    0b1111101111111111000000000001111111111111111110111111110111111111  // 9
};

const uint64_t digits1[10] = {
    // 0bccccc0dddddddddd0eeeeeeeee0fffffffff0aaaaaaaaabbbbbbbbbggggggggg
    0b1111101111111111011111111100000000000111111111111111111111111111, // 0
    0b1111100000000000000000000000000000000111111111000000000000000000, // 1
    0b0000000111111111011111111101111111110111111111111111111000000000, // 2
    0b1111101111111111000000000000111111110111111111111111111000000000, // 3
    0b1111100000000000000000000000111111110111111111000000000011111111, // 4
    0b1111101111111111000000000001111111110000000000111111111111111111, // 5
    0b1111101111111111011111111101111111110000000000111111111111111111, // 6
    0b1111100000000000000000000000000000000111111111111111111000000000, // 7
    0b1111101111111111011111111101111111110111111111111111111111111111, // 8
    0b1111101111111111000000000001111111110111111111111111111111111111  // 9
};

void setDigit(int display, int val, CHSV color)
{
    if (display == 0)
    {
        for (int i = 0; i < NUM_LEDS_PER_DIGIT; i++)
        {
            color.v = bitRead(digits0[val], i) * 255;
            leds[display * NUM_LEDS_PER_DIGIT + i] = color;
        }
        switch (val)
        {
        case 0:
            color.v = 0;
            for (int i = 64; i < 68; i++)
            {
                leds[i] = color;
            }
            break;
        case 2:
            color.v = 0;
            for (int i = 64; i < 68; i++)
            {
                leds[i] = color;
            }
            break;
        default:
            color.v = 255;
            for (int i = 64; i < 68; i++)
            {
                leds[i] = color;
            }
            break;
        }
    }
    if (display == 1)
    {
        for (int i = 0; i < NUM_LEDS_PER_DIGIT; i++)
        {
            color.v = bitRead(digits1[val], i) * 255;
            leds[7 + display * NUM_LEDS_PER_DIGIT + i] = color;
        }
        switch (val)
        {
        case 2:
            color.v = 0;
            for (int i = 135; i < 139; i++)
            {
                leds[i] = color;
            }
            break;
        default:
            color.v = 255;
            for (int i = 135; i < 139; i++)
            {
                leds[i] = color;
            }
            break;
        }
    }
}

void blinkLED(int PIN, int interval)
{
    digitalWrite(PIN, HIGH);
    delay(interval);
    digitalWrite(PIN, LOW);
    delay(interval);
    digitalWrite(PIN, HIGH);
    delay(interval);
    digitalWrite(PIN, LOW);
    delay(1000);
}

void blinkSpeed(int speed0, int speed1, int color, int ms)
{
    for (int c = 0; c < 3; c++)
    {
        FastLED.clear();
        FastLED.show();
        delay(ms);
        setDigit(0, speed0, CHSV(color, 0, 255));
        setDigit(1, speed1, CHSV(color, 0, 255));
        FastLED.show();
        delay(ms);
    }
}
void blinkZeros(int speed0, int speed1, int color, int ms)
{
    for (int c = 0; c < 2; c++)
    {
        FastLED.clear();
        FastLED.show();
        delay(ms);
        setDigit(0, speed0, CHSV(color, 255, 255));
        setDigit(1, speed1, CHSV(color, 255, 255));
        FastLED.show();
        delay(ms);
    }
}

void setup()
{
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    Serial.begin(19200);
    pinMode(13, OUTPUT);
    Serial1.begin(19200);
    Serial.println("ready to go");
}
int inSpeed = 0;
int colorCount = 0;
int bump = 1;

void loop()
{ // run over and over

    if (Serial1.available())
    {
        int inSpeed = Serial1.parseInt();
        if (inSpeed > 0)
        {
            Serial.println(inSpeed);
            int inSpeed0 = inSpeed / 10;
            int inSpeed1 = inSpeed % 10;
            blinkSpeed(inSpeed0, inSpeed1, colorCount, 50);
            colorCount = 0;
            delay(50);
        }
    }
    else
    {
        blinkZeros(0, 0, colorCount, 0);
    }
    colorCount = colorCount + (3 * bump);
    if (colorCount > 254)
    {
        bump = -1;
    }
    if (colorCount < 0)
    {
        bump = 1;
    }
}