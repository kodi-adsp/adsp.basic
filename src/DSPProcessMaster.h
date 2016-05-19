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

#include "kodi_adsp_types.h"

#define ID_MENU_SPEAKER_GAIN_SETUP                      1
#define ID_MENU_SPEAKER_DISTANCE_SETUP                  2

#define ID_MASTER_PROCESS_STEREO_DOWNMIX                1300
#define ID_POST_PROCESS_SPEAKER_CORRECTION              1400

class CDSPProcessMaster
{
public:
  CDSPProcessMaster(unsigned int streamId, unsigned int modeId, const char *modeName);
  virtual ~CDSPProcessMaster();

  unsigned int GetStreamId() const { return m_StreamId; }
  unsigned int GetId() const { return m_ModeId; }
  const char *GetName() { return m_ModeName; }
  virtual const char *GetStreamInfoString() { return ""; }
  virtual bool IsSupported(const AE_DSP_SETTINGS *settings, const AE_DSP_STREAM_PROPERTIES *pProperties) = 0;
  virtual AE_DSP_ERROR Initialize(const AE_DSP_SETTINGS *settings) = 0;
  virtual void Deinitialize() = 0;
  virtual void ResetSettings() {}
  virtual float GetDelay() = 0;
  virtual unsigned int GetNeededSamplesize() { return 0; }
  virtual unsigned int Process(float **array_in, float **array_out, unsigned int samples) = 0;
  virtual int MasterProcessGetOutChannels(unsigned long &out_channel_present_flags) { return -1; }

  static CDSPProcessMaster *AllocateMaster(unsigned int streamId, unsigned int modeId);

  struct AE_DSP_MODES::AE_DSP_MODE m_ModeInfoStruct;

protected:
  const unsigned int  m_StreamId;
  const unsigned int  m_ModeId;
  const char         *m_ModeName;
};
