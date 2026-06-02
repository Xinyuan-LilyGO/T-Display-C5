/*
   MIT License

  Copyright (c) 2021 Felix Biego

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <CST816S.h>
#include "Arduino.h"
#include "board_config.h"
#include "TCA6408.h"

TCA6408 ioExpander;

CST816S touch(IIC_SDA_PIN, IIC_SCL_PIN, -1, TP_INT); // sda, scl, rst, irq

volatile bool touchEventFlag = false;

// Interrupt function to set the flag
void IRAM_ATTR onTouchInterrupt()
{
    touchEventFlag = true;
}

void setup()
{
    Serial.begin(115200);
    Wire.begin(IIC_SDA_PIN, IIC_SCL_PIN);
    ioExpander.begin(&Wire);
    ioExpander.pinMode(IO_EXPANDER_3, OUTPUT);
    ioExpander.digitalWrite(IO_EXPANDER_3, LOW);
    delay(50);
    ioExpander.digitalWrite(IO_EXPANDER_3, HIGH);
    delay(100);

    touch.begin(RISING);
    touch.attachUserInterrupt(onTouchInterrupt);
    // touch.enable_double_click(); // Enable double-click detection

    Serial.print(touch.data.version);
    Serial.print("\t");
    Serial.print(touch.data.versionInfo[0]);
    Serial.print("-");
    Serial.print(touch.data.versionInfo[1]);
    Serial.print("-");
    Serial.println(touch.data.versionInfo[2]);
}

void loop()
{
    if (touchEventFlag)
    {
        touchEventFlag = false; // Clear the flag
        if (touch.available())
        {
            Serial.print(touch.gesture());
            Serial.print("     \t");
            Serial.print(touch.data.points);
            Serial.print("\t");
            Serial.print(touch.data.event);
            Serial.print("\t");
            Serial.print(touch.data.x);
            Serial.print("\t");
            Serial.println(touch.data.y);
        }
    }
}