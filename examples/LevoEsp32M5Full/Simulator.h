/*
 *
 *  Created: 13/03/2021
 *      Author: Bernd Wokoeck
 *
 * Replay a log file to simulate a trip
 *
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "DisplayData.h"

class Simulator
{
public:
    bool Update(DisplayData::enIds & id, float & fVal, uint32_t timestamp);

protected:
    File m_simFile;

    static const int s_valColArray[];

    bool readLine(uint32_t & time, int & id, float & fVal );

    uint32_t m_startTime = 0L;
    uint32_t m_nextValueTime = 0L;
    uint32_t m_startTimeOffset = 0L;
    int      m_nextValueId;
    float    m_nextValue;
};

#endif // SIMULATOR_H