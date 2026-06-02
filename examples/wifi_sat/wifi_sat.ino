#include <WiFi.h>

// WiFi热点配置
const char *apSSID = "ESP32C5_AP";   // 热点名称
const char *apPassword = "12345678"; // 热点密码(至少8个字符)

void setup()
{
    // 初始化串口
    Serial.begin(115200);
    delay(1000);

    Serial.println("\nESP32-C5 WiFi热点示例");

    // 设置WiFi为AP模式
    WiFi.mode(WIFI_AP);

    // 配置热点
    WiFi.softAP(apSSID, apPassword);

    // 获取热点IP地址
    IPAddress IP = WiFi.softAPIP();

    Serial.print("热点名称(SSID): ");
    Serial.println(apSSID);
    Serial.print("热点IP地址: ");
    Serial.println(IP);
    Serial.print("热点密码: ");
    Serial.println(apPassword);
    Serial.println("热点已启动，等待设备连接...");
}

void loop()
{
    // 检查连接的设备数量
    unsigned char number_client = WiFi.softAPgetStationNum();

    if (number_client > 0)
    {
        Serial.printf("当前连接设备数: %d\n", number_client);
    }

    delay(5000); // 每5秒检查一次
}
