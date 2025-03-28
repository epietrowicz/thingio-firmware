#include "Particle.h"
#include "../nau7802/nau7802.h"

#define LOCATION_CALIBRATION_FACTOR 0

#define AVG_SIZE 25

int32_t readScale();
int getInventoryCount(long countsPerUnit);
void initializeScale();
void zeroScale();