#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>

namespace {
constexpr uint8_t SDA_PIN = 26;
constexpr uint8_t SCL_PIN = 32;
constexpr uint8_t PAHUB_ADDRESS = 0x70;
constexpr uint8_t TOF_ADDRESS = 0x29;
constexpr uint32_t OUTPUT_PERIOD_MS = 100;
constexpr uint32_t MEASUREMENT_TIMEOUT_MS = 500;
constexpr uint32_t RETRY_PERIOD_MS = 2000;

struct AxisSensor {
  AxisSensor(const char* axisName, uint8_t hubChannel)
      : name(axisName), channel(hubChannel) {}

  const char* name;
  uint8_t channel;
  VL53L1X tof;
  bool initialized = false;
  bool hasDistance = false;
  uint16_t distanceMm = 0;
  uint32_t lastMeasurementAt = 0;
  uint32_t lastInitAttemptAt = 0;
  const char* error = "initializing";
};

AxisSensor sensors[] = {{"x", 0}, {"y", 1}, {"z", 2}};
constexpr size_t SENSOR_COUNT = sizeof(sensors) / sizeof(sensors[0]);
uint32_t lastOutputAt = 0;

bool selectChannel(uint8_t channel) {
  Wire.beginTransmission(PAHUB_ADDRESS);
  Wire.write(static_cast<uint8_t>(1U << channel));
  return Wire.endTransmission() == 0;
}

bool sensorResponds() {
  Wire.beginTransmission(TOF_ADDRESS);
  return Wire.endTransmission() == 0;
}

void fail(AxisSensor& sensor, const char* error, bool retryInit = false) {
  sensor.hasDistance = false;
  sensor.error = error;
  if (retryInit) {
    sensor.initialized = false;
    sensor.lastInitAttemptAt = millis();
  }
}

void initialize(AxisSensor& sensor) {
  sensor.lastInitAttemptAt = millis();
  if (!selectChannel(sensor.channel)) {
    fail(sensor, "channel_select_failed");
    return;
  }
  if (!sensorResponds()) {
    fail(sensor, "sensor_missing");
    return;
  }

  // Recreate the library state after a disconnect or sensor reset.
  sensor.tof = VL53L1X();
  sensor.tof.setBus(&Wire);
  sensor.tof.setTimeout(MEASUREMENT_TIMEOUT_MS);
  if (!sensor.tof.init()) {
    fail(sensor, "init_failed");
    return;
  }
  if (!sensor.tof.setDistanceMode(VL53L1X::Long) ||
      !sensor.tof.setMeasurementTimingBudget(50000)) {
    fail(sensor, "config_failed");
    return;
  }

  sensor.tof.startContinuous(OUTPUT_PERIOD_MS);
  sensor.initialized = true;
  sensor.lastMeasurementAt = millis();
  sensor.hasDistance = false;
  sensor.error = "waiting_for_measurement";
}

void update(AxisSensor& sensor) {
  if (!sensor.initialized) {
    if (millis() - sensor.lastInitAttemptAt >= RETRY_PERIOD_MS) {
      initialize(sensor);
    }
    return;
  }

  if (!selectChannel(sensor.channel)) {
    fail(sensor, "channel_select_failed", true);
    return;
  }
  if (!sensorResponds()) {
    fail(sensor, "sensor_missing", true);
    return;
  }

  if (sensor.tof.dataReady()) {
    const uint16_t distanceMm = sensor.tof.read(false);
    if (sensor.tof.timeoutOccurred() || sensor.tof.last_status != 0) {
      fail(sensor, "measurement_io_failed", true);
    } else if (sensor.tof.ranging_data.range_status != VL53L1X::RangeValid) {
      fail(sensor, "range_invalid");
      sensor.lastMeasurementAt = millis();
    } else {
      sensor.distanceMm = distanceMm;
      sensor.lastMeasurementAt = millis();
      sensor.hasDistance = true;
      sensor.error = nullptr;
    }
  } else if (sensor.tof.last_status != 0) {
    fail(sensor, "measurement_io_failed", true);
  } else if (millis() - sensor.lastMeasurementAt >= MEASUREMENT_TIMEOUT_MS) {
    fail(sensor, "measurement_timeout", true);
  }
}

void printDistance(const AxisSensor& sensor) {
  Serial.print('"');
  Serial.print(sensor.name);
  Serial.print("\":");
  if (sensor.hasDistance) {
    Serial.print(sensor.distanceMm);
  } else {
    Serial.print("null");
  }
}

void printError(const AxisSensor& sensor) {
  Serial.print('"');
  Serial.print(sensor.name);
  Serial.print("\":");
  if (sensor.error == nullptr) {
    Serial.print("null");
  } else {
    Serial.print('"');
    Serial.print(sensor.error);
    Serial.print('"');
  }
}

void printReading() {
  Serial.print("{\"type\":\"distance\",");
  for (size_t i = 0; i < SENSOR_COUNT; ++i) {
    if (i != 0) Serial.print(',');
    printDistance(sensors[i]);
  }
  Serial.print(",\"errors\":{");
  for (size_t i = 0; i < SENSOR_COUNT; ++i) {
    if (i != 0) Serial.print(',');
    printError(sensors[i]);
  }
  Serial.println("}}");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  Wire.setTimeOut(50);

  for (auto& sensor : sensors) {
    initialize(sensor);
  }
  lastOutputAt = millis();
}

void loop() {
  for (auto& sensor : sensors) {
    update(sensor);
  }

  if (millis() - lastOutputAt >= OUTPUT_PERIOD_MS) {
    lastOutputAt = millis();
    printReading();
  }
  delay(5);
}
