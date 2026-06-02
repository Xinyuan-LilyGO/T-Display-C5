#include "Arduino.h"
#include "SPI.h"

#define SPI_MOSI 9
#define SPI_MISO -1
#define SPI_SCK 7
#define SPI_CS 8

uint8_t testData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
// SPIClass SPI;

void setup()
{
    Serial.begin(115200);
    Serial.println("SPI test");
    delay(1000);

    pinMode(SPI_CS, OUTPUT);
    digitalWrite(SPI_CS, HIGH); // 初始状态为高电平

    if (SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS))
    {
        Serial.println("SPI.begin() succeeded");
        Serial.printf("SCK: %d, MOSI: %d, CS: %d\n", SPI_SCK, SPI_MOSI, SPI_CS);
    }
    else
    {
        Serial.println("SPI.begin() failed");
    }
    SPI.setFrequency(10000000);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
}

void loop()
{
    //  SPI传输测试
    Serial.println("Starting SPI transfer...");

    // 开始SPI事务
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));

    // 拉低CS信号，选中设备
    digitalWrite(SPI_CS, LOW);

    // 发送测试数据
    for (int i = 0; i < 8; i++)
    {
        uint8_t received = SPI.transfer(testData[i]);
        Serial.printf("Sent: 0x%02X, Received: 0x%02X\n", testData[i], received);
        delay(10); // 添加延时，便于观察
    }

    // 拉高CS信号，取消选中设备
    digitalWrite(SPI_CS, HIGH);

    // 结束SPI事务
    SPI.endTransaction();

    Serial.println("SPI transfer completed");
    Serial.println("-------------------");

    // 等待5秒后再次测试
    delay(2000);
}