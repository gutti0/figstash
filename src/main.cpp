#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>

namespace {
constexpr uint8_t SDA_PIN = 26;
constexpr uint8_t SCL_PIN = 32;
constexpr uint32_t MEASUREMENT_PERIOD_MS = 100;
constexpr uint32_t MEASUREMENT_TIMEOUT_MS = 500;

VL53L1X tof;
uint32_t lastMeasurementAt = 0;
bool timeoutReported = false;
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("ToF4M distance measurement starting...");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  Wire.setTimeOut(50);
  tof.setBus(&Wire);
  tof.setTimeout(MEASUREMENT_TIMEOUT_MS);

  if (!tof.init()) {
    Serial.println("ERROR: VL53L1X initialization failed. Check Grove wiring and power.");
    while (true) {
      delay(1000);
    }
  }

  if (!tof.setDistanceMode(VL53L1X::Long) ||
      !tof.setMeasurementTimingBudget(50000)) {
    Serial.println("ERROR: Failed to configure VL53L1X measurement mode.");
    while (true) {
      delay(1000);
    }
  }

  tof.startContinuous(MEASUREMENT_PERIOD_MS);
  lastMeasurementAt = millis();
  Serial.println("ToF4M initialized. Waiting for measurements...");
}

void loop() {
  const uint32_t now = millis();

  if (tof.dataReady()) {
    const uint16_t distanceMm = tof.read(false);
    lastMeasurementAt = millis();
    timeoutReported = false;

    if (tof.ranging_data.range_status == VL53L1X::RangeValid) {
      Serial.print("Distance: ");
      Serial.print(distanceMm);
      Serial.println(" mm");
    } else {
      Serial.print("ERROR: Measurement failed: ");
      Serial.println(VL53L1X::rangeStatusToString(tof.ranging_data.range_status));
    }
  } else if (!timeoutReported && now - lastMeasurementAt >= MEASUREMENT_TIMEOUT_MS) {
    Serial.println("ERROR: Measurement timeout. Check sensor connection and target range.");
    timeoutReported = true;
  }

  delay(5);
}
