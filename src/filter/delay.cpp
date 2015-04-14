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

#include "../AudioDSPBasic.h"
#include "delay.h"

#define CURRENT_DEPTH ((m_in >= m_out) ? m_in-m_out : m_Size - (m_out-m_in))
#define BUFFER_INCREASED_TIME 1000

CDelay::CDelay(void)
{
  m_Buffer      = NULL;
  m_BufferSize  = 0;
}

CDelay::~CDelay(void)
{
  if (m_Buffer != NULL)
  {
    delete m_Buffer;
  }
}

void CDelay::Init(unsigned int delay, unsigned sampling_rate)
{
  m_Delay         = delay;
  m_SamplingRate  = sampling_rate;
  m_Size          = (unsigned int)(double(m_Delay)/DELAY_RESOLUTION*m_SamplingRate);

  if (m_Size > m_BufferSize)
  {
    m_BufferSize = (unsigned int)(double(m_Delay+BUFFER_INCREASED_TIME)/DELAY_RESOLUTION*m_SamplingRate);
    if (m_Buffer != NULL)
      delete m_Buffer;

    m_Buffer = new double[m_BufferSize+1];
  }

  m_in  = m_Buffer;
  m_out = m_Buffer;

  m_DataReady = false;
}

void CDelay::SetSamplingRate(unsigned int sampling_rate)
{
  if (sampling_rate != m_SamplingRate)
  {
    Init(m_Delay, sampling_rate);
  }
}

void CDelay::SetDelay(unsigned int delay)
{
  if (delay != m_Delay)
  {
    Init(delay, m_SamplingRate);
  }
}

unsigned int CDelay::GetDelay(void)
{
  return m_Delay;
}

unsigned int CDelay::GetLatency(void)
{
  return m_Size;
}

unsigned int CDelay::GetSamplingRate(void)
{
  return m_SamplingRate;
}

void CDelay::Store(double input)
{
  if (m_Buffer != NULL)
  {
    *m_in++ = input;

     // wrap input buffer pointer
    if (m_in >= m_Buffer + m_Size)
    {
      m_in = m_Buffer;
      m_DataReady = true;
    }
  }
}

double CDelay::Retrieve(void)
{
  double data_out;

  if ((m_Buffer != NULL) && m_DataReady)
  {
    data_out = *m_out++;

    // wrap output buffer pointer
    if (m_out >= (m_Buffer+m_Size))
    {
      m_out = m_Buffer;
    }
  }
  else
  {
    data_out = 0.0;
  }
  return data_out;
}

void CDelay::Flush(void)
{
  m_in        = m_Buffer;
  m_out       = m_Buffer;
  m_DataReady = false;
}
