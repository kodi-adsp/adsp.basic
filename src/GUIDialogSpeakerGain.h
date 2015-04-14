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

#include "AudioDSPSettings.h"

class CGUIDialogSpeakerGain : private CDSPSettings
{
public:
  CGUIDialogSpeakerGain(unsigned int streamId);
  virtual ~CGUIDialogSpeakerGain();

  bool Show();
  void Close();
  void DoModal();

  void ContinuesTestSwitchInfoCB(AE_DSP_CHANNEL channel);

private:
  bool OnClick(int controlId);
  bool OnFocus(int controlId);
  bool OnInit();
  bool OnAction(int actionId);

  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
  static bool OnInitCB(GUIHANDLE cbhdl);
  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);

  void SetVolumeSpin(int id, AE_DSP_CHANNEL channel, bool present);
  void SetVolumeSpins();

  const unsigned int        m_StreamId;
  int                       m_GainTestSound;
  CAddonGUIWindow          *m_window;
  CAddonGUISpinControl     *m_spinSpeakerGainTest;
  CAddonGUIRadioButton     *m_radioSpeakerContinuesTest;
};
