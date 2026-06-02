#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"
#include "axp2602.h"

AXP2602 axp;

void i2c_scan(void)
{
    Serial.println("Scanning I2C bus...");
    byte error, address;
    int nDevices;

    nDevices = 0;
    for (address = 0; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            nDevices++;
            Serial.print("I2C device found at address 0x");
            if (address < 16)
            {
                Serial.print("0");
            }
            Serial.print(address, HEX);
        }
    }
    Serial.println();
    if (nDevices == 0)
    {
        Serial.println("No I2C devices found\n");
    }
    else
    {
        Serial.printf("I2C devices %d\n", nDevices);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("init");
    Wire.begin(IIC_SDA_PIN, IIC_SCL_PIN);
    Wire.setClock(400000);
    i2c_scan();
    if (axp.begin() == false)
    {
        Serial.println("AXP2602 init failed");
    }
    else
    {
        Serial.println("AXP2602 init success");
    }

    axp.setLowSOCThreshold(10);   // 10% 低电量报警
    axp.setOverTempThreshold(60); // 60°C 过温报警
    axp.enable_All_IRQ(true);          // 使能中断（需要连接 IRQ 引脚处理）
}

void loop()
{
    float voltage = axp.getBatteryVoltage();
    float current = axp.getBatteryCurrent();
    int8_t temp = axp.getTemperature();
    uint8_t soc = axp.getSOC();
    uint16_t tte = axp.getTimeToEmpty();
    uint16_t ttf = axp.getTimeToFull();
    uint8_t soh = axp.getSOH();

    Serial.printf("V=%.3f V, I=%.3f A, T=%d°C, SOC=%d%%, TTE=%d min, TTF=%d min, SOH=%d%%\n",
                  voltage, current, temp, soc, tte, ttf, soh);

    delay(2000);
}