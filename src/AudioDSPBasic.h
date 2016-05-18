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

#include <string>
#include <vector>
#include <map>

#include "kodi_adsp_types.h"

#include "p8-platform/threads/threads.h"
#include "p8-platform/threads/mutex.h"
#include "filter/delay.h"

#include "DSPProcessMaster.h"

// Maximal channels
#define MAX_CHANNEL 16

// This rate is a default value required (especially by division) prior to
// the audio framework providing the value
#define DEFAULT_SAMPLING_RATE 44100
#define MAX_SAMPLING_RATE 192000

#define SPEAKER_GAIN_RANGE_DB_MIN -12
#define SPEAKER_GAIN_RANGE_DB_MAX +6

// Convert a value in dB's to a coefficent
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#define CO_DB(v) (20.0f * log10f(v))

extern std::string g_strAddonPath;
extern std::string GetSettingsFile();

class cDSPProcessor;
class cDSPProcessorSoundTest;
class CGUIDialogSpeakerGain;

using namespace P8PLATFORM;

typedef std::map<unsigned int, CDSPProcessMaster *> masterModesMap;

class cDSPProcessorStream
{
  /*!
   * Basic addon functions
   */
public:
  cDSPProcessorStream(unsigned int id);
  virtual ~cDSPProcessorStream();

  /*!
   * Control function for start and stop of dsp processing.
   */
  AE_DSP_ERROR StreamCreate(const AE_DSP_SETTINGS *settings, const AE_DSP_STREAM_PROPERTIES *pProperties);
  AE_DSP_ERROR StreamDestroy();
  AE_DSP_ERROR StreamIsModeSupported(AE_DSP_MODE_TYPE mode_type, unsigned int mode_id, int unique_db_mode_id);
  AE_DSP_ERROR StreamInitialize(const AE_DSP_SETTINGS *settings);

  /*!
   * Pre processing related functions before pre resampling.
   * all enabled dsp addons called todo it
   */
  bool InputProcess(const float **array_in, unsigned int samples);

  /*!
   * Resampling related functions before master processing.
   * only one dsp addon is allowed to do this
   */
  unsigned int  InputResampleProcessNeededSamplesize();
  int           InputResampleSampleRate();
  float         InputResampleGetDelay();
  unsigned int  InputResampleProcess(float **array_in, float **array_out, unsigned int samples);

  /*!
   * Pre processing related functions
   * all enabled addons allowed todo this
   */
  unsigned int PreProcessNeededSamplesize();
  float        PreProcessGetDelay();
  unsigned int PreProcess(float **array_in, float **array_out, unsigned int samples);

  /*!
   * Master processing functions
   * only one during playback selectable dsp addon is allowed to do this
   */
  AE_DSP_ERROR  MasterProcessSetMode(AE_DSP_STREAMTYPE type, unsigned int mode_id, int unique_db_mode_id);
  unsigned int  MasterProcessNeededSamplesize();
  float         MasterProcessGetDelay();
  unsigned int  MasterProcess(float **array_in, float **array_out, unsigned int samples);
  int           MasterProcessGetOutChannels(unsigned long &out_channel_present_flags);
  const char   *MasterProcessGetStreamInfoString();

  /*!
   * Post processing related functions
   * all enabled addons allowed todo this
   */
  unsigned int PostProcessNeededSamplesize(unsigned int modeId);
  float        PostProcessGetDelay(unsigned int modeId);
  unsigned int PostProcess(unsigned int modeId, float **array_in, float **array_out, unsigned int samples);

  /*!
   * Resampling related functions after final processing.
   * only one dsp addon is allowed to do this
   */
  unsigned int  OutputResampleProcessNeededSamplesize();
  int           OutputResampleSampleRate();
  float         OutputResampleGetDelay();
  unsigned int  OutputResampleProcess(float **array_in, float **array_out, unsigned int samples);

private:
  const unsigned int        m_StreamID;           /*!< @brief (required) unique id of the audio stream packets */
  AE_DSP_SETTINGS           m_Settings;           /*!< @brief (required) the active KODI audio settings */
  AE_DSP_STREAM_PROPERTIES  m_Properties;
  int                       m_iStreamType;
  int                       m_iBaseType;
  std::string               m_strName;            /*!< @brief (required) the audio stream name */
  std::string               m_strCodecId;         /*!< @brief (required) codec id string of the audio stream */
  std::string               m_strLanguage;        /*!< @brief (required) language id of the audio stream */
  int                       m_iIdentifier;        /*!< @brief (required) audio stream id inside player */
  int                       m_iChannels;          /*!< @brief (required) amount of basic channels */
  int                       m_iSampleRate;        /*!< @brief (required) input sample rate */

  /*!
   * Internal processing functions
   */
public:
  void UpdateDelay(AE_DSP_CHANNEL channel);
  void SetTestSound(AE_DSP_CHANNEL channel, int mode, CGUIDialogSpeakerGain *cbClass = NULL, bool continues = false);
  AE_DSP_SETTINGS *GetStreamSettings();

private:
  friend class cDSPProcessor;

  unsigned int CopyInToOut(float **array_in, float **array_out, unsigned int samples);
  void PostProcessChannelSample(AE_DSP_CHANNEL channel, float **array_out, int pos);

  float SoftClamp(float x);

  CDelay                           *m_Delay[AE_DSP_CH_MAX];

  unsigned int                      m_ProcessSamplerate;
  unsigned int                      m_ProcessSamplesize;
  double                            m_ProcessSourceRatio;

  cDSPProcessorSoundTest           *m_SoundTest;
  std::vector<CDSPProcessMaster*>   m_MasterModes;
  CDSPProcessMaster                *m_MasterCurrrentMode;
};

/*!
 * Master Processing functions
 */
class cDSPProcessor
{
public:
  cDSPProcessor();
  virtual ~cDSPProcessor();

  bool SupportsInputProcess() const;
  bool SupportsInputResample() const;
  bool SupportsPreProcess() const;
  bool SupportsMasterProcess() const;
  bool SupportsPostProcess() const;
  bool SupportsOutputResample() const;

  bool InitDSP();
  void DestroyDSP();
  ADDON_STATUS SetSetting(const char *settingName, const void *settingValue);
  AE_DSP_ERROR CallMenuHook(const AE_DSP_MENUHOOK &menuhook, const AE_DSP_MENUHOOK_DATA &item);
  void SetOutputGain(AE_DSP_CHANNEL channel, float GainCoeff);
  void SetDelay(AE_DSP_CHANNEL channel, unsigned int delay);
  void SetTestSound(AE_DSP_CHANNEL channel, int mode, CGUIDialogSpeakerGain *cbClass = NULL, bool continues = false);
  CDSPProcessMaster *GetProcessMaster(unsigned streamId);

  void SetOutChannelPresentFlags(unsigned long flags) { m_outChannelPresentFlags = flags; }
  unsigned long GetOutChannelPresentFlags() { return m_outChannelPresentFlags; }

protected:
  friend class cDSPProcessorStream;
  friend class cDSPProcessorSoundTest;

  bool IsMasterProcessorEnabled(unsigned int masterId);
  bool EnableMasterProcessor(unsigned int masterId, bool enable);

  masterModesMap           m_MasterModesMap;

  AE_DSP_CHANNEL_PRESENT   m_CurrentOutChannelPresentFlags;

  float                    m_OutputGain[AE_DSP_CH_MAX];
  unsigned int             m_SpeakerDelay[AE_DSP_CH_MAX];
  unsigned int             m_SpeakerDelayMax;
  bool                     m_SpeakerCorrection;
  unsigned long            m_outChannelPresentFlags;

  P8PLATFORM::CMutex         m_Mutex;
};

extern cDSPProcessor g_DSPProcessor;
