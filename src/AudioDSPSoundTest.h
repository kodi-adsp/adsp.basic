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

#include "p8-platform/threads/threads.h"
#include "p8-platform/threads/mutex.h"

#define SOUND_TEST_OFF              0
#define SOUND_TEST_PINK_NOICE       1
#define SOUND_TEST_VOICE            2

#define CONTINUES_PINK_NOISE_TIME   2
#define CONTINUES_SOUND_TEST_TIME   2

class cPinkNoise;
class CGUIDialogSpeakerGain;

class cDSPProcessorSoundTest
{
public:
  cDSPProcessorSoundTest(unsigned long outChannelPresentFlags, CGUIDialogSpeakerGain *cbClass);
  ~cDSPProcessorSoundTest();

  void SetTestMode(int mode, AE_DSP_CHANNEL channel, bool continues);
  void SetTestVolume(float volume);
  unsigned int ProcessTestMode(float **array_in, float **array_out, unsigned int samples);

private:
  AE_DSP_CHANNEL GetNextChannelPtr(AE_DSP_CHANNEL previous);

  AE_DSP_CHANNEL    m_currentTestPointer;
  int               m_currentTestMode;
  bool              m_currentTestContinues;
  time_t            m_lastContinuesChange;
  unsigned long     m_OutChannelPresentFlags;
  cPinkNoise       *m_NoiseSource;
  CAddonSoundPlay  *m_TestSound;
  P8PLATFORM::CMutex  m_Mutex;
  CGUIDialogSpeakerGain *m_ContinueTestCBClass;
};
