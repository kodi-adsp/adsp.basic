#pragma once
/*
 *      Copyright (C) 2014-2015 Team KODI
 *      http://kodi.tv
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */
/*
 * This code is taken from foo_dsp_xover and a bit modified
 * http://sourceforge.net/projects/xover/
 */

const int DELAY_RESOLUTION  = 1000000;
const int MAX_DELAY_SEC     = 1;
const int MAX_DELAY         = MAX_DELAY_SEC * DELAY_RESOLUTION;

#define MAX_SPEAKER_DISTANCE_METER      60

#define ROUND(VAL)  (unsigned int)((VAL)+0.5)

/// Speed of sound = 331.451 m/s + 0.6 m/s/C * T
#define SPEED_OF_SOUND              331.451
#define METER_TO_INCHES             39.370078
#define METER_TO_FEETS              3.2808399

#define SEC_TO_DELAY(VAL)       ROUND(double(VAL)*DELAY_RESOLUTION)
#define mSEC_TO_DELAY(VAL)      ROUND(double(VAL)/1000*DELAY_RESOLUTION)
#define M_TO_DELAY(VAL)         ROUND(double(VAL)/SPEED_OF_SOUND*DELAY_RESOLUTION)
#define mM_TO_DELAY(VAL)        ROUND(double(VAL)/1000/SPEED_OF_SOUND*DELAY_RESOLUTION)
#define IN_TO_DELAY(VAL)        ROUND(double(VAL)/SPEED_OF_SOUND/METER_TO_INCHES*DELAY_RESOLUTION)
#define FT_TO_DELAY(VAL)        ROUND(double(VAL)/SPEED_OF_SOUND/METER_TO_FEETS*DELAY_RESOLUTION)

#define DELAY_TO_SEC(VAL)       ROUND(double(VAL)*1000/DELAY_RESOLUTION)/1000
#define DELAY_TO_SEC_FRAC(VAL)  ROUND(double(VAL)*1000/DELAY_RESOLUTION)%1000
#define DELAY_TO_mSEC(VAL)      ROUND(double(VAL)*1000/DELAY_RESOLUTION)
#define DELAY_TO_M(VAL)         ROUND(double(VAL)*SPEED_OF_SOUND*1000/DELAY_RESOLUTION)/1000
#define DELAY_TO_M_FRAC(VAL)    ROUND(double(VAL)*SPEED_OF_SOUND*1000/DELAY_RESOLUTION)%1000
#define DELAY_TO_mM(VAL)        ROUND(double(VAL)*SPEED_OF_SOUND*1000/DELAY_RESOLUTION)
#define DELAY_TO_IN(VAL)        ROUND(double(VAL)*SPEED_OF_SOUND*METER_TO_INCHES*100/DELAY_RESOLUTION)/100
#define DELAY_TO_IN_FRAC(VAL)   ROUND(double(VAL)*SPEED_OF_SOUND*METER_TO_INCHES*100/DELAY_RESOLUTION)%100
#define DELAY_TO_FT(VAL)        ROUND(double(VAL)*SPEED_OF_SOUND*METER_TO_FEETS*100/DELAY_RESOLUTION)

class CDelay
{
public:
  CDelay(void);
  ~CDelay(void);

  void Init(unsigned int delay, unsigned sampling_rate);

  void Store(double input);
  double Retrieve(void);
  void Flush(void);

  void SetSamplingRate(unsigned int sampling_rate); //!< in Hz
  void SetDelay(unsigned int delay);                //!< defined in DELAY_RESOLUTION ( currently in uS)

  unsigned int GetSamplingRate(void);               //!< Return Hz
  unsigned int GetDelay(void);                      //!< Return in DELAY_RESOLUTION ( currently in uS)
  unsigned int GetLatency(void);                    //!< Return number of samples

private:
  double       *m_Buffer;
  double       *m_in;
  double       *m_out;

  unsigned int  m_Size;
  unsigned int  m_BufferSize;
  unsigned int  m_SamplingRate;

  unsigned int  m_Delay;

  bool          m_DataReady;
};
