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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <complex>
#include <cmath>
#include <vector>

#include "../DSPProcessMaster.h"

#define DM_GAIN 1.0
#define DM_FL   1.0
#define DM_CL   0.707
#define DM_LFEL 0.707
#define DM_SLA  0.8165
#define DM_SLB  0.5774

#define D_SIZE 256
#define NZEROS 200

class CDSPProcess_StereoDownmix : public CDSPProcessMaster
{
private:
  int               m_InputChannels;  ///< Current input channel format
  bool              m_StereoDownmix;  ///< Downmix multichannel to stereo
  bool              m_StereoLFE;      ///< Output also to LFE enabled if true
  unsigned int      m_SampleRate;     ///< The current sample rate (not used in the moment)

  float            *m_DelayRL;        ///< Delay audio buffer for back channel rear left
  float            *m_DelayRR;        ///< Delay audio buffer for back channel rear right
  unsigned int      m_dptrRL;         ///< X coefficients pointer for rear left
  unsigned int      m_dptrRR;         ///< X coefficients pointer for rear right

public:
  CDSPProcess_StereoDownmix(unsigned int streamId);
  virtual ~CDSPProcess_StereoDownmix();

  virtual const char *GetName();
  virtual bool IsSupported(const AE_DSP_SETTINGS *settings, const AE_DSP_STREAM_PROPERTIES *pProperties);
  virtual AE_DSP_ERROR Initialize(const AE_DSP_SETTINGS *settings);
  virtual void Deinitialize() {}
  virtual float GetDelay();
  virtual unsigned int Process(float **array_in, float **array_out, unsigned int samples);
  virtual int MasterProcessGetOutChannels(unsigned long &out_channel_present_flags);
};
