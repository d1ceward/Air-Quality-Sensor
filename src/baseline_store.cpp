#include <ArduinoJson.h>
#include <LittleFS.h>

// Local includes
#include "app_config.h"
#include "baseline_store.h"
#include "config.h"

namespace BaselineStore {
  namespace {
    // A baseline of zero on both channels is what the sensor reports before it has calibrated anything, so
    // it is indistinguishable from no baseline at all
    bool isUsable(const Baseline &value) {
      return value.eco2 != 0 || value.tvoc != 0;
    }

    // Configuration written by 1.1.0 and earlier kept the baseline inline
    bool loadLegacy(Baseline &result) {
      const AppConfig &config = Config::get();

      Baseline legacy;
      legacy.eco2 = config.legacySgpBaselineCo2;
      legacy.tvoc = config.legacySgpBaselineTvoc;
      legacy.epoch = config.legacySgpBaselineEpoch;

      if (!isUsable(legacy))
        return false;

      result = legacy;
      Serial.println(F("[baseline] migrating the baseline out of the configuration"));

      // Moving it across now means the next configuration save drops the old keys for good
      save(result);

      return true;
    }
  }

  bool load(Baseline &result) {
    if (!Config::isStorageAvailable())
      return false;

    if (LittleFS.exists(BASELINE_TMP_PATH))
      LittleFS.remove(BASELINE_TMP_PATH);

    File file = LittleFS.open(BASELINE_PATH, "r");
    if (!file)
      return loadLegacy(result);

    JsonDocument document;
    const DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error) {
      Serial.printf("[baseline] invalid file (%s), ignoring\n", error.c_str());
      return false;
    }

    Baseline stored;
    stored.eco2 = document["eco2"] | 0;
    stored.tvoc = document["tvoc"] | 0;
    stored.epoch = document["epoch"] | 0UL;

    if (!isUsable(stored))
      return false;

    result = stored;

    return true;
  }

  bool save(const Baseline &value) {
    JsonDocument document;
    document["eco2"] = value.eco2;
    document["tvoc"] = value.tvoc;
    document["epoch"] = value.epoch;

    return Config::writeJsonAtomically(BASELINE_PATH, BASELINE_TMP_PATH, document);
  }
}
