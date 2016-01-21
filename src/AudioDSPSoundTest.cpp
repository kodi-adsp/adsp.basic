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

#include <string>
#include "addon.h"
#include "PinkNoise.h"
#include "GUIDialogSpeakerGain.h"
#include "AudioDSPSoundTest.h"

using namespace P8PLATFORM;
using namespace ADDON;

cDSPProcessorSoundTest::cDSPProcessorSoundTest(unsigned long outChannelPresentFlags, CGUIDialogSpeakerGain *cbClass)
{
  m_OutChannelPresentFlags  = outChannelPresentFlags;
  m_currentTestPointer      = AE_DSP_CH_INVALID;
  m_currentTestMode         = SOUND_TEST_OFF;
  m_currentTestContinues    = false;
  m_NoiseSource             = NULL;
  m_TestSound               = NULL;
  m_ContinueTestCBClass     = cbClass;
}

cDSPProcessorSoundTest::~cDSPProcessorSoundTest()
{
  if (m_TestSound)
    delete m_TestSound;
}

std::string GetSoundFile(AE_DSP_CHANNEL channel)
{
  std::string soundFile = g_strAddonPath;
  if (soundFile.at(soundFile.size() - 1) != '\\' &&
      soundFile.at(soundFile.size() - 1) != '/')
    soundFile += "/";
  soundFile += "resources/sounds/";

  std::string defaultSound = soundFile + "en/";
  char *language = KODI->GetDVDMenuLanguage();
  soundFile += language;
  soundFile += "/";
  KODI->FreeString(language);

  if (!KODI->DirectoryExists(soundFile.c_str()))
    soundFile = defaultSound;

  switch (channel)
  {
  case AE_DSP_CH_FL:
    soundFile.append("Front_Left.wav");
    break;
  case AE_DSP_CH_FR:
    soundFile.append("Front_Right.wav");
    break;
  case AE_DSP_CH_FC:
    soundFile.append("Front_Center.wav");
    break;
  case AE_DSP_CH_BL:
    soundFile.append("Rear_Left.wav");
    break;
  case AE_DSP_CH_BR:
    soundFile.append("Rear_Right.wav");
    break;
  case AE_DSP_CH_BC:
    soundFile.append("Rear_Center.wav");
    break;
  case AE_DSP_CH_SL:
    soundFile.append("Side_Left.wav");
    break;
  case AE_DSP_CH_SR:
    soundFile.append("Side_Right.wav");
    break;
  case AE_DSP_CH_LFE:
  case AE_DSP_CH_FLOC:
  case AE_DSP_CH_FROC:
  case AE_DSP_CH_TFL:
  case AE_DSP_CH_TFR:
  case AE_DSP_CH_TFC:
  case AE_DSP_CH_TC:
  case AE_DSP_CH_TBL:
  case AE_DSP_CH_TBR:
  case AE_DSP_CH_TBC:
  case AE_DSP_CH_BLOC:
  case AE_DSP_CH_BROC:
  default:
    soundFile.append("Noise.wav");
    break;
  };
  return soundFile;
}

void cDSPProcessorSoundTest::SetTestMode(int mode, AE_DSP_CHANNEL channel, bool continues)
{
  CLockObject lock(m_Mutex);

  /* Override mode if continues check is disabled */
  if (!continues && m_currentTestContinues)
    mode = SOUND_TEST_OFF;

  if (mode == SOUND_TEST_PINK_NOICE)
  {
    if (!m_NoiseSource)
      m_NoiseSource = new cPinkNoise;

    if (continues)
    {
      m_lastContinuesChange = time(NULL);
      channel = GetNextChannelPtr(AE_DSP_CH_LFE);
      if (m_ContinueTestCBClass)
        m_ContinueTestCBClass->ContinuesTestSwitchInfoCB(channel);
    }
  }
  else if (mode == SOUND_TEST_VOICE)
  {
    if (m_TestSound)
      delete m_TestSound;
    m_TestSound = ADSP->GetSoundPlay(GetSoundFile(channel).c_str());
    m_TestSound->SetChannel(channel);
    m_TestSound->SetVolume(g_DSPProcessor.m_OutputGain[channel]);
    m_TestSound->Play();

    if (continues)
    {
      m_lastContinuesChange = time(NULL);
      channel = GetNextChannelPtr(AE_DSP_CH_LFE);
      if (m_ContinueTestCBClass)
        m_ContinueTestCBClass->ContinuesTestSwitchInfoCB(channel);
    }
  }
  else
  {
    if (m_NoiseSource)
      delete m_NoiseSource;
    m_NoiseSource = NULL;
  }

  m_currentTestMode       = mode;
  m_currentTestPointer    = channel;
  m_currentTestContinues  = continues;
}

void cDSPProcessorSoundTest::SetTestVolume(float volume)
{
  if (m_TestSound)
    m_TestSound->SetVolume(g_DSPProcessor.m_OutputGain[m_currentTestPointer]);

}
unsigned int cDSPProcessorSoundTest::ProcessTestMode(float **array_in, float **array_out, unsigned int samples)
{
  CLockObject lock(m_Mutex);

  for (unsigned pos = 0; pos < samples; pos++)
  {
    if (m_currentTestMode != SOUND_TEST_OFF)
    {
      array_out[AE_DSP_CH_FL][pos]   = 0;
      array_out[AE_DSP_CH_FR][pos]   = 0;
      array_out[AE_DSP_CH_FC][pos]   = 0;
      array_out[AE_DSP_CH_LFE][pos]  = 0;
      array_out[AE_DSP_CH_BL][pos]   = 0;
      array_out[AE_DSP_CH_BR][pos]   = 0;
      array_out[AE_DSP_CH_FLOC][pos] = 0;
      array_out[AE_DSP_CH_FROC][pos] = 0;
      array_out[AE_DSP_CH_BC][pos]   = 0;
      array_out[AE_DSP_CH_SL][pos]   = 0;
      array_out[AE_DSP_CH_SR][pos]   = 0;
      array_out[AE_DSP_CH_TFL][pos]  = 0;
      array_out[AE_DSP_CH_TFR][pos]  = 0;
      array_out[AE_DSP_CH_TFC][pos]  = 0;
      array_out[AE_DSP_CH_TC][pos]   = 0;
      array_out[AE_DSP_CH_TBL][pos]  = 0;
      array_out[AE_DSP_CH_TBR][pos]  = 0;
      array_out[AE_DSP_CH_TBC][pos]  = 0;
      array_out[AE_DSP_CH_BLOC][pos] = 0;
      array_out[AE_DSP_CH_BROC][pos] = 0;

      if (m_currentTestPointer != AE_DSP_CH_INVALID)
      {
        if (m_currentTestMode == SOUND_TEST_PINK_NOICE)
        {
          if (m_currentTestContinues)
          {
            time_t Now = time(NULL);
            if (Now - m_lastContinuesChange > CONTINUES_PINK_NOISE_TIME)
            {
              m_currentTestPointer = GetNextChannelPtr(m_currentTestPointer);
              if (m_ContinueTestCBClass)
                m_ContinueTestCBClass->ContinuesTestSwitchInfoCB(m_currentTestPointer);
              m_lastContinuesChange = Now;
            }
          }
          array_out[m_currentTestPointer][pos] = m_NoiseSource->getValue();
        }
        else if (m_currentTestMode == SOUND_TEST_VOICE)
        {
          if (m_currentTestContinues)
          {
            time_t Now = time(NULL);
            if (Now - m_lastContinuesChange > CONTINUES_SOUND_TEST_TIME)
            {
              if (m_TestSound)
                delete m_TestSound;

              m_currentTestPointer = GetNextChannelPtr(m_currentTestPointer);
              if (m_ContinueTestCBClass)
                m_ContinueTestCBClass->ContinuesTestSwitchInfoCB(m_currentTestPointer);
              m_TestSound = ADSP->GetSoundPlay(GetSoundFile(m_currentTestPointer).c_str());
              m_TestSound->SetChannel(m_currentTestPointer);
              m_TestSound->SetVolume(g_DSPProcessor.m_OutputGain[m_currentTestPointer]);
              m_TestSound->Play();
              m_lastContinuesChange = Now;
            }
          }
        }
      }
    }
    else
    {
      array_out[AE_DSP_CH_FL][pos]   = array_in[AE_DSP_CH_FL][pos];
      array_out[AE_DSP_CH_FR][pos]   = array_in[AE_DSP_CH_FR][pos];
      array_out[AE_DSP_CH_FC][pos]   = array_in[AE_DSP_CH_FC][pos];
      array_out[AE_DSP_CH_LFE][pos]  = array_in[AE_DSP_CH_LFE][pos];
      array_out[AE_DSP_CH_BL][pos]   = array_in[AE_DSP_CH_BL][pos];
      array_out[AE_DSP_CH_BR][pos]   = array_in[AE_DSP_CH_BR][pos];
      array_out[AE_DSP_CH_FLOC][pos] = array_in[AE_DSP_CH_FLOC][pos];
      array_out[AE_DSP_CH_FROC][pos] = array_in[AE_DSP_CH_FROC][pos];
      array_out[AE_DSP_CH_BC][pos]   = array_in[AE_DSP_CH_BC][pos];
      array_out[AE_DSP_CH_SL][pos]   = array_in[AE_DSP_CH_SL][pos];
      array_out[AE_DSP_CH_SR][pos]   = array_in[AE_DSP_CH_SR][pos];
      array_out[AE_DSP_CH_TFL][pos]  = array_in[AE_DSP_CH_TFL][pos];
      array_out[AE_DSP_CH_TFR][pos]  = array_in[AE_DSP_CH_TFR][pos];
      array_out[AE_DSP_CH_TFC][pos]  = array_in[AE_DSP_CH_TFC][pos];
      array_out[AE_DSP_CH_TC][pos]   = array_in[AE_DSP_CH_TC][pos];
      array_out[AE_DSP_CH_TBL][pos]  = array_in[AE_DSP_CH_TBL][pos];
      array_out[AE_DSP_CH_TBR][pos]  = array_in[AE_DSP_CH_TBR][pos];
      array_out[AE_DSP_CH_TBC][pos]  = array_in[AE_DSP_CH_TBC][pos];
      array_out[AE_DSP_CH_BLOC][pos] = array_in[AE_DSP_CH_BLOC][pos];
      array_out[AE_DSP_CH_BROC][pos] = array_in[AE_DSP_CH_BROC][pos];
    }
  }
  return samples;
}

AE_DSP_CHANNEL cDSPProcessorSoundTest::GetNextChannelPtr(AE_DSP_CHANNEL previous)
{
  AE_DSP_CHANNEL next = AE_DSP_CH_FL;
  switch (previous)
  {
    case AE_DSP_CH_FL:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_TFL)   { next = AE_DSP_CH_TFL;  break; }
    case AE_DSP_CH_TFL:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_FLOC)  { next = AE_DSP_CH_FLOC; break; }
    case AE_DSP_CH_FLOC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_FC)    { next = AE_DSP_CH_FC;   break; }
    case AE_DSP_CH_FC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_TFC)   { next = AE_DSP_CH_TFC;  break; }
    case AE_DSP_CH_TFC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_FROC)  { next = AE_DSP_CH_FROC; break; }
    case AE_DSP_CH_FROC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_TFR)   { next = AE_DSP_CH_TFR;  break; }
    case AE_DSP_CH_TFR:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_FR)    { next = AE_DSP_CH_FR;   break; }
    case AE_DSP_CH_FR:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_SR)    { next = AE_DSP_CH_SR;   break; }
    case AE_DSP_CH_SR:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_BR)    { next = AE_DSP_CH_BR;   break; }
    case AE_DSP_CH_BR:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_TBR)   { next = AE_DSP_CH_TBR;  break; }
    case AE_DSP_CH_TBR:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_BROC)  { next = AE_DSP_CH_BROC; break; }
    case AE_DSP_CH_BROC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_BC)    { next = AE_DSP_CH_BC;   break; }
    case AE_DSP_CH_BC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_TBC)   { next = AE_DSP_CH_TBC;  break; }
    case AE_DSP_CH_TBC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_BLOC)  { next = AE_DSP_CH_BLOC; break; }
    case AE_DSP_CH_BLOC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_TBL)   { next = AE_DSP_CH_TBL;  break; }
    case AE_DSP_CH_TBL:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_BL)    { next = AE_DSP_CH_BL;   break; }
    case AE_DSP_CH_BL:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_SL)    { next = AE_DSP_CH_SL;   break; }
    case AE_DSP_CH_SL:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_TC)    { next = AE_DSP_CH_TC;   break; }
    case AE_DSP_CH_TC:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_LFE)   { next = AE_DSP_CH_LFE;  break; }
    case AE_DSP_CH_LFE:
      if (m_OutChannelPresentFlags & AE_DSP_PRSNT_CH_FL)    { next = AE_DSP_CH_FL;   break; }
    default:
      break;
  }
  return next;
}
