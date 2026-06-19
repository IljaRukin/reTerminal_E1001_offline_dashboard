#include "sd_manager.h"
#include "config.h"
#include "dataSeries.h"

// Use the HSPI bus for the SD card to avoid conflict with other peripherals
SPIClass spiSD(HSPI);

// Global variables to track SD card state
bool sdMounted = false;
bool lastCardPresent = false;
unsigned long lastCheckMs = 0;
const unsigned long checkIntervalMs = 1000; // Check for card changes every second

// Checks if a card is physically inserted.
// The detection pin is LOW when a card is present.
bool SdManager::isCardInserted() {
  return digitalRead(SD_DET_PIN) == LOW;
}

bool SdManager::begin()
{
    if (m_initialized) {
        return true;
    }

    pinMode(SD_DET_PIN, INPUT_PULLUP);
    pinMode(SD_EN_PIN, OUTPUT);
    digitalWrite(SD_EN_PIN, LOW);
    
    m_spi.end();
    m_spi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    m_initialized = true;
    LOG.println("[SD] initialized");
    return true;
}

bool SdManager::end()
{
    if (!m_initialized) {
        return true;
    }
    m_spi.end();
    m_initialized = false;
    LOG.println("[SD] deinitialized");
    return true;
}

bool SdManager::mountCard()
{
    digitalWrite(SD_EN_PIN, HIGH);
    if (!SD.begin(SD_CS_PIN, m_spi)) {
        LOG.println("[SD] mount failed");
        m_spi.end();
        unmountCard();
        return false;
    }
    m_mounted = true;
    LOG.println("[SD] mounted");
    return true;
}

bool SdManager::unmountCard()
{
    digitalWrite(SD_EN_PIN, LOW);
    SD.end();
    m_mounted = false;
    LOG.println("[SD] unmounted");
    return true;
}

// Helper function to print indentation for directory listing
void SdManager::printIndent(uint8_t level) {
  for (uint8_t i = 0; i < level; ++i) {
    LOG.print(" ");
  }
}

// Recursively lists files and directories
void SdManager::listDir(File dir, uint8_t level) {
  LOG.printf("--------------------[FC]--------------------\n");
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
    LOG.printf("[SD] Failed to open directory %s.\n",entry.name());
      // No more entries in this directory
      break;
    }

    printIndent(level);
    if (entry.isDirectory()) {
      LOG.printf("[DIR] %s  is not a directory.\n",entry.name());
      // Recurse into the subdirectory
      listDir(entry, level + 1);
    } else {
      // It's a file, print its name and size
      LOG.print("[FILE] ");
      LOG.print(entry.name());
      LOG.print("  ");
      LOG.print(entry.size());
      LOG.println(" bytes");
    }
    entry.close();
  }
  LOG.printf("--------------------[FC]--------------------\n");
}

bool SdManager::ensureCsvHeader()
{
    if (!m_mounted) {
        return false;
    }

    if (SD.exists(INTERNAL_CSV_PATH)) {
        return true;
    } else {
        File f = SD.open(INTERNAL_CSV_PATH, FILE_WRITE);
        if (!f) {
            return false;
        }
        f.print(CSV_HEADER_INTERNAL);
        f.close();
    }

    return true;
}

bool SdManager::appendReading(sensorInternal_t sensorInternal)
{
    if (!m_mounted) {
        return false;
    }

    File f = SD.open(INTERNAL_CSV_PATH, FILE_APPEND);
    if (!f) {
        return false;
    }

    f.printf("%i,%.2f,%.2f\n",
     (uint64_t)sensorInternal.timeStamp,
     sensorInternal.temperature,
     sensorInternal.humidity);
    f.close();
    return true;
}

bool SdManager::readRecentReadings(historicData_t &historicData, time_t startTime)
{
    if (!m_mounted) {
        return false;
    }

    File f = SD.open(INTERNAL_CSV_PATH, FILE_READ);
    if (!f) {
        return false;
    }

    // Skip header.
    if (f.available()) {
        String header = f.readStringUntil('\n');
        printf("[SD] read in header: %s\n", header.c_str());
        (void)header;
    }

    String timeStr;
    String temperatureStr;
    String humidityStr;
    time_t parsedTime;
    float parsedTemperature;
    float parsedHumidity;
    time_t min_Time = std::numeric_limits<time_t>::max();
    time_t max_Time = std::numeric_limits<time_t>::min();

    while (f.available()) {
        String entry = f.readStringUntil('\n');
        entry.trim();
        if (entry.length() == 0) {
            continue;  // Skip empty lines
        }

        int firstComma = entry.indexOf(',');
        int secondComma = entry.indexOf(',', firstComma + 1);
        if (firstComma < 0 || secondComma < 0) {
            //LOG.printf("[SD] malformed CSV line: %s\n", entry.c_str());
            continue;
        }

        timeStr = entry.substring(0, firstComma);
        temperatureStr = entry.substring(firstComma + 1, secondComma);
        humidityStr = entry.substring(secondComma + 1);

        parsedTime = static_cast<time_t>(timeStr.toInt());

        if (parsedTime < startTime) {
            //LOG.printf("[SD] skipping entry: %s\n", entry.c_str());
            continue;
        } else {
            //LOG.printf("[SD] parsing entry: %s\n", entry.c_str());

            parsedTemperature = temperatureStr.toFloat();
            parsedHumidity = humidityStr.toFloat();

            min_Time = min(min_Time,parsedTime);
            max_Time = max(max_Time,parsedTime);

            historicData.temperature.insert({parsedTime,parsedTemperature});
            historicData.min_temperature = min(historicData.min_temperature, parsedTemperature);
            historicData.max_temperature = max(historicData.max_temperature, parsedTemperature);
            historicData.relative_humidity.insert({parsedTime,parsedHumidity});
        }
    }
    
    f.close();
    if (min_Time < std::numeric_limits<time_t>::max()) {
        historicData.min_Time = min_Time;
        historicData.max_Time = max_Time;
        return true;
    } else {
        return false;
    }
}

bool SdManager::readForecast(String &json)
{
    if (!m_mounted) {
        return false;
    }
    if (!SD.exists(FORECAST_PATH)) {
        return false;
    }

    File f = SD.open(FORECAST_PATH, FILE_READ);
    if (!f) {
        return false;
    }

    json = f.readString();
    f.close();

    return true;
}

bool SdManager::writeForecast(const String &json)
{
    if (!m_mounted) {
        return false;
    }

    // Write atomically via a temperature file.
    File f = SD.open(FORECAST_PATH, FILE_WRITE);
    if (!f) {
        return false;
    }
    f.print(json);
    f.close();

    return true;
}