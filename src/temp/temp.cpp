#include "temp.h"

const float scalingFactor = 0.01;

float readTemperature()
{
    float total = 0.0;
    uint8_t samplesAcquired = 0;
    uint8_t averageAmount = 64;

    while (1)
    {
        float voltage = (analogRead(TEMP_ANALOG) * 3.3) / 4095.0;
        float temp = voltage / scalingFactor;
        total += temp;
        if (++samplesAcquired == averageAmount)
            break; // All done

        delay(1);
    }
    total /= averageAmount;
    return total;
}

void initializeTemp()
{
    pinMode(TEMP_ANALOG, INPUT);
}