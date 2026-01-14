// ==================== WiFi Manager Implementation ====================

#include "wifi_manager.h"

// 全局WiFiManager对象
WiFiManager wifiMgr;

// ==================== 构造函数 ====================
WiFiManager::WiFiManager()
    : wifiConnected(false)
{
    memset(&currentTime, 0, sizeof(currentTime));
}

// ==================== WiFi连接 ====================
bool WiFiManager::connect(const char* ssid, const char* password, int maxAttempts) {
    Serial.println("\n连接WiFi...");
    Serial.printf("SSID: %s\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\n✓ WiFi连接成功");
        Serial.print("  IP地址: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        wifiConnected = false;
        Serial.println("\n✗ WiFi连接失败");
        return false;
    }
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    wifiConnected = false;
    Serial.println("WiFi已断开");
}

bool WiFiManager::isConnected() const {
    return wifiConnected && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getIPAddress() const {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

// ==================== NTP时间同步 ====================
bool WiFiManager::syncTime(const char* ntpServer, long timezoneOffset) {
    if (!isConnected()) {
        Serial.println("✗ WiFi未连接，无法同步时间");
        return false;
    }

    Serial.println("同步时间...");
    configTime(timezoneOffset, 0, ntpServer);

    int attempts = 0;
    while (!::getLocalTime(&currentTime) && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (::getLocalTime(&currentTime)) {
        Serial.println("\n✓ 时间同步成功");
        Serial.printf("  当前时间: %04d-%02d-%02d %02d:%02d:%02d\n",
                     currentTime.tm_year + 1900, currentTime.tm_mon + 1,
                     currentTime.tm_mday, currentTime.tm_hour,
                     currentTime.tm_min, currentTime.tm_sec);
        return true;
    } else {
        Serial.println("\n✗ 时间同步失败");
        return false;
    }
}

bool WiFiManager::getLocalTime(struct tm* timeinfo) {
    return ::getLocalTime(timeinfo);
}

// ==================== 时间格式化 ====================
String WiFiManager::getTimeString() {
    if (!::getLocalTime(&currentTime)) {
        return "--:--:--";
    }

    char buffer[16];
    sprintf(buffer, "%02d:%02d:%02d",
            currentTime.tm_hour, currentTime.tm_min, currentTime.tm_sec);
    return String(buffer);
}

String WiFiManager::getDateString() {
    if (!::getLocalTime(&currentTime)) {
        return "--/--";
    }

    char buffer[16];
    sprintf(buffer, "%02d/%02d", currentTime.tm_mon + 1, currentTime.tm_mday);
    return String(buffer);
}

String WiFiManager::getFullDateString() {
    if (!::getLocalTime(&currentTime)) {
        return "----/--/--";
    }

    char buffer[32];
    sprintf(buffer, "%04d-%02d-%02d",
            currentTime.tm_year + 1900, currentTime.tm_mon + 1, currentTime.tm_mday);
    return String(buffer);
}
