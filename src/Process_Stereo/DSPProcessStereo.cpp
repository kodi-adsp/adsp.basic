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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "libXBMC_addon.h"
#include "libKODI_adsp.h"

#include "DSPProcessStereo.h"
#include "../addon.h"
#include "../AudioDSPBasic.h"

/* The non-zero taps of the Hilbert transformer */
static float xcoeffs[] = {
  +0.0008103736f, +0.0008457886f, +0.0009017196f, +0.0009793364f,
  +0.0010798341f, +0.0012044365f, +0.0013544008f, +0.0015310235f,
  +0.0017356466f, +0.0019696659f, +0.0022345404f, +0.0025318040f,
  +0.0028630784f, +0.0032300896f, +0.0036346867f, +0.0040788644f,
  +0.0045647903f, +0.0050948365f, +0.0056716186f, +0.0062980419f,
  +0.0069773575f, +0.0077132300f, +0.0085098208f, +0.0093718901f,
  +0.0103049226f, +0.0113152847f, +0.0124104218f, +0.0135991079f,
  +0.0148917649f, +0.0163008758f, +0.0178415242f, +0.0195321089f,
  +0.0213953037f, +0.0234593652f, +0.0257599469f, +0.0283426636f,
  +0.0312667947f, +0.0346107648f, +0.0384804823f, +0.0430224431f,
  +0.0484451086f, +0.0550553725f, +0.0633242001f, +0.0740128560f,
  +0.0884368322f, +0.1090816773f, +0.1412745301f, +0.1988673273f,
  +0.3326528346f, +0.9997730178f, -0.9997730178f, -0.3326528346f,
  -0.1988673273f, -0.1412745301f, -0.1090816773f, -0.0884368322f,
  -0.0740128560f, -0.0633242001f, -0.0550553725f, -0.0484451086f,
  -0.0430224431f, -0.0384804823f, -0.0346107648f, -0.0312667947f,
  -0.0283426636f, -0.0257599469f, -0.0234593652f, -0.0213953037f,
  -0.0195321089f, -0.0178415242f, -0.0163008758f, -0.0148917649f,
  -0.0135991079f, -0.0124104218f, -0.0113152847f, -0.0103049226f,
  -0.0093718901f, -0.0085098208f, -0.0077132300f, -0.0069773575f,
  -0.0062980419f, -0.0056716186f, -0.0050948365f, -0.0045647903f,
  -0.0040788644f, -0.0036346867f, -0.0032300896f, -0.0028630784f,
  -0.0025318040f, -0.0022345404f, -0.0019696659f, -0.0017356466f,
  -0.0015310235f, -0.0013544008f, -0.0012044365f, -0.0010798341f,
  -0.0009793364f, -0.0009017196f, -0.0008457886f, -0.0008103736f,
};

CDSPProcess_StereoDownmix::CDSPProcess_StereoDownmix(unsigned int streamId)
  : CDSPProcessMaster(streamId, ID_MASTER_PROCESS_STEREO_DOWNMIX, "StereoDownmix")
{
  m_ModeInfoStruct.iUniqueDBModeId        = -1;         // set by RegisterMode
  m_ModeInfoStruct.iModeNumber            = ID_MASTER_PROCESS_STEREO_DOWNMIX;
  m_ModeInfoStruct.bHasSettingsDialog     = false;
  m_ModeInfoStruct.iModeDescription       = 30002;
  m_ModeInfoStruct.iModeHelp              = 30003;
  m_ModeInfoStruct.iModeName              = 30000;
  m_ModeInfoStruct.iModeSetupName         = -1;
  m_ModeInfoStruct.iModeSupportTypeFlags  = AE_DSP_PRSNT_ASTREAM_BASIC | AE_DSP_PRSNT_ASTREAM_MUSIC | AE_DSP_PRSNT_ASTREAM_MOVIE;
  m_ModeInfoStruct.bIsDisabled            = false;

  strncpy(m_ModeInfoStruct.strModeName, m_ModeName, sizeof(m_ModeInfoStruct.strModeName) - 1);
  memset(m_ModeInfoStruct.strOwnModeImage, 0, sizeof(m_ModeInfoStruct.strOwnModeImage)); // unused
  memset(m_ModeInfoStruct.strOverrideModeImage, 0, sizeof(m_ModeInfoStruct.strOverrideModeImage)); // unused

  m_DelayRL = (float*) calloc(D_SIZE, sizeof(float));
  m_DelayRR = (float*) calloc(D_SIZE, sizeof(float));
}

CDSPProcess_StereoDownmix::~CDSPProcess_StereoDownmix()
{
  free(m_DelayRL);
  free(m_DelayRR);
}

const char *CDSPProcess_StereoDownmix::GetName()
{
  return m_ModeName;
}

bool CDSPProcess_StereoDownmix::IsSupported(const AE_DSP_SETTINGS *settings, const AE_DSP_STREAM_PROPERTIES *pProperties)
{
  /* For surround downmix 5.1 to 2.0 */
  if (settings->iInChannels > 2 && settings->iOutChannels == 2)
    return true;

  return false;
}

AE_DSP_ERROR CDSPProcess_StereoDownmix::Initialize(const AE_DSP_SETTINGS *settings)
{
  m_SampleRate    = settings->iProcessSamplerate;
  m_StereoDownmix = settings->iOutChannels == 2;
  m_StereoLFE     = settings->lOutChannelPresentFlags & AE_DSP_PRSNT_CH_LFE;
  m_InputChannels = settings->iInChannels;
  m_dptrRL        = 0;
  m_dptrRR        = 0;

  return AE_DSP_ERROR_NO_ERROR;
}

float CDSPProcess_StereoDownmix::GetDelay()
{
  return 0.0;
}

unsigned int CDSPProcess_StereoDownmix::Process(float **array_in, float **array_out, unsigned int samples)
{
  float hilbRL;
  float hilbRR;

  for (unsigned pos = 0; pos < samples; pos++)
  {
    m_DelayRL[m_dptrRL] = array_in[AE_DSP_CH_SL][pos];
    m_DelayRR[m_dptrRR] = array_in[AE_DSP_CH_SR][pos];

    hilbRL = 0.0f;
    hilbRR = 0.0f;
    for (unsigned int i = 0; i < NZEROS/2; i++)
    {
      hilbRL += (xcoeffs[i] * m_DelayRL[(m_dptrRL - i*2) & (D_SIZE - 1)]);
      hilbRR += (xcoeffs[i] * m_DelayRR[(m_dptrRR - i*2) & (D_SIZE - 1)]);
    }

    array_in[AE_DSP_CH_SL][pos] = hilbRL;
    array_in[AE_DSP_CH_SR][pos] = hilbRR;

    m_dptrRL = (m_dptrRL + 1) & (D_SIZE - 1);
    m_dptrRR = (m_dptrRR + 1) & (D_SIZE - 1);

    array_out[AE_DSP_CH_FL][pos] = DM_GAIN * (DM_FL*array_in[AE_DSP_CH_FL][pos] +
                                              DM_CL*array_in[AE_DSP_CH_FC][pos] +
                                              DM_SLA*array_in[AE_DSP_CH_SL][pos] +
                                              DM_SLB*array_in[AE_DSP_CH_SR][pos]);

    array_out[AE_DSP_CH_FR][pos] = DM_GAIN * (DM_FL*array_in[AE_DSP_CH_FR][pos] +
                                              DM_CL*array_in[AE_DSP_CH_FC][pos] +
                                              DM_SLB*array_in[AE_DSP_CH_SL][pos] -
                                              DM_SLA*array_in[AE_DSP_CH_SR][pos]);

    if (m_StereoLFE)
      array_out[AE_DSP_CH_LFE][pos] = (array_in[AE_DSP_CH_FL][pos] + array_in[AE_DSP_CH_FR][pos]) * 0.5;
    else
      array_out[AE_DSP_CH_LFE][pos] = 0.0f;

    array_out[AE_DSP_CH_SL][pos] = 0.0f;
    array_out[AE_DSP_CH_SR][pos] = 0.0f;
    array_out[AE_DSP_CH_BL][pos] = 0.0f;
    array_out[AE_DSP_CH_BR][pos] = 0.0f;
    array_out[AE_DSP_CH_FC][pos] = 0.0f;
  }
  return samples;
}

int CDSPProcess_StereoDownmix::MasterProcessGetOutChannels(unsigned long &out_channel_present_flags)
{
  out_channel_present_flags = AE_DSP_CH_FL | AE_DSP_CH_FR;
  return 2;
}
