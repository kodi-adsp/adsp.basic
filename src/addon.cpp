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

#include <vector>
#include <string>
#include "addon.h"
#include "kodi_adsp_dll.h"
#include "p8-platform/util/util.h"
#include "p8-platform/util/StdString.h"
#include "AudioDSPBasic.h"

using namespace std;
using namespace ADDON;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

#if defined(TARGET_WINDOWS)
  #undef CreateDirectory
#endif

int            m_iStreamsPresent  = 0;
bool           m_bCreated         = false;
ADDON_STATUS   m_CurStatus        = ADDON_STATUS_UNKNOWN;

/* User adjustable settings are saved here.
 * Default values are defined inside addon.h
 * and exported to the other source files.
 */
std::string   g_strUserPath       = "";
std::string   g_strAddonPath      = "";

CHelper_libXBMC_addon  *KODI      = NULL;
CHelper_libKODI_adsp   *ADSP      = NULL;
CHelper_libKODI_guilib *GUI       = NULL;

/*!
 * Pointer array for active dsp processing classes, for this reason the
 * stream id is never more as AE_DSP_STREAM_MAX_STREAMS and can be used as pointer to this array.
 */
extern cDSPProcessorStream *g_usedDSPs[AE_DSP_STREAM_MAX_STREAMS];

extern "C" {

void ADDON_ReadSettings(void)
{

}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  AE_DSP_PROPERTIES* adspprops = (AE_DSP_PROPERTIES*)props;

  KODI = new CHelper_libXBMC_addon;
  if (!KODI->RegisterMe(hdl))
  {
    SAFE_DELETE(KODI);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  GUI = new CHelper_libKODI_guilib;
  if (!GUI->RegisterMe(hdl))
  {
    SAFE_DELETE(GUI);
    SAFE_DELETE(KODI);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  ADSP = new CHelper_libKODI_adsp;
  if (!ADSP->RegisterMe(hdl))
  {
    SAFE_DELETE(ADSP);
    SAFE_DELETE(GUI);
    SAFE_DELETE(KODI);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  KODI->Log(LOG_DEBUG, "%s - Creating the basic audio DSP processing system", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_strUserPath   = adspprops->strUserPath;
  g_strAddonPath  = adspprops->strAddonPath;

  // create addon user path
  if (!KODI->DirectoryExists(g_strUserPath.c_str()))
  {
    KODI->CreateDirectory(g_strUserPath.c_str());
  }

  ADDON_ReadSettings();

  if (!g_DSPProcessor.InitDSP())
    return m_CurStatus;

  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
  m_iStreamsPresent = 0;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  m_bCreated = false;
  m_iStreamsPresent = 0;

  g_DSPProcessor.DestroyDSP();

  SAFE_DELETE(ADSP);
  SAFE_DELETE(GUI);
  SAFE_DELETE(KODI);

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  return g_DSPProcessor.SetSetting(settingName, settingValue);
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}


/***********************************************************
 * Audio DSP Client AddOn specific public library functions
 ***********************************************************/

const char* GetAudioDSPAPIVersion(void)
{
  return KODI_AE_DSP_API_VERSION;
}

const char* GetMinimumAudioDSPAPIVersion(void)
{
  return KODI_AE_DSP_MIN_API_VERSION;
}

const char* GetGUIAPIVersion(void)
{
  return KODI_GUILIB_API_VERSION;
}

const char* GetMinimumGUIAPIVersion(void)
{
  return KODI_GUILIB_MIN_API_VERSION;
}

AE_DSP_ERROR GetAddonCapabilities(AE_DSP_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsInputProcess    = g_DSPProcessor.SupportsInputProcess();
  pCapabilities->bSupportsInputResample   = g_DSPProcessor.SupportsInputResample();
  pCapabilities->bSupportsPreProcess      = g_DSPProcessor.SupportsPreProcess();
  pCapabilities->bSupportsMasterProcess   = g_DSPProcessor.SupportsMasterProcess();
  pCapabilities->bSupportsPostProcess     = g_DSPProcessor.SupportsPostProcess();
  pCapabilities->bSupportsOutputResample  = g_DSPProcessor.SupportsOutputResample();

  return AE_DSP_ERROR_NO_ERROR;
}

const char *GetDSPName(void)
{
  return "Basic audio DSP processing system";
}

const char *GetDSPVersion(void)
{
  return ADSP_BASIC_VERSION;
}

AE_DSP_ERROR CallMenuHook(const AE_DSP_MENUHOOK &menuhook, const AE_DSP_MENUHOOK_DATA &item)
{
  return g_DSPProcessor.CallMenuHook(menuhook, item);
}


/*!
 * Control function for start and stop of dsp processing.
 */

AE_DSP_ERROR StreamCreate(const AE_DSP_SETTINGS *addonSettings, const AE_DSP_STREAM_PROPERTIES* pProperties, ADDON_HANDLE handle)
{
  if (g_usedDSPs[addonSettings->iStreamID])
  {
    delete g_usedDSPs[addonSettings->iStreamID];
    g_usedDSPs[addonSettings->iStreamID] = NULL;
  }

  cDSPProcessorStream *proc = new cDSPProcessorStream(addonSettings->iStreamID);
  AE_DSP_ERROR err = proc->StreamCreate(addonSettings, pProperties);
  if (err == AE_DSP_ERROR_NO_ERROR)
  {
    g_usedDSPs[addonSettings->iStreamID] = proc;
    handle->dataIdentifier = addonSettings->iStreamID;
    handle->callerAddress = proc;
  }
  else
    delete proc;

  return err;
}

AE_DSP_ERROR StreamDestroy(const ADDON_HANDLE handle)
{
  AE_DSP_ERROR err = ((cDSPProcessorStream*)handle->callerAddress)->StreamDestroy();
  delete ((cDSPProcessorStream*)handle->callerAddress);
  g_usedDSPs[handle->dataIdentifier] = NULL;
  return err;
}

AE_DSP_ERROR StreamInitialize(const ADDON_HANDLE handle, const AE_DSP_SETTINGS *settings)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->StreamInitialize(settings);
}

AE_DSP_ERROR StreamIsModeSupported(const ADDON_HANDLE handle, AE_DSP_MODE_TYPE type, unsigned int mode_id, int unique_db_mode_id)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->StreamIsModeSupported(type, mode_id, unique_db_mode_id);
}


/*!
 * Input processing related functions
 */

bool InputProcess(const ADDON_HANDLE handle, const float **array_in, unsigned int samples)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->InputProcess(array_in, samples);
}


/*!
 * Resampling related functions before master processing.
 * only one dsp addon is allowed to do this
 */

unsigned int InputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->InputResampleProcessNeededSamplesize();
}

int InputResampleSampleRate(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->InputResampleSampleRate();
}

float InputResampleGetDelay(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->InputResampleGetDelay();
}

unsigned int InputResampleProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->InputResampleProcess(array_in, array_out, samples);
}


/*!
 * Pre processing related functions
 * all enabled addons allowed todo this
 */

unsigned int PreProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->PreProcessNeededSamplesize();
}

float PreProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->PreProcessGetDelay();
}

unsigned int PreProcess(const ADDON_HANDLE handle, unsigned int mode_id, float **array_in, float **array_out, unsigned int samples)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->PreProcess(array_in, array_out, samples);
}

/*!
 * Master processing functions
 * only one during playback selectable dsp addon is allowed to do this
 */

AE_DSP_ERROR MasterProcessSetMode(const ADDON_HANDLE handle, AE_DSP_STREAMTYPE type, unsigned int client_mode_id, int unique_db_mode_id)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->MasterProcessSetMode(type, client_mode_id, unique_db_mode_id);
}

unsigned int MasterProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->MasterProcessNeededSamplesize();
}

float MasterProcessGetDelay(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->MasterProcessGetDelay();
}

unsigned int MasterProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->MasterProcess(array_in, array_out, samples);
}

int MasterProcessGetOutChannels(const ADDON_HANDLE handle, unsigned long &out_channel_present_flags)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->MasterProcessGetOutChannels(out_channel_present_flags);
}

const char *MasterProcessGetStreamInfoString(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->MasterProcessGetStreamInfoString();
}


/*!
 * Post processing related functions
 * all enabled addons allowed todo this
 */

unsigned int PostProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->PostProcessNeededSamplesize(mode_id);
}

float PostProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->PostProcessGetDelay(mode_id);
}

unsigned int PostProcess(const ADDON_HANDLE handle, unsigned int mode_id, float **array_in, float **array_out, unsigned int samples)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->PostProcess(mode_id, array_in, array_out, samples);
}


/*!
 * Resampling related functions after final processing.
 * only one dsp addon is allowed to do this
 */

unsigned int OutputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->OutputResampleProcessNeededSamplesize();
}

int OutputResampleSampleRate(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->OutputResampleSampleRate();
}

float OutputResampleGetDelay(const ADDON_HANDLE handle)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->OutputResampleGetDelay();
}

unsigned int OutputResampleProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples)
{
  return ((cDSPProcessorStream*)handle->callerAddress)->OutputResampleProcess(array_in,  array_out, samples);
}

}
