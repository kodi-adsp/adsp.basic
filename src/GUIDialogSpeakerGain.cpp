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
#include "libKODI_adsp.h"
#include "libKODI_guilib.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/util.h"
#include "p8-platform/util/StdString.h"

#include "GUIDialogSpeakerGain.h"
#include "AudioDSPSoundTest.h"

using namespace std;
using namespace ADDON;

#define BUTTON_OK                                 1
#define BUTTON_CANCEL                             2

#define SPIN_CONTROL_SPEAKER_GAIN_TEST           10
#define SPIN_CONTROL_SPEAKER_CONTINUES_TEST      11
#define SPIN_CONTROL_SPEAKER_CONTINUES_TEST_POS  12

#define ACTION_NAV_BACK                          92

CGUIDialogSpeakerGain::CGUIDialogSpeakerGain(unsigned int streamId)
    : m_StreamId(streamId)
    , m_GainTestSound(SOUND_TEST_OFF)
    , m_window(NULL)
    , m_spinSpeakerGainTest(NULL)
    , m_radioSpeakerContinuesTest(NULL)
{
  m_window              = GUI->Window_create("DialogSpeakerGain.xml", "skin.estuary", false, true);
  m_window->m_cbhdl     = this;
  m_window->CBOnInit    = OnInitCB;
  m_window->CBOnFocus   = OnFocusCB;
  m_window->CBOnClick   = OnClickCB;
  m_window->CBOnAction  = OnActionCB;
}

CGUIDialogSpeakerGain::~CGUIDialogSpeakerGain()
{
  GUI->Window_destroy(m_window);
}

bool CGUIDialogSpeakerGain::OnInitCB(GUIHANDLE cbhdl)
{
  CGUIDialogSpeakerGain* dialog = static_cast<CGUIDialogSpeakerGain*>(cbhdl);
  return dialog->OnInit();
}

bool CGUIDialogSpeakerGain::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogSpeakerGain* dialog = static_cast<CGUIDialogSpeakerGain*>(cbhdl);
  return dialog->OnClick(controlId);
}

bool CGUIDialogSpeakerGain::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogSpeakerGain* dialog = static_cast<CGUIDialogSpeakerGain*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CGUIDialogSpeakerGain::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  CGUIDialogSpeakerGain* dialog = static_cast<CGUIDialogSpeakerGain*>(cbhdl);
  return dialog->OnAction(actionId);
}

void CGUIDialogSpeakerGain::ContinuesTestSwitchInfoCB(AE_DSP_CHANNEL channel)
{
  m_window->SetControlLabel(SPIN_CONTROL_SPEAKER_CONTINUES_TEST_POS, KODI->GetLocalizedString(TranslateChannelIdToStringId(channel)));
}

bool CGUIDialogSpeakerGain::Show()
{
  if (m_window)
    return m_window->Show();

  return false;
}

void CGUIDialogSpeakerGain::Close()
{
  if (m_window)
    m_window->Close();
}

void CGUIDialogSpeakerGain::DoModal()
{
  if (m_window)
    m_window->DoModal();
}

void CGUIDialogSpeakerGain::SetVolumeSpin(int id, AE_DSP_CHANNEL channel, bool present)
{
  m_Settings.m_channels[channel].ptrSpinControl = GUI->Control_getSpin(m_window, id);
  m_Settings.m_channels[channel].ptrSpinControl->Clear();

  if (present)
  {
    CStdString label;
    for (int i = SPEAKER_GAIN_RANGE_DB_MIN; i <= SPEAKER_GAIN_RANGE_DB_MAX; ++i)
    {
      label.Format("%+i dB", i);
      m_Settings.m_channels[channel].ptrSpinControl->AddLabel(label.c_str(), i);
    }

    m_Settings.m_channels[channel].ptrSpinControl->SetValue(m_Settings.m_channels[channel].iVolumeCorrection);
  }
  m_Settings.m_channels[channel].ptrSpinControl->SetVisible(present);
}

void CGUIDialogSpeakerGain::SetVolumeSpins()
{
  unsigned long presentFlags = g_DSPProcessor.GetOutChannelPresentFlags();
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_FL,   AE_DSP_CH_FL,   presentFlags & AE_DSP_PRSNT_CH_FL);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_FR,   AE_DSP_CH_FR,   presentFlags & AE_DSP_PRSNT_CH_FR);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_FC,   AE_DSP_CH_FC,   presentFlags & AE_DSP_PRSNT_CH_FC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_LFE,  AE_DSP_CH_LFE,  presentFlags & AE_DSP_PRSNT_CH_LFE);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_BL,   AE_DSP_CH_BL,   presentFlags & AE_DSP_PRSNT_CH_BL);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_BR,   AE_DSP_CH_BR,   presentFlags & AE_DSP_PRSNT_CH_BR);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_FLOC, AE_DSP_CH_FLOC, presentFlags & AE_DSP_PRSNT_CH_FLOC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_FROC, AE_DSP_CH_FROC, presentFlags & AE_DSP_PRSNT_CH_FROC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_BC,   AE_DSP_CH_BC,   presentFlags & AE_DSP_PRSNT_CH_BC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_SL,   AE_DSP_CH_SL,   presentFlags & AE_DSP_PRSNT_CH_SL);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_SR,   AE_DSP_CH_SR,   presentFlags & AE_DSP_PRSNT_CH_SR);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_TFL,  AE_DSP_CH_TFL,  presentFlags & AE_DSP_PRSNT_CH_TFL);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_TFR,  AE_DSP_CH_TFR,  presentFlags & AE_DSP_PRSNT_CH_TFR);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_TFC,  AE_DSP_CH_TFC,  presentFlags & AE_DSP_PRSNT_CH_TFC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_TC,   AE_DSP_CH_TC,   presentFlags & AE_DSP_PRSNT_CH_TC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_TBL,  AE_DSP_CH_TBL,  presentFlags & AE_DSP_PRSNT_CH_TBL);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_TBR,  AE_DSP_CH_TBR,  presentFlags & AE_DSP_PRSNT_CH_TBR);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_TBC,  AE_DSP_CH_TBC,  presentFlags & AE_DSP_PRSNT_CH_TBC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_BLOC, AE_DSP_CH_BLOC, presentFlags & AE_DSP_PRSNT_CH_BLOC);
  SetVolumeSpin(SPIN_CONTROL_SPEAKER_CH_BROC, AE_DSP_CH_BROC, presentFlags & AE_DSP_PRSNT_CH_BROC);
}

bool CGUIDialogSpeakerGain::OnInit()
{
  LoadSettingsData(ID_MENU_SPEAKER_GAIN_SETUP);

  m_spinSpeakerGainTest = GUI->Control_getSpin(m_window, SPIN_CONTROL_SPEAKER_GAIN_TEST);
  m_spinSpeakerGainTest->Clear();
  m_spinSpeakerGainTest->AddLabel(KODI->GetLocalizedString(30049), SOUND_TEST_OFF);
  m_spinSpeakerGainTest->AddLabel(KODI->GetLocalizedString(30050), SOUND_TEST_PINK_NOICE);
  m_spinSpeakerGainTest->AddLabel(KODI->GetLocalizedString(30051), SOUND_TEST_VOICE);

  m_radioSpeakerContinuesTest = GUI->Control_getRadioButton(m_window, SPIN_CONTROL_SPEAKER_CONTINUES_TEST);
  m_radioSpeakerContinuesTest->SetSelected(false);
  m_radioSpeakerContinuesTest->SetVisible(false);
  m_radioSpeakerContinuesTest->SetText(KODI->GetLocalizedString(30065));

  m_window->SetControlLabel(SPIN_CONTROL_SPEAKER_CONTINUES_TEST_POS, "");

  SetVolumeSpins();

  return true;
}

bool CGUIDialogSpeakerGain::OnClick(int controlId)
{
  AE_DSP_CHANNEL channelId = TranslateGUIIdToChannelId(controlId);
  if (channelId != AE_DSP_CH_MAX)
  {
    g_DSPProcessor.SetOutputGain(channelId, m_Settings.m_channels[channelId].ptrSpinControl->GetValue());
  }
  else
  {
    switch (controlId)
    {
      case SPIN_CONTROL_SPEAKER_CONTINUES_TEST:
      {
        m_window->SetControlLabel(SPIN_CONTROL_SPEAKER_CONTINUES_TEST_POS, "");
        g_DSPProcessor.SetTestSound(AE_DSP_CH_FL, m_GainTestSound, this, m_radioSpeakerContinuesTest->IsSelected());
        break;
      }
      case SPIN_CONTROL_SPEAKER_GAIN_TEST:
        m_GainTestSound = m_spinSpeakerGainTest->GetValue();
        if (m_GainTestSound == SOUND_TEST_OFF)
        {
          m_radioSpeakerContinuesTest->SetSelected(false);
          m_radioSpeakerContinuesTest->SetVisible(false);
          g_DSPProcessor.SetTestSound(AE_DSP_CH_MAX, SOUND_TEST_OFF);
          m_window->SetControlLabel(SPIN_CONTROL_SPEAKER_CONTINUES_TEST_POS, "");
        }
        else
          m_radioSpeakerContinuesTest->SetVisible(true);
        break;
      case BUTTON_CANCEL:
      {
        g_DSPProcessor.SetTestSound(AE_DSP_CH_MAX, SOUND_TEST_OFF);
        m_window->Close();
        GUI->Control_releaseSpin(m_spinSpeakerGainTest);
        GUI->Control_releaseRadioButton(m_radioSpeakerContinuesTest);
        for (int i = 0; i < AE_DSP_CH_MAX; ++i)
        {
          if (m_Settings.m_channels[i].ptrSpinControl)
          {
            if (m_Settings.m_channels[i].ptrSpinControl->GetValue() != m_Settings.m_channels[i].iOldVolumeCorrection)
              g_DSPProcessor.SetOutputGain((AE_DSP_CHANNEL)i, m_Settings.m_channels[i].iOldVolumeCorrection);
            GUI->Control_releaseSpin(m_Settings.m_channels[i].ptrSpinControl);
          }
        }
        break;
      }
      case  BUTTON_OK:
      {
        g_DSPProcessor.SetTestSound(AE_DSP_CH_MAX, SOUND_TEST_OFF);
        m_window->Close();
        GUI->Control_releaseSpin(m_spinSpeakerGainTest);
        GUI->Control_releaseRadioButton(m_radioSpeakerContinuesTest);
        for (int i = 0; i < AE_DSP_CH_MAX; ++i)
        {
          if (m_Settings.m_channels[i].ptrSpinControl)
          {
            m_Settings.m_channels[i].iVolumeCorrection = m_Settings.m_channels[i].ptrSpinControl->GetValue();
            GUI->Control_releaseSpin(m_Settings.m_channels[i].ptrSpinControl);
          }
        }
        SaveSettingsData();
        break;
      }
      default:
        break;
    }
  }

  return true;
}

bool CGUIDialogSpeakerGain::OnFocus(int controlId)
{
  if (m_GainTestSound != SOUND_TEST_OFF && !m_radioSpeakerContinuesTest->IsSelected())
  {
    AE_DSP_CHANNEL channelId = TranslateGUIIdToChannelId(controlId);
    if (channelId != AE_DSP_CH_MAX)
      g_DSPProcessor.SetTestSound(channelId, m_GainTestSound);
    else
      g_DSPProcessor.SetTestSound(AE_DSP_CH_MAX, SOUND_TEST_OFF);
  }
  return true;
}

bool CGUIDialogSpeakerGain::OnAction(int actionId)
{
  if (actionId == ADDON_ACTION_CLOSE_DIALOG  ||
      actionId == ADDON_ACTION_PREVIOUS_MENU ||
      actionId == ACTION_NAV_BACK)
    return OnClick(BUTTON_CANCEL);
  else
    return false;
}
