#include "AXP2602.h"

AXP2602::AXP2602(uint8_t addr, TwoWire &wire)
    : _addr(addr), _wire(wire)
{
}

bool AXP2602::begin()
{
    _wire.begin();
    if (!isConnected()) {
        return false;
    }
    if (!checkChipID()) {
        return false;
    }
    wakeUp();
    waitForSOCReady();
    return true;
}

bool AXP2602::isConnected()
{
    _wire.beginTransmission(_addr);
    return _wire.endTransmission() == 0;
}

bool AXP2602::writeRegister(uint8_t reg, uint8_t value)
{
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    _wire.write(value);
    return _wire.endTransmission() == 0;
}

bool AXP2602::readRegister(uint8_t reg, uint8_t *value)
{
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    if (_wire.endTransmission(false) != 0) return false;
    if (_wire.requestFrom(_addr, (uint8_t)1) != 1) return false;
    *value = _wire.read();
    return true;
}

bool AXP2602::readRegister16(uint8_t reg, uint16_t *value, bool bigEndian)
{
    uint8_t buf[2];
    if (!readRegisterMulti(reg, buf, 2)) return false;
    if (bigEndian) {
        *value = (buf[0] << 8) | buf[1];
    } else {
        *value = (buf[1] << 8) | buf[0];
    }
    return true;
}

bool AXP2602::readRegisterS16(uint8_t reg, int16_t *value, bool bigEndian)
{
    uint16_t uval;
    if (!readRegister16(reg, &uval, bigEndian)) return false;
    *value = (int16_t)uval;
    return true;
}

bool AXP2602::readRegisterMulti(uint8_t reg, uint8_t *buffer, size_t len)
{
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    if (_wire.endTransmission(false) != 0) return false;
    if (_wire.requestFrom(_addr, (uint8_t)len) != len) return false;
    for (size_t i = 0; i < len; i++) {
        buffer[i] = _wire.read();
    }
    return true;
}

bool AXP2602::checkChipID()
{
    uint8_t id;
    if (!readRegister(AXP2602_REG_ID, &id)) return false;
    // 根据数据手册，ID 应为 0x1C
    return (id == 0x1C);
}

void AXP2602::waitForSOCReady()
{
    // 唤醒后约350ms SOC 才稳定
    delay(350);
}

void AXP2602::wakeUp()
{
    uint8_t mode;
    if (!readRegister(AXP2602_REG_MODE, &mode)) return;
    mode &= ~0x01;  // 清除 sleep 位
    writeRegister(AXP2602_REG_MODE, mode);
}

void AXP2602::enterSleep()
{
    uint8_t mode;
    if (!readRegister(AXP2602_REG_MODE, &mode)) return;
    mode |= 0x01;   // 设置 sleep 位
    writeRegister(AXP2602_REG_MODE, mode);
}

void AXP2602::resetMCU()
{
    uint8_t mode;
    if (!readRegister(AXP2602_REG_MODE, &mode)) return;
    mode |= (1 << 5);   // reset_mcu 位
    writeRegister(AXP2602_REG_MODE, mode);
    delay(10);          // 等待复位完成
}

float AXP2602::getBatteryVoltage()
{
    uint16_t raw;
    if (!readRegister16(AXP2602_REG_VBAT_H, &raw, true)) return 0.0f;
    // 高6位在 REG04 的低6位，REG05 为低8位，组合后低14位有效
    raw = raw & 0x3FFF;  // 屏蔽高2位（实际只有14位）
    // 电压 = ADC值 * (4500mV / 0x1194) = ADC值 * 1mV
    // 0x1194 = 4500，因此直接返回 mV 转换为 V
    return raw / 1000.0f;
}

float AXP2602::getBatteryCurrent()
{
    int16_t raw;
    if (!readRegisterS16(AXP2602_REG_IBAT_H, &raw, true)) return 0.0f;
    // 电流 ADC 值每 LSB = 0.0025mV，再除以采样电阻得到电流
    // 获取当前采样电阻配置
    uint8_t conf;
    if (!readRegister(AXP2602_REG_COMM_CONFIG, &conf)) return 0.0f;
    uint8_t res_sel = conf & 0x03;
    float res_mohm = 10.0f;  // 默认 10mΩ
    switch (res_sel) {
        case RES_5mOhm:  res_mohm = 5.0f;  break;
        case RES_10mOhm: res_mohm = 10.0f; break;
        case RES_20mOhm: res_mohm = 20.0f; break;
        case RES_40mOhm: res_mohm = 40.0f; break;
    }
    // 电流 (mA) = raw * 0.0025mV / res_mohm * 1000 = raw * (2.5 / res_mohm)
    // 单位转换 mA -> A
    float current_A = raw * (2.5f / res_mohm) / 1000.0f;
    return current_A;
}

int8_t AXP2602::getTemperature()
{
    uint8_t temp;
    if (!readRegister(AXP2602_REG_TEMP, &temp)) return 0;
    // 数据手册：寄存器值为有符号8位，1LSB=1°C
    return (int8_t)temp;
}

uint8_t AXP2602::getSOC()
{
    uint8_t soc;
    if (!readRegister(AXP2602_REG_SOC, &soc)) return 0;
    return soc;
}

uint16_t AXP2602::getTimeToEmpty()
{
    uint16_t tte;
    if (!readRegister16(AXP2602_REG_TTE_H, &tte, true)) return 0;
    return tte;
}

uint16_t AXP2602::getTimeToFull()
{
    uint16_t ttf;
    if (!readRegister16(AXP2602_REG_TTF_H, &ttf, true)) return 0;
    return ttf;
}

uint8_t AXP2602::getSOH()
{
    uint8_t soh;
    if (!readRegister(AXP2602_REG_SOH, &soh)) return 0;
    return soh;
}

void AXP2602::setResistor(axp2602_resistor_t res)
{
    uint8_t conf;
    if (!readRegister(AXP2602_REG_COMM_CONFIG, &conf)) return;
    conf &= ~0x03;
    conf |= (res & 0x03);
    writeRegister(AXP2602_REG_COMM_CONFIG, conf);
    // 需要复位 MCU 使新配置生效
    resetMCU();
    // 复位后可能需要重新唤醒
    wakeUp();
    waitForSOCReady();
}

void AXP2602::setOperatingMode(axp2602_opmode_t mode)
{
    // 模式寄存器地址 0xC6，低3位有效
    uint8_t reg;
    if (!readRegister(AXP2602_REG_OP_MODE, &reg)) return;
    reg &= ~0x07;
    reg |= (mode & 0x07);
    writeRegister(AXP2602_REG_OP_MODE, reg);
}

void AXP2602::setLowSOCThreshold(uint8_t percent)
{
    if (percent > 63) percent = 63;
    uint8_t reg;
    if (!readRegister(AXP2602_REG_LOW_SOC_TH, &reg)) return;
    reg &= ~0x3F;
    reg |= percent;
    writeRegister(AXP2602_REG_LOW_SOC_TH, reg);
}

void AXP2602::setOverTempThreshold(uint8_t temp)
{
    if (temp > 127) temp = 127;
    uint8_t reg;
    if (!readRegister(AXP2602_REG_OT_TH, &reg)) return;
    reg &= ~0x7F;
    reg |= temp;
    writeRegister(AXP2602_REG_OT_TH, reg);
}

void AXP2602::enable_All_IRQ(bool enable)
{
    uint8_t en = enable ? 0x0F : 0x00;  // 简单使能所有IRQ
    // 实际可以单独控制，这里简化使能/禁止所有中断源
    writeRegister(AXP2602_REG_IRQ_ENABLE, en);
}

void AXP2602::clear_All_IRQ()
{
    // 向 IRQ_STATUS 寄存器写0再写1清除（先写0再写1）
    writeRegister(AXP2602_REG_IRQ_STATUS, 0x00);
    writeRegister(AXP2602_REG_IRQ_STATUS, 0xFF);
}