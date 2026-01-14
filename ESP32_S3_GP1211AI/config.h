// ==================== 配置文件 ====================
// 在此文件中配置 WiFi、时区和天气 API

#ifndef CONFIG_H
#define CONFIG_H

// WiFi 配置
#define WIFI_SSID     "Your_WiFi_SSID"      // 修改为你的WiFi名称
#define WIFI_PASSWORD "Your_WiFi_Password"  // 修改为你的WiFi密码

// 时区配置（中国标准时间 UTC+8）
#define TIMEZONE_OFFSET 8 * 3600  // 秒数
#define NTP_SERVER "pool.ntp.org"

// 天气API配置（使用OpenWeatherMap）
// 获取免费API密钥: https://openweathermap.org/api
#define WEATHER_API_KEY "your_api_key_here"  // 修改为你的API密钥
#define WEATHER_CITY    "Beijing"             // 城市名称
#define WEATHER_COUNTRY "CN"                  // 国家代码

// 天气更新间隔（毫秒）
#define WEATHER_UPDATE_INTERVAL 600000  // 10分钟

#endif // CONFIG_H
