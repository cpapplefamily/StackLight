//+--------------------------------------------------------------------------
//
// NightDriver - (c) 2020 Dave Plummer.  All Rights Reserved.
//
// File:        marque.h
//
// Description:
//
//   Draws a theatre-style marquee
//
// History:     Sep-15-2020     davepl      Created
//
//---------------------------------------------------------------------------

#include <Arduino.h>
#include <U8g2lib.h>
#define FASTLED_INTERNAL
#include <FastLED.h>


void DrawMarquee()
{
    static byte j = 0;
    j+=4;
    byte k = j;

    // Roughly equivalent to fill_rainbow(g_LEDs, NUM_LEDS, j, 8);

    CRGB c;
    for (int i = 0; i < NUM_LEDS; i ++)
        FastLED.leds()[i] = c.setHue(k+=8);

    static int scroll = 0;
    scroll++;

    for (int i = scroll % 5; i < NUM_LEDS - 1; i += 5)
    {
        FastLED.leds()[i] = CRGB::Black;
    }
    delay(50);
}

void DrawMarqueeMirrored()
{
    static byte j = 0;
    j+=4;
    byte k = j;

    // Roughly equivalent to fill_rainbow(g_LEDs, NUM_LEDS, j, 8);

    CRGB c;
    for (int i = 0; i < (NUM_LEDS + 1) / 2; i ++)
    {
        FastLED.leds()[i] = c.setHue(k);
        FastLED.leds()[NUM_LEDS - 1 - i] = c.setHue(k);
        k+= 8;
    }


    static int scroll = 0;
    scroll++;

    for (int i = scroll % 5; i < NUM_LEDS / 2; i += 5)
    {
        FastLED.leds()[i] = CRGB::Black;
        FastLED.leds()[NUM_LEDS - 1 - i] = CRGB::Black;
    }   

    delay(50);
}



