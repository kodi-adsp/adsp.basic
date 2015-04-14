#pragma once
/*
 *      Copyright (C) 2014-2015 Team KODI
 *      Copyright (C) 2002 Nathaniel Virgo
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
 *  pink noise generating class using the Voss-McCartney algorithm, as
 *  described at www.firstpr.com.au/dsp/pink-noise/
 */

#include <stdlib.h>

typedef unsigned int CounterType;
typedef float DataValue;

const int n_generators = 8 * sizeof(CounterType);

class cPinkNoise
{
private:
  CounterType m_counter;
  DataValue *m_generators;
  DataValue m_lastValue;

public:
  cPinkNoise()
  {
    m_generators = new DataValue[n_generators];
    reset();
  }

  ~cPinkNoise()
  {
    delete [] m_generators;
  };

  void reset()
  {
    m_counter = 0;
    m_lastValue = 0;
    for (int i = 0; i < n_generators; ++i)
    {
      m_generators[i] = 2 * (rand() / DataValue(RAND_MAX)) - 1;
      m_lastValue += m_generators[i];
    }
  }

  inline DataValue getUnscaledValue()
  {
    if (m_counter != 0)
    {
      // set index to number of trailing zeros in m_counter.
      // hangs if m_counter==0, hence the slightly inefficient
      // test above.
      CounterType n = m_counter;
      int index = 0;
      while ( (n & 1) == 0 )
      {
        n >>= 1;
        index++;
        // this loop means that the plugins cannot be labelled as
        // capable of hard real-time performance.
      }

      m_lastValue -= m_generators[index];
      m_generators[index] = 2 * (rand() / DataValue(RAND_MAX)) - 1;
      m_lastValue += m_generators[index];
    }

    m_counter++;

    return m_lastValue;
  }

  inline DataValue getValue()
  {
    return getUnscaledValue() / n_generators;
  }

  inline DataValue getLastValue()
  {
    return m_lastValue / n_generators;
  }

  inline DataValue getValue2()
  {
    // adding some white noise gets rid of some nulls in the frequency spectrum
    // but makes the signal spikier, so possibly not so good for control signals.
    return (getUnscaledValue() + rand() / DataValue(RAND_MAX * 0.5) - 1) / (n_generators + 1);
  }
};
