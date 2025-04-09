#include "Particle.h"
#include "scale/scale.h"
#include "temp/temp.h"

SYSTEM_MODE(AUTOMATIC);

PRODUCT_VERSION(1);

#define UNLOCK_DELAY 500
#define SOL_STATE_1 D4
#define SOL_SIG_1 D23
#define REFRESH_PERCENT 0.3

static int32_t scaleReading = 0;
static int inventoryCount = 0;
static float temperature = 0;

long unsigned int statusInterval = 15 * 60 * 1000;
static unsigned long lastStatusTime = 0;
static long countsPerUnit = 1;

static volatile bool shouldUnlock = false;
static volatile bool resetLock = false;
static volatile bool shouldCalibrate = false;
static volatile bool shouldTare = false;
static volatile unsigned long unlockTime = 0;
static bool lastLockState = false;
static bool currentLockState = false;
Ledger statusLedger;

SerialLogHandler logHandler;
// SerialLogHandler logHandler(
//     LOG_LEVEL_NONE, // Default logging level for all categories
//     {
//         {"app", LOG_LEVEL_ALL} // Only enable all log levels for the application
//     });

int tare(String command)
{
  shouldTare = true;
  return 0;
}

int calibrate(String command)
{
  shouldCalibrate = true;
  return 0;
}

int unlock(String command)
{
  shouldUnlock = true;
  return 0;
}

int refresh(String command)
{
  lastStatusTime = 0;
  return 0;
}

void setup()
{
  Serial.begin(115200);
  Cellular.prefer();

  pinMode(SOL_SIG_1, OUTPUT);
  pinMode(SOL_STATE_1, INPUT_PULLUP);

  Particle.function("tare", tare);
  Particle.function("calibrate", calibrate);
  Particle.function("unlock", unlock);
  Particle.function("refresh", refresh);

  statusLedger = Particle.ledger("thingio-status-d2c");

  EEPROM.get(LOCATION_CALIBRATION_FACTOR, countsPerUnit);
  if (countsPerUnit == 0x7FFFFFFF)
  {
    countsPerUnit = 1;
  }

  Log.info("Got %ld counts per unit", countsPerUnit);
  initializeScale();
  delay(2000);
  unlock("");

  lastLockState = digitalRead(SOL_STATE_1);
}

void loop()
{
  if (Particle.connected())
  {
    Variant data;

    scaleReading = readScale();
    inventoryCount = getInventoryCount(countsPerUnit);
    temperature = readTemperature();

    if (lastStatusTime == 0 || ((millis() - lastStatusTime) >= statusInterval))
    {
      if (Time.isValid())
      {
        data.set("time", Time.format(TIME_FORMAT_ISO8601_FULL));
      }
      data.set("raw", scaleReading);
      data.set("inventory", inventoryCount);
      data.set("temperature", temperature);
      data.set("locked", !digitalRead(SOL_STATE_1));
      data.set("counts_per_unit", countsPerUnit);

      Log.info("%s", data.toJSON().c_str());
      statusLedger.set(data, particle::Ledger::MERGE);
      Particle.publish("evt", "LEDGER_SYNC");

      lastStatusTime = millis();
    }
  }
  // Log.info("Scale diff %ld raw %ld", abs(scaleReading - lastScaleReading), scaleReading);
  // if (abs(scaleReading - lastScaleReading) > refreshThreshold)
  // {
  //   Log.info("Large change detected, forcing a system refresh!");
  //   lastStatusTime = 0; // Force a system status update
  // }
  // lastScaleReading = scaleReading;

  if (shouldUnlock)
  {
    Log.info("Unlocking...");
    digitalWrite(SOL_SIG_1, 1);
    unlockTime = millis();
    lastStatusTime = 0; // Force a system status update
    shouldUnlock = false;
    resetLock = true;
  }
  if (resetLock && (millis() - unlockTime) >= UNLOCK_DELAY)
  {
    Log.info("Reset lock pin");
    digitalWrite(SOL_SIG_1, 0);
    resetLock = false;
  }
  if (shouldCalibrate)
  {
    countsPerUnit = readScale();
    EEPROM.put(LOCATION_CALIBRATION_FACTOR, countsPerUnit);

    shouldCalibrate = false;
    lastStatusTime = 0; // Force a system status update
    Log.info("Calibrated: %ld", countsPerUnit);
  }
  if (shouldTare)
  {
    zeroScale();
    shouldTare = false;
    lastStatusTime = 0; // Force a system status update
    Log.info("Zero'd scale");
  }

  // TODO: Test this!!
  currentLockState = digitalRead(SOL_STATE_1);
  if (currentLockState != lastLockState && !currentLockState)
  {
    Log.info("Lock state changed to %d", currentLockState);
    lastStatusTime = 0; // Force a system status update
  }

  lastLockState = currentLockState;
  delay(250);
}