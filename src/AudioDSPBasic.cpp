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

#include <math.h>
#include <vector>

#include "libXBMC_addon.h"
#include "libKODI_adsp.h"
#include "libKODI_guilib.h"

#include "addon.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/util.h"
#include "p8-platform/util/StdString.h"

#include "AudioDSPBasic.h"
#include "AudioDSPSoundTest.h"

#include "GUIDialogSpeakerGain.h"
#include "GUIDialogSpeakerDistance.h"

using namespace std;
using namespace ADDON;

std::string GetSettingsFile()
{
  string settingFile = g_strUserPath;
  if (settingFile.at(settingFile.size() - 1) == '\\' ||
      settingFile.at(settingFile.size() - 1) == '/')
    settingFile.append("ADSPBasicAddonSettings.xml");
  else
#if defined(TARGET_WINDOWS)
    settingFile.append("\\ADSPBasicAddonSettings.xml");
#else
    settingFile.append("/ADSPBasicAddonSettings.xml");
#endif
  return settingFile;
}

static inline const float GainToScale(const float dB)
{
  return pow(10.0f, dB / 20);
}


/*!
 * Pointer array for active dsp processing classes, for this reason the
 * stream id is never more as AE_DSP_STREAM_MAX_STREAMS and can be used as pointer to this array.
 */
cDSPProcessorStream *g_usedDSPs[AE_DSP_STREAM_MAX_STREAMS];

/*!
 * Start of the processor class
 */

cDSPProcessorStream::cDSPProcessorStream(AE_DSP_STREAM_ID id)
  : m_StreamID(id)
  , m_SoundTest(NULL)
  , m_MasterCurrrentMode(NULL)
{
  memset(m_Delay, 0, sizeof(m_Delay));
}

cDSPProcessorStream::~cDSPProcessorStream()
{
  StreamDestroy();

  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
  {
    if (m_Delay[i] != NULL)
      delete m_Delay[i];
  }
}


/*!
 * Control function for start and stop of dsp processing.
 */

AE_DSP_ERROR cDSPProcessorStream::StreamCreate(const AE_DSP_SETTINGS *settings, const AE_DSP_STREAM_PROPERTIES *pProperties)
{
  int presentFlag;

  AE_DSP_ERROR err = AE_DSP_ERROR_NO_ERROR;

  m_iStreamType     = pProperties->iStreamType;
  m_iBaseType       = pProperties->iBaseType;
  m_strName         = pProperties->strName;
  m_strCodecId      = pProperties->strCodecId;
  m_strLanguage     = pProperties->strLanguage;
  m_iIdentifier     = pProperties->iIdentifier;
  m_iChannels       = pProperties->iChannels;
  m_iSampleRate     = pProperties->iSampleRate;

  m_Settings.iStreamID              = settings->iStreamID;
  m_Settings.iStreamType            = settings->iStreamType;
  m_Settings.iInChannels            = settings->iInChannels;
  m_Settings.lInChannelPresentFlags = settings->lInChannelPresentFlags;
  m_Settings.iOutChannels           = settings->iOutChannels;
  m_Settings.lOutChannelPresentFlags= settings->lOutChannelPresentFlags;
  m_Settings.iInFrames              = settings->iInFrames;
  m_Settings.iInSamplerate          = settings->iInSamplerate;
  m_Settings.iProcessFrames         = settings->iProcessFrames;
  m_Settings.iProcessSamplerate     = settings->iProcessSamplerate;
  m_Settings.iOutFrames             = settings->iOutFrames;
  m_Settings.iOutSamplerate         = settings->iOutSamplerate;
  m_Settings.bStereoUpmix           = settings->bStereoUpmix;

  g_DSPProcessor.SetOutChannelPresentFlags(settings->lOutChannelPresentFlags);

  for (masterModesMap::iterator it = g_DSPProcessor.m_MasterModesMap.begin(); it != g_DSPProcessor.m_MasterModesMap.end(); it++)
  {
    if (it->second->IsSupported(settings, pProperties))
    {
      CDSPProcessMaster *mode;
      if (m_Settings.iStreamID == 0)
        mode = it->second;
      else
      {
        mode = CDSPProcessMaster::AllocateMaster(m_Settings.iStreamID, it->second->GetId());
        mode->m_ModeInfoStruct.iUniqueDBModeId = it->second->m_ModeInfoStruct.iUniqueDBModeId; //!< Is set on first load registration of mode and is not set on load further loads
      }
      m_MasterModes.push_back(mode);
    }
  }

  m_ProcessSamplerate = settings->iOutSamplerate;
  m_ProcessSamplesize = 8192;

  return err;
}

AE_DSP_ERROR cDSPProcessorStream::StreamDestroy()
{
  if (m_MasterCurrrentMode)
    m_MasterCurrrentMode->Deinitialize();
  m_MasterCurrrentMode = NULL;
  for (unsigned int i = 0; i < m_MasterModes.size(); ++i)
  {
    if (m_MasterModes[i]->GetStreamId() > 0)
      delete m_MasterModes[i];
  }
  m_MasterModes.clear();

  if (m_SoundTest)
    delete m_SoundTest;

  return AE_DSP_ERROR_NO_ERROR;
}

AE_DSP_ERROR cDSPProcessorStream::StreamIsModeSupported(AE_DSP_MODE_TYPE mode_type, unsigned int mode_id, int unique_db_mode_id)
{
  (void) unique_db_mode_id;

  for (unsigned int i = 0; i < m_MasterModes.size(); ++i)
  {
    if (m_MasterModes[i]->m_ModeInfoStruct.iModeType == mode_type &&
        m_MasterModes[i]->m_ModeInfoStruct.iModeNumber == mode_id)
    return AE_DSP_ERROR_NO_ERROR;
  }

  if (mode_type == AE_DSP_MODE_TYPE_POST_PROCESS)
  {
    if (mode_id == ID_POST_PROCESS_SPEAKER_CORRECTION)
      return AE_DSP_ERROR_NO_ERROR;
  }

  return AE_DSP_ERROR_IGNORE_ME;
}

AE_DSP_ERROR cDSPProcessorStream::StreamInitialize(const AE_DSP_SETTINGS *settings)
{
  AE_DSP_ERROR err = AE_DSP_ERROR_NO_ERROR;

  m_Settings.iStreamID              = settings->iStreamID;
  m_Settings.iStreamType            = settings->iStreamType;
  m_Settings.iInChannels            = settings->iInChannels;
  m_Settings.lInChannelPresentFlags = settings->lInChannelPresentFlags;
  m_Settings.iOutChannels           = settings->iOutChannels;
  m_Settings.lOutChannelPresentFlags= settings->lOutChannelPresentFlags;
  m_Settings.iInFrames              = settings->iInFrames;
  m_Settings.iInSamplerate          = settings->iInSamplerate;
  m_Settings.iProcessFrames         = settings->iProcessFrames;
  m_Settings.iProcessSamplerate     = settings->iProcessSamplerate;
  m_Settings.iOutFrames             = settings->iOutFrames;
  m_Settings.iOutSamplerate         = settings->iOutSamplerate;
  m_Settings.bStereoUpmix           = settings->bStereoUpmix;

  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FL)
  {
    UpdateDelay(AE_DSP_CH_FL);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FR)
  {
    UpdateDelay(AE_DSP_CH_FR);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FC)
  {
    UpdateDelay(AE_DSP_CH_FC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_LFE)
  {
    UpdateDelay(AE_DSP_CH_LFE);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BL)
  {
    UpdateDelay(AE_DSP_CH_BL);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BR)
  {
    UpdateDelay(AE_DSP_CH_BR);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FLOC)
  {
    UpdateDelay(AE_DSP_CH_FLOC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FROC)
  {
    UpdateDelay(AE_DSP_CH_FROC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BC)
  {
    UpdateDelay(AE_DSP_CH_BC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_SL)
  {
    UpdateDelay(AE_DSP_CH_SL);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_SR)
  {
    UpdateDelay(AE_DSP_CH_SR);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFL)
  {
    UpdateDelay(AE_DSP_CH_TFL);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFR)
  {
    UpdateDelay(AE_DSP_CH_TFR);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFC)
  {
    UpdateDelay(AE_DSP_CH_TFC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TC)
  {
    UpdateDelay(AE_DSP_CH_TC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBL)
  {
    UpdateDelay(AE_DSP_CH_TBL);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBR)
  {
    UpdateDelay(AE_DSP_CH_TBR);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBC)
  {
    UpdateDelay(AE_DSP_CH_TBC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BLOC)
  {
    UpdateDelay(AE_DSP_CH_BLOC);
  }
  if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BROC)
  {
    UpdateDelay(AE_DSP_CH_BROC);
  }

  if (m_MasterCurrrentMode)
    err = m_MasterCurrrentMode->Initialize(&m_Settings);

  return err;
}


/*!
 * Pre processing related functions
 */

bool cDSPProcessorStream::InputProcess(const float **array_in, unsigned int samples)
{
  return true;
}


/*!
 * Resampling related functions before master processing.
 * only one dsp addon is allowed to do this
 */

unsigned int cDSPProcessorStream::InputResampleProcessNeededSamplesize()
{
  return m_ProcessSamplesize;
}

int cDSPProcessorStream::InputResampleSampleRate()
{
  return m_ProcessSamplerate;
}

float cDSPProcessorStream::InputResampleGetDelay()
{
  return 0.0;
}

unsigned int cDSPProcessorStream::InputResampleProcess(float **array_in, float **array_out, unsigned int samples)
{
  return CopyInToOut(array_in, array_out, samples);
}

/*!
 * Pre processing related functions
 * all enabled addons allowed todo this
 */

unsigned int cDSPProcessorStream::PreProcessNeededSamplesize()
{
  return 0;
}

float cDSPProcessorStream::PreProcessGetDelay()
{
  return 0.0f;
}

unsigned int cDSPProcessorStream::PreProcess(float **array_in, float **array_out, unsigned int samples)
{
  return CopyInToOut(array_in, array_out, samples);
}

/*!
 * Master processing functions
 * only one during playback selectable dsp addon is allowed to do this
 */

AE_DSP_ERROR cDSPProcessorStream::MasterProcessSetMode(AE_DSP_STREAMTYPE type, unsigned int mode_id, int unique_db_mode_id)
{
  for (unsigned int i = 0; i < m_MasterModes.size(); ++i)
  {
    if (m_MasterModes[i] != NULL && m_MasterModes[i]->GetId() == mode_id)
    {
      m_MasterCurrrentMode = m_MasterModes[i];
      break;
    }
  }

  if (m_MasterCurrrentMode == NULL)
  {
    KODI->Log(LOG_ERROR, "Requested client id '%i' not present on current processor", mode_id);
    return AE_DSP_ERROR_UNKNOWN;
  }

  KODI->Log(LOG_INFO, "Master processing set mode to '%s' with id '%i'", m_MasterCurrrentMode->GetName(), mode_id);
  return AE_DSP_ERROR_NO_ERROR;
}

unsigned int cDSPProcessorStream::MasterProcessNeededSamplesize()
{
  if (!m_MasterCurrrentMode)
    return 0;
  return m_MasterCurrrentMode->GetNeededSamplesize();
}

float cDSPProcessorStream::MasterProcessGetDelay()
{
  if (!m_MasterCurrrentMode)
    return 0.0;
  return m_MasterCurrrentMode->GetDelay();
}

unsigned int cDSPProcessorStream::MasterProcess(float **array_in, float **array_out, unsigned int samples)
{
  if (!m_MasterCurrrentMode)
    return CopyInToOut(array_in, array_out, samples);
  return m_MasterCurrrentMode->Process(array_in, array_out, samples);
}

unsigned int cDSPProcessorStream::CopyInToOut(float **array_in, float **array_out, unsigned int samples)
{
  int presentFlag = 1;
  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
  {
    if (m_Settings.lOutChannelPresentFlags & presentFlag)
      memcpy(array_out[i], array_in[i], samples*sizeof(float));
    presentFlag <<= 1;
  }
  return samples;
}

int cDSPProcessorStream::MasterProcessGetOutChannels(unsigned long &out_channel_present_flags)
{
  if (!m_MasterCurrrentMode)
    return -1;
  return m_MasterCurrrentMode->MasterProcessGetOutChannels(out_channel_present_flags);
}

const char *cDSPProcessorStream::MasterProcessGetStreamInfoString()
{
  static std::string strStreamInfoString;
  if (m_MasterCurrrentMode)
    strStreamInfoString = m_MasterCurrrentMode->GetStreamInfoString();
  else
    strStreamInfoString = "";
  return strStreamInfoString.c_str();
}


/*!
 * Post processing related functions
 * all enabled addons allowed todo this
 */

unsigned int cDSPProcessorStream::PostProcessNeededSamplesize(unsigned int modeId)
{
  return 0;
}

float cDSPProcessorStream::PostProcessGetDelay(unsigned int modeId)
{
  CLockObject lock(g_DSPProcessor.m_Mutex);

  float delay = 0.0;

  if (g_DSPProcessor.m_SpeakerDelayMax > 0)
  {
    delay += (float)(g_DSPProcessor.m_SpeakerDelayMax) / DELAY_RESOLUTION;
  }

  return delay;
}

void cDSPProcessorStream::PostProcessChannelSample(AE_DSP_CHANNEL channel, float **array_out, int pos)
{
  array_out[channel][pos] = SoftClamp(g_DSPProcessor.m_OutputGain[channel] * array_out[channel][pos]);
  if (m_Delay[channel] != NULL)
  {
    m_Delay[channel]->Store(array_out[channel][pos]);
    array_out[channel][pos] = m_Delay[channel]->Retrieve();
  }
}

unsigned int cDSPProcessorStream::PostProcess(unsigned int modeId, float **array_in, float **array_out, unsigned int samples)
{
  /*!
   * This is a hacked way, is used to show on code of InitDSP() that it is possible to add several different process types
   * to one point identified by modeId. To have this hack working the distance correction must be enabled.
   * Normally my post processing must be used as only one post process mode (Speaker correction)
   */
  if (modeId == ID_POST_PROCESS_SPEAKER_CORRECTION)
  {
    if (m_SoundTest)
      return m_SoundTest->ProcessTestMode(array_in, array_out, samples);
    else
      samples = CopyInToOut(array_in, array_out, samples);

    CLockObject lock(g_DSPProcessor.m_Mutex);

    for (unsigned int pos = 0; pos < samples; pos++)
    {
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FL)
        PostProcessChannelSample(AE_DSP_CH_FL,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FR)
        PostProcessChannelSample(AE_DSP_CH_FR,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FC)
        PostProcessChannelSample(AE_DSP_CH_FC,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_LFE)
        PostProcessChannelSample(AE_DSP_CH_LFE,  array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BL)
        PostProcessChannelSample(AE_DSP_CH_BL,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BR)
        PostProcessChannelSample(AE_DSP_CH_BR,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FLOC)
        PostProcessChannelSample(AE_DSP_CH_FLOC, array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FROC)
        PostProcessChannelSample(AE_DSP_CH_FROC, array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BC)
        PostProcessChannelSample(AE_DSP_CH_BC,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_SL)
        PostProcessChannelSample(AE_DSP_CH_SL,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_SR)
        PostProcessChannelSample(AE_DSP_CH_SR,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFL)
        PostProcessChannelSample(AE_DSP_CH_TFL,  array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFR)
        PostProcessChannelSample(AE_DSP_CH_TFR,  array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFC)
        PostProcessChannelSample(AE_DSP_CH_TFC,  array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TC)
        PostProcessChannelSample(AE_DSP_CH_TC,   array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBL)
        PostProcessChannelSample(AE_DSP_CH_TBL,  array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBR)
        PostProcessChannelSample(AE_DSP_CH_TBR,  array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBC)
        PostProcessChannelSample(AE_DSP_CH_TBC,  array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BLOC)
        PostProcessChannelSample(AE_DSP_CH_BLOC, array_out, pos);
      if (m_Settings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BROC)
        PostProcessChannelSample(AE_DSP_CH_BROC, array_out, pos);
    }
  }
  return samples;
}

inline float cDSPProcessorStream::SoftClamp(float x)
{
#if 0
  /*
     This is a rational function to approximate a tanh-like soft clipper.
     It is based on the pade-approximation of the tanh function with tweaked coefficients.
     See: http://www.musicdsp.org/showone.php?id=238
  */
  if (x < -3.0f)
    return -1.0f;
  else if (x >  3.0f)
    return 1.0f;
  float y = x * x;
  return x * (27.0f + y) / (27.0f + 9.0f * y);
#else
  /* slower method using tanh, but more accurate */

  static const double k = 0.9f;
  /* perform a soft clamp */
  if (x >  k)
    x = (float)(tanh((x - k) / (1 - k)) * (1 - k) + k);
  else if (x < -k)
    x = (float)(tanh((x + k) / (1 - k)) * (1 - k) - k);

  /* hard clamp anything still outside the bounds */
  if (x >  1.0f)
    return  1.0f;
  if (x < -1.0f)
    return -1.0f;

  /* return the final sample */
  return x;
#endif
}

void cDSPProcessorStream::UpdateDelay(AE_DSP_CHANNEL channel)
{
  if (g_DSPProcessor.m_SpeakerDelay[channel] > 0)
  {
    if (m_Delay[channel] == NULL)
      m_Delay[channel] = new CDelay;

    m_Delay[channel]->Init(g_DSPProcessor.m_SpeakerDelay[channel], m_Settings.iProcessSamplerate);
  }
  else if (m_Delay[channel] != NULL)
  {
    delete m_Delay[channel];
    m_Delay[channel] = NULL;
  }
}

void cDSPProcessorStream::SetTestSound(AE_DSP_CHANNEL channel, int mode, CGUIDialogSpeakerGain *cbClass, bool continues)
{
  CLockObject lock(g_DSPProcessor.m_Mutex);

  if (mode != SOUND_TEST_OFF)
  {
    if (!m_SoundTest)
      m_SoundTest = new cDSPProcessorSoundTest(m_Settings.lOutChannelPresentFlags, cbClass);
    m_SoundTest->SetTestMode(mode, channel, continues);
  }
  else
  {
    if (m_SoundTest)
      delete m_SoundTest;
    m_SoundTest = NULL;
  }
}

AE_DSP_SETTINGS *cDSPProcessorStream::GetStreamSettings()
{
  return &m_Settings;
}


/*!
 * Resampling related functions after final processing.
 * only one dsp addon is allowed to do this
 */

unsigned int cDSPProcessorStream::OutputResampleProcessNeededSamplesize()
{
  return 0;
}

int cDSPProcessorStream::OutputResampleSampleRate()
{
  return m_Settings.iInSamplerate;
}

float cDSPProcessorStream::OutputResampleGetDelay()
{
  return 0.0;
}

unsigned int cDSPProcessorStream::OutputResampleProcess(float **array_in, float **array_out, unsigned int samples)
{
  return CopyInToOut(array_in, array_out, samples);
}


/*!
 * Processing functions
 */

cDSPProcessor g_DSPProcessor;

cDSPProcessor::cDSPProcessor() :
  m_outChannelPresentFlags(0)
{
}

cDSPProcessor::~cDSPProcessor()
{
  for (masterModesMap::iterator it = m_MasterModesMap.begin(); it != m_MasterModesMap.end(); ++it)
  {
    delete it->second;
  }
  m_MasterModesMap.clear();
}

bool cDSPProcessor::SupportsInputProcess() const
{
  return true;
}

bool cDSPProcessor::SupportsInputResample() const
{
  return false;
}

bool cDSPProcessor::SupportsPreProcess() const
{
  return true;
}

bool cDSPProcessor::SupportsMasterProcess() const
{
  return !m_MasterModesMap.empty();
}

bool cDSPProcessor::SupportsOutputResample() const
{
  return false;
}

bool cDSPProcessor::SupportsPostProcess() const
{
  return m_SpeakerCorrection;
}

bool cDSPProcessor::InitDSP()
{
  for (int i = 0; i < AE_DSP_STREAM_MAX_STREAMS; ++i)
    g_usedDSPs[i] = NULL;

  /*!
   * Reset all settings data to default
   */
  SetOutputGain(AE_DSP_CH_MAX, 0.0);

  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
    m_SpeakerDelay[i] = 0;

  m_SpeakerCorrection = false;

  CDSPSettings settings;
  settings.LoadSettingsData(-1, true);

  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
  {
    SetOutputGain((AE_DSP_CHANNEL) i, settings.m_Settings.m_channels[i].iVolumeCorrection);

    m_SpeakerDelay[i] = settings.m_Settings.m_channels[i].iDistanceCorrection;
    if (m_SpeakerDelay[i] > m_SpeakerDelayMax)
      m_SpeakerDelayMax = m_SpeakerDelay[i];
  }

  AE_DSP_MENUHOOK hook;

  /* Read setting "speaker_correction" from settings.xml */
  if (!KODI->GetSetting("speaker_correction", &m_SpeakerCorrection))
  {
    /* If setting is unknown fallback to defaults */
    KODI->Log(LOG_ERROR, "Couldn't get 'speaker_correction' setting, falling back to 'true' as default");
    m_SpeakerCorrection = true;
  }
  if (m_SpeakerCorrection)
  {
    hook.iHookId            = ID_MENU_SPEAKER_GAIN_SETUP;
    hook.category           = AE_DSP_MENUHOOK_POST_PROCESS;
    hook.iLocalizedStringId = 30011;
    hook.iRelevantModeId    = ID_POST_PROCESS_SPEAKER_CORRECTION;
    hook.bNeedPlayback      = false;
    ADSP->AddMenuHook(&hook);

    hook.iHookId            = ID_MENU_SPEAKER_DISTANCE_SETUP;
    hook.category           = AE_DSP_MENUHOOK_POST_PROCESS;
    hook.iLocalizedStringId = 30012;
    hook.iRelevantModeId    = ID_POST_PROCESS_SPEAKER_CORRECTION;
    hook.bNeedPlayback      = true;
    ADSP->AddMenuHook(&hook);
  }

  /* Read setting "master_stereo" from settings.xml */
  bool enable = false;
  if (!KODI->GetSetting("master_stereo", &enable))
  {
    /* If setting is unknown fallback to defaults */
    KODI->Log(LOG_ERROR, "Couldn't get 'master_stereo' setting, falling back to 'true' as default");
    enable = true;
  }
  EnableMasterProcessor(ID_MASTER_PROCESS_STEREO_DOWNMIX, enable);

  struct AE_DSP_MODES::AE_DSP_MODE modeInfoStruct;
  modeInfoStruct.iModeType              = AE_DSP_MODE_TYPE_POST_PROCESS;
  modeInfoStruct.iUniqueDBModeId        = -1;         // set by RegisterMode
  modeInfoStruct.iModeNumber            = ID_POST_PROCESS_SPEAKER_CORRECTION;
  modeInfoStruct.bHasSettingsDialog     = true;
  modeInfoStruct.iModeDescription       = 30005;
  modeInfoStruct.iModeHelp              = -1;
  modeInfoStruct.iModeName              = 30004;
  modeInfoStruct.iModeSetupName         = -1;
  modeInfoStruct.iModeSupportTypeFlags  = AE_DSP_PRSNT_ASTREAM_BASIC | AE_DSP_PRSNT_ASTREAM_MUSIC | AE_DSP_PRSNT_ASTREAM_MOVIE;
  strncpy(modeInfoStruct.strModeName, "Speaker correction", sizeof(modeInfoStruct.strModeName) - 1);
  memset(modeInfoStruct.strOwnModeImage, 0, sizeof(modeInfoStruct.strOwnModeImage)); // unused
  memset(modeInfoStruct.strOverrideModeImage, 0, sizeof(modeInfoStruct.strOverrideModeImage)); // unused

  ADSP->RegisterMode(&modeInfoStruct);

  return true;
}

void cDSPProcessor::DestroyDSP()
{
  for (int i = 0; i < AE_DSP_STREAM_MAX_STREAMS; ++i)
    SAFE_DELETE(g_usedDSPs[i]);

  for (masterModesMap::iterator it = m_MasterModesMap.begin(); it != m_MasterModesMap.end(); it++)
    delete it->second;
  m_MasterModesMap.clear();
}

bool cDSPProcessor::IsMasterProcessorEnabled(unsigned int masterId)
{
  CLockObject lock(m_Mutex);
  masterModesMap::iterator it = m_MasterModesMap.find(masterId);
  return it != m_MasterModesMap.end();
}

bool cDSPProcessor::EnableMasterProcessor(unsigned int masterId, bool enable)
{
  CLockObject lock(m_Mutex);

  masterModesMap::iterator it = m_MasterModesMap.find(masterId);
  if (enable && it == m_MasterModesMap.end())
  {
    CDSPProcessMaster *proc = CDSPProcessMaster::AllocateMaster(0, masterId);
    if (proc)
    {
      m_MasterModesMap.insert(make_pair(masterId, proc));
      ADSP->RegisterMode(&proc->m_ModeInfoStruct);
    }
    else
    {
      KODI->Log(LOG_ERROR, "Couldn't find master mode id '%i'", masterId);
      return false;
    }
  }
  else if (!enable && it != m_MasterModesMap.end())
  {
    ADSP->UnregisterMode(&it->second->m_ModeInfoStruct);
    delete it->second;
    m_MasterModesMap.erase(it);
  }

  return true;
}

ADDON_STATUS cDSPProcessor::SetSetting(const char *settingName, const void *settingValue)
{
  CLockObject lock(m_Mutex);

  AE_DSP_MENUHOOK hook;

  string str = settingName;
  if (str == "speaker_correction")
  {
    hook.iHookId            = ID_MENU_SPEAKER_GAIN_SETUP;
    hook.category           = AE_DSP_MENUHOOK_POST_PROCESS;
    hook.iLocalizedStringId = 30011;
    hook.bNeedPlayback      = true;
    hook.iRelevantModeId    = ID_POST_PROCESS_SPEAKER_CORRECTION;

    if (m_SpeakerCorrection && !* (bool *) settingValue)
      ADSP->RemoveMenuHook(&hook);
    else if (!m_SpeakerCorrection && * (bool *) settingValue)
      ADSP->AddMenuHook(&hook);

    hook.iHookId            = ID_MENU_SPEAKER_DISTANCE_SETUP;
    hook.category           = AE_DSP_MENUHOOK_POST_PROCESS;
    hook.iLocalizedStringId = 30012;
    hook.bNeedPlayback      = true;
    hook.iRelevantModeId    = ID_POST_PROCESS_SPEAKER_CORRECTION;

    if (m_SpeakerCorrection && !* (bool *) settingValue)
      ADSP->RemoveMenuHook(&hook);
    else if (!m_SpeakerCorrection && * (bool *) settingValue)
      ADSP->AddMenuHook(&hook);

    KODI->Log(LOG_INFO, "Changed Setting 'speaker_correction' from %u to %u", m_SpeakerCorrection, * (bool *) settingValue);
    m_SpeakerCorrection = * (bool *) settingValue;
  }
  else if (str == "master_stereo")
  {
    KODI->Log(LOG_INFO, "Changed Setting 'master_stereo' from %u to %u", IsMasterProcessorEnabled(ID_MASTER_PROCESS_STEREO_DOWNMIX), * (bool *) settingValue);
    EnableMasterProcessor(ID_MASTER_PROCESS_STEREO_DOWNMIX, * (bool *) settingValue);
  }

  return ADDON_STATUS_OK;
}

AE_DSP_ERROR cDSPProcessor::CallMenuHook(const AE_DSP_MENUHOOK &menuhook, const AE_DSP_MENUHOOK_DATA &item)
{
  if (menuhook.iHookId == ID_MENU_SPEAKER_GAIN_SETUP && m_SpeakerCorrection)
  {
    CGUIDialogSpeakerGain settings(item.data.iStreamId);
    settings.DoModal();
  }
  else if (menuhook.iHookId == ID_MENU_SPEAKER_DISTANCE_SETUP && m_SpeakerCorrection)
  {
    CGUIDialogSpeakerDistance settings(item.data.iStreamId);
    settings.DoModal();
  }
  return AE_DSP_ERROR_NO_ERROR;
}

void cDSPProcessor::SetOutputGain(AE_DSP_CHANNEL channel, float GainCoeff)
{
  CLockObject lock(m_Mutex);

  GainCoeff = GainToScale(GainCoeff);
  if (GainCoeff > 2.0)
    GainCoeff = 2.0;
  if (GainCoeff < 0)
    GainCoeff = 0;

  if (channel == AE_DSP_CH_MAX)
  {
    for (unsigned i = 0; i < AE_DSP_CH_MAX; ++i)
      g_DSPProcessor.m_OutputGain[i] = GainCoeff;
  }
  else if (channel < AE_DSP_CH_MAX && channel > AE_DSP_CH_INVALID)
    g_DSPProcessor.m_OutputGain[channel] = GainCoeff;
}

void cDSPProcessor::SetDelay(AE_DSP_CHANNEL channel, unsigned int delay)
{
  CLockObject lock(m_Mutex);

  m_SpeakerDelay[channel] = delay;

  if (delay > m_SpeakerDelayMax)
    m_SpeakerDelayMax = delay;
  else
  {
    m_SpeakerDelayMax = 0;
    for (int i = 0; i < AE_DSP_CH_MAX; ++i)
    {
      if (m_SpeakerDelay[i] > m_SpeakerDelayMax)
        m_SpeakerDelayMax = m_SpeakerDelay[i];
    }
  }

  for (int i = 0; i < AE_DSP_STREAM_MAX_STREAMS; ++i)
  {
    if (g_usedDSPs[i] != NULL)
      g_usedDSPs[i]->UpdateDelay(channel);
  }
}

void cDSPProcessor::SetTestSound(AE_DSP_CHANNEL channel, int mode, CGUIDialogSpeakerGain *cbClass, bool continues)
{
  CLockObject lock(m_Mutex);

  for (int i = 0; i < AE_DSP_STREAM_MAX_STREAMS; ++i)
  {
    if (g_usedDSPs[i] != NULL)
      g_usedDSPs[i]->SetTestSound(channel, mode, cbClass, continues);
  }
}

CDSPProcessMaster *cDSPProcessor::GetProcessMaster(unsigned streamId)
{
  CLockObject lock(m_Mutex);
  if (streamId >= AE_DSP_STREAM_MAX_STREAMS ||
      g_usedDSPs[streamId] == NULL ||
      g_usedDSPs[streamId]->m_MasterCurrrentMode == NULL)
    return NULL;

  return g_usedDSPs[streamId]->m_MasterCurrrentMode;
}

