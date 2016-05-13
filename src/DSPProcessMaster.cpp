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

#include "libXBMC_addon.h"
#include "p8-platform/util/util.h"
#include "p8-platform/util/StdString.h"

#include "DSPProcessMaster.h"
#include "Process_Stereo/DSPProcessStereo.h"

using namespace std;

CDSPProcessMaster::CDSPProcessMaster(AE_DSP_STREAM_ID streamId, unsigned int modeId, const char *modeName)
  : m_StreamId(streamId),
    m_ModeId(modeId),
    m_ModeName(modeName)
{
  m_ModeInfoStruct.iModeType = AE_DSP_MODE_TYPE_MASTER_PROCESS;
}

CDSPProcessMaster::~CDSPProcessMaster()
{
}

CDSPProcessMaster *CDSPProcessMaster::AllocateMaster(AE_DSP_STREAM_ID streamId, unsigned int modeId)
{
  CDSPProcessMaster *mode = NULL;
  switch (modeId)
  {
    case ID_MASTER_PROCESS_STEREO_DOWNMIX:
      mode = new CDSPProcess_StereoDownmix(streamId);
      break;
    default:
      break;
  }
  return mode;
}
