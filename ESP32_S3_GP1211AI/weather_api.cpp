// ==================== Weather API Implementation ====================

#include "weather_api.h"

// 全局WeatherAPI对象
WeatherAPI weatherAPI;

// ==================== 构造函数 ====================
WeatherAPI::WeatherAPI()
    : lastUpdate(0)
{
}

// ==================== 配置API ====================
void WeatherAPI::configure(const char* key, const char* cityName, const char* countryCode) {
    apiKey = String(key);
    city = String(cityName);
    country = String(countryCode);

    Serial.println("天气API配置完成");
    Serial.printf("  城市: %s, %s\n", city.c_str(), country.c_str());
}

// ==================== 获取当前天气 ====================
bool WeatherAPI::fetchCurrentWeather() {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
                 city + "," + country +
                 "&appid=" + apiKey + "&units=metric";

    Serial.println("获取当前天气...");
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            currentWeather.temp = doc["main"]["temp"];
            currentWeather.humidity = doc["main"]["humidity"];
            currentWeather.description = doc["weather"][0]["description"].as<String>();
            currentWeather.weatherId = doc["weather"][0]["id"];
            currentWeather.valid = true;

            lastUpdate = millis();

            Serial.println("✓ 天气数据获取成功");
            Serial.printf("  温度: %.1f°C\n", currentWeather.temp);
            Serial.printf("  湿度: %d%%\n", currentWeather.humidity);
            Serial.printf("  状况: %s\n", currentWeather.description.c_str());

            http.end();
            return true;
        }
    }

    http.end();
    Serial.printf("✗ 天气获取失败 (HTTP: %d)\n", httpCode);
    currentWeather.valid = false;
    return false;
}

// ==================== 获取小时预报 ====================
bool WeatherAPI::fetchHourlyForecast() {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" +
                 city + "," + country +
                 "&appid=" + apiKey + "&units=metric&cnt=8";

    Serial.println("获取小时预报...");
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            JsonArray list = doc["list"];
            for (int i = 0; i < 8 && i < list.size(); i++) {
                hourlyForecast[i].temp = list[i]["main"]["temp"];
                hourlyForecast[i].humidity = list[i]["main"]["humidity"];
                hourlyForecast[i].weatherId = list[i]["weather"][0]["id"];
                hourlyForecast[i].description = list[i]["weather"][0]["description"].as<String>();
                hourlyForecast[i].valid = true;
            }

            Serial.println("✓ 小时预报获取成功");
            http.end();
            return true;
        }
    }

    http.end();
    Serial.printf("✗ 小时预报获取失败 (HTTP: %d)\n", httpCode);
    return false;
}

// ==================== 获取数据 ====================
const WeatherData& WeatherAPI::getCurrentWeather() const {
    return currentWeather;
}

const WeatherData& WeatherAPI::getHourlyForecast(int index) const {
    if (index >= 0 && index < 8) {
        return hourlyForecast[index];
    }
    static WeatherData empty;
    return empty;
}

// ==================== 自动更新检查 ====================
bool WeatherAPI::needsUpdate() const {
    return (millis() - lastUpdate) >= WEATHER_UPDATE_INTERVAL;
}

// ==================== 根据天气ID获取图标 ====================
const uint8_t* WeatherAPI::getWeatherIcon(int weatherId) const {
    if (weatherId >= 200 && weatherId < 300) {
        return icon_thunderstorm;  // 雷暴
    } else if (weatherId >= 300 && weatherId < 600) {
        return icon_rainy;  // 雨
    } else if (weatherId >= 600 && weatherId < 700) {
        return icon_snowy;  // 雪
    } else if (weatherId >= 700 && weatherId < 800) {
        return icon_mist;  // 雾/霾
    } else if (weatherId == 800) {
        return icon_sunny;  // 晴天
    } else {
        return icon_cloudy;  // 多云
    }
}
