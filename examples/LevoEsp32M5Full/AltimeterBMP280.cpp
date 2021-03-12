/*
 *
 *  Created: 21/02/2021
 *      Author: Bernd Woköck
 *
 * Altimeter function using a BMP280 module with Adafruit library
 * Derived from Adafruit example
 *
 */

#include "AltimeterBmp280.h"

float AltimeterBMP280::m_lastPressure = 0.0; // last pressure for sealevel calibration

AltimeterBMP280::AltimeterBMP280( TwoWire* tw ) : m_pWire(tw)
{
    m_pBmp = new Adafruit_BMP280(m_pWire);
}

AltimeterBMP280::~AltimeterBMP280()
{
    delete m_pBmp;
}

bool  AltimeterBMP280::Init()
{
    uint8_t i2cAddress = 0x77;

    if (!m_pBmp->begin(i2cAddress))
    {
        i2cAddress = 0x76;
        if (!m_pBmp->begin(i2cAddress))
        {
            // Serial.println( "Could not find a valid BMP280 sensor, check wiring!" );
            return false;
        }
    }

    /* Default settings from datasheet. */
    m_pBmp->setSampling( Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                         Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                         Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                         Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                         Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    return true;
}

float AltimeterBMP280::GetAltitude(float sealevelPressure)
{
    float alt = readAltitude(sealevelPressure);
    alt = round(alt * 10.0) / 10.0;
    return alt;
}

// copied from Adafruit_BMP280 and made static
float AltimeterBMP280::GetCurrentSealevelhPa(float calibrationAltitude)
{
    if( m_lastPressure == 0.0 || calibrationAltitude == 0.0 )
        return 0.0;

    return m_lastPressure / pow(1.0 - (calibrationAltitude / 44330.0), 5.255);
}

// copied from Adafruit_BMP280 to get access to pressure and altitude in one read cycle
float AltimeterBMP280::readAltitude(float seaLevelhPa)
{
    float altitude;

    m_lastPressure = m_pBmp->readPressure(); // in Si units for Pascal
    m_lastPressure /= 100.0;                 // hPa
    altitude = 44330 * (1.0 - pow(m_lastPressure / seaLevelhPa, 0.1903));
    // Serial.printf( "Pressure: %f, altitude: %f\r\n", m_lastPressure, altitude );

    // on error 
    if( altitude < -100.0 )
    {
        Serial.printf( "BMP280 sensor error - reset\r\n");
        m_pBmp->reset();
    }

    return altitude;
}

