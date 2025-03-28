#include "scale.h"

NAU7802 scale;
int32_t latestReading = 0;

// Create an array to take average of weights. This helps smooth out jitter.
int32_t avgReadings[AVG_SIZE];
byte avgReadingIdx = 0;

int32_t readScale()
{
    if (scale.available() == true)
    {
        int32_t currentReading = scale.getAverage(20, 5000);
        latestReading = currentReading;
        // avgReadings[avgReadingIdx++] = currentReading;
        // if (avgReadingIdx == AVG_SIZE)
        //     avgReadingIdx = 0;

        // int32_t avgReading = 0;
        // for (int x = 0; x < AVG_SIZE; x++)
        //     avgReading += avgReadings[x];
        // double avgCalc = (double)(avgReading) / AVG_SIZE;
        // latestReading = (int32_t)(avgCalc);
    }
    return latestReading;
}

int getInventoryCount(long countsPerUnit)
{
    return (int32_t)(latestReading / (double)(countsPerUnit) + 0.5);
}

void zeroScale()
{
    scale.calibrateAFE(NAU7802_CALMOD_OFFSET);
    delay(50);

    for (int x = 0; x < AVG_SIZE; x++)
    {
        int32_t tmpReading = scale.getReading();
        avgReadings[x] = tmpReading;
    }
}

void initializeScale()
{
    if (scale.begin() == false)
    {
        Log.info("Scale not detected. Please check wiring. Freezing...");
    }
    scale.setSampleRate(NAU7802_SPS_80); // Set sample rate: 10, 20, 40, 80 or 320
    scale.setGain(NAU7802_GAIN_32);      // Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128.
    scale.setLDO(NAU7802_LDO_3V0);       // Set LDO voltage. 3.0V is the best choice for Qwiic
    scale.calibrateAFE(NAU7802_CALMOD_INTERNAL);

    delay(500);
    // Take 10 readings to flush out readings
    for (uint8_t i = 0; i < 10; i++)
    {
        while (!scale.available())
            delay(1);
        scale.getReading();
    }
    zeroScale();
}