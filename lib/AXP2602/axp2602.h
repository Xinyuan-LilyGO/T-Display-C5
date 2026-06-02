#ifndef AXP2602_H
#define AXP2602_H

#include <Arduino.h>
#include <Wire.h>

// I2C 地址（7位）
#define AXP2602_I2C_ADDR_DEFAULT 0x62 // 0xC4 >> 1

// 寄存器地址
#define AXP2602_REG_ID 0x00
#define AXP2602_REG_BROM 0x01
#define AXP2602_REG_MODE 0x02
#define AXP2602_REG_PARA_CONFIG 0x03
#define AXP2602_REG_VBAT_H 0x04
#define AXP2602_REG_VBAT_L 0x05
#define AXP2602_REG_TEMP 0x06
#define AXP2602_REG_SOH 0x07
#define AXP2602_REG_SOC 0x08
#define AXP2602_REG_TTE_H 0x0A
#define AXP2602_REG_TTE_L 0x0B
#define AXP2602_REG_TTF_H 0x0C
#define AXP2602_REG_TTF_L 0x0D
#define AXP2602_REG_LOW_SOC_TH 0x0E
#define AXP2602_REG_OT_TH 0x0F
#define AXP2602_REG_COMM_CONFIG 0x11
#define AXP2602_REG_IBAT_H 0x14
#define AXP2602_REG_IBAT_L 0x15
#define AXP2602_REG_RDC25_H 0x1A
#define AXP2602_REG_RDC25_L 0x1B
#define AXP2602_REG_QMAX_H 0x1E
#define AXP2602_REG_QMAX_L 0x1F
#define AXP2602_REG_IRQ_STATUS 0x20
#define AXP2602_REG_IRQ_ENABLE 0x21
#define AXP2602_REG_TDIE_H 0xC2
#define AXP2602_REG_TDIE_L 0xC3
#define AXP2602_REG_QMAX_RDC25_SEL 0xC5
#define AXP2602_REG_OP_MODE 0xC6

// 采样电阻选项
typedef enum
{
    RES_10mOhm = 0,
    RES_5mOhm = 1,
    RES_20mOhm = 2,
    RES_40mOhm = 3
} axp2602_resistor_t;

// 工作模式
typedef enum
{
    MODE_HIGH_ACCURACY = 0,
    MODE_NORMAL = 2,
    MODE_LOW_FREQ = 6
} axp2602_opmode_t;

class AXP2602
{
public:
    AXP2602(uint8_t addr = AXP2602_I2C_ADDR_DEFAULT, TwoWire &wire = Wire);
    bool begin();       // 初始化并唤醒芯片
    bool isConnected(); // 检测芯片是否响应

    // 基本数据读取
    float getBatteryVoltage(); // 电压 (V)
    float getBatteryCurrent(); // 电流 (A)，正为充电，负为放电
    int8_t getTemperature();   // 温度 (°C)
    uint8_t getSOC();          // 剩余电量百分比
    uint16_t getTimeToEmpty(); // 剩余使用时间 (分钟)
    uint16_t getTimeToFull();  // 充满时间 (分钟)
    uint8_t getSOH();          // 电池健康度 (0~100%)

    // 配置
    void setResistor(axp2602_resistor_t res);     // 设置采样电阻（需调用resetMCU()生效）
    void setOperatingMode(axp2602_opmode_t mode); // 设置工作模式
    void setLowSOCThreshold(uint8_t percent);     // 设置低电量报警阈值 (0~63%)
    void setOverTempThreshold(uint8_t temp);      // 设置过温报警阈值 (0~127°C)
    void enable_All_IRQ(bool enable);                  // 使能/禁止所有IRQ（简化，实际可单独控制）
    void clear_All_IRQ();                              // 清除中断标志

    // 高级控制
    void resetMCU();   // 复位MCU（软件复位）
    void enterSleep(); // 进入睡眠模式（停止计量）
    void wakeUp();     // 唤醒芯片

private:
    uint8_t _addr;
    TwoWire &_wire;

    // I2C 读写
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t *value);
    bool readRegister16(uint8_t reg, uint16_t *value, bool bigEndian = true);
    bool readRegisterS16(uint8_t reg, int16_t *value, bool bigEndian = true);
    bool readRegisterMulti(uint8_t reg, uint8_t *buffer, size_t len);

    // 内部辅助
    bool checkChipID();
    void waitForSOCReady(); // 唤醒后等待SOC稳定
};

#endif