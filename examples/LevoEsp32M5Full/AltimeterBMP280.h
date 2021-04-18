/*
 *
 *  Created: 21/02/2021
 *      Author: Bernd Wokoeck
 *
 * Altimeter function using a BMP280 module with Adafruit library
 * https://github.com/adafruit/Adafruit_BMP280_Library
 * 
 * For use with M5Core2: SDA and SCL to pin 32 and 33
 * (PA_SDA & PA_SCL --> access via class "Wire")
 *
 */

#ifndef ALTIMETER_BMP280_H
#define ALTIMETER_BMP280_H

#include <Adafruit_BMP280.h>
#include <Wire.h>

class AltimeterBMP280
{
public:
    AltimeterBMP280( TwoWire* tw  = &Wire );
    ~AltimeterBMP280();

    bool  Init(); // returns true, if sensor exists
    float GetAltitude( float sealevelhPa = 1013.25 );
    float GetTemp();

    static bool  CanCalculateSealevel() { return m_lastPressure != 0.0; }
    static float GetCurrentSealevelhPa( float calibrationAltitude );

protected:
    TwoWire * m_pWire;
    uint8_t   m_i2cAddress;

    Adafruit_BMP280* m_pBmp; // use I2C interface

    static float m_lastPressure;
    float readAltitude(float seaLevelhPa);
};

#endif // ALTIMETER_BMP280_H