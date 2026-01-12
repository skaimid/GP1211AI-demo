// ==================== WiFi Manager Header ====================
// WiFi管理模块
// 负责WiFi连接、NTP时间同步和网络状态管理

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// ==================== WiFi Manager类 ====================
class WiFiManager {
public:
    WiFiManager();

    // WiFi连接
    bool connect(const char* ssid, const char* password, int maxAttempts = 20);
    void disconnect();
    bool isConnected() const;
    String getIPAddress() const;

    // NTP时间同步
    bool syncTime(const char* ntpServer, long timezoneOffset);
    bool getLocalTime(struct tm* timeinfo);

    // 时间格式化
    String getTimeString();         // 返回 "HH:MM:SS"
    String getDateString();         // 返回 "MM/DD"
    String getFullDateString();     // 返回 "YYYY-MM-DD"

private:
    bool wifiConnected;
    struct tm currentTime;
};

// 全局WiFiManager对象（在cpp中定义）
extern WiFiManager wifiMgr;

#endif // WIFI_MANAGER_H
