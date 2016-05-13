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

#include "GUIDialogSpeakerDistance.h"
#include "AudioDSPSoundTest.h"

using namespace std;
using namespace ADDON;

#define BUTTON_OK                                 1
#define BUTTON_CANCEL                             2

#define SPIN_CONTROL_SPEAKER_DISTANCE_UNIT       10
#define SPIN_CONTROL_SPEAKER_DISTANCE_UNIT_INFO  12

#define ACTION_NAV_BACK                          92

#define DELAY_UNIT_SECONDS                        0
#define DELAY_UNIT_MILLISECONDS                   1
#define DELAY_UNIT_METER                          2
#define DELAY_UNIT_MILLIMETER                     3
#define DELAY_UNIT_FEET                           4
#define DELAY_UNIT_INCHES                         5

CGUIDialogSpeakerDistance::CGUIDialogSpeakerDistance(unsigned int streamId)
    : m_StreamId(streamId)
    , m_window(NULL)
    , m_spinSpeakerDistanceUnit(NULL)
{
  m_window              = GUI->Window_create("DialogSpeakerDistance.xml", "skin.estuary", false, true);
  m_window->m_cbhdl     = this;
  m_window->CBOnInit    = OnInitCB;
  m_window->CBOnFocus   = OnFocusCB;
  m_window->CBOnClick   = OnClickCB;
  m_window->CBOnAction  = OnActionCB;
  m_DelayUnit           = DELAY_UNIT_METER;
}

CGUIDialogSpeakerDistance::~CGUIDialogSpeakerDistance()
{
  GUI->Window_destroy(m_window);
}

bool CGUIDialogSpeakerDistance::OnInitCB(GUIHANDLE cbhdl)
{
  CGUIDialogSpeakerDistance* dialog = static_cast<CGUIDialogSpeakerDistance*>(cbhdl);
  return dialog->OnInit();
}

bool CGUIDialogSpeakerDistance::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogSpeakerDistance* dialog = static_cast<CGUIDialogSpeakerDistance*>(cbhdl);
  return dialog->OnClick(controlId);
}

bool CGUIDialogSpeakerDistance::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogSpeakerDistance* dialog = static_cast<CGUIDialogSpeakerDistance*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CGUIDialogSpeakerDistance::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  CGUIDialogSpeakerDistance* dialog = static_cast<CGUIDialogSpeakerDistance*>(cbhdl);
  return dialog->OnAction(actionId);
}

bool CGUIDialogSpeakerDistance::Show()
{
  if (m_window)
    return m_window->Show();

  return false;
}

void CGUIDialogSpeakerDistance::Close()
{
  if (m_window)
    m_window->Close();
}

void CGUIDialogSpeakerDistance::DoModal()
{
  if (m_window)
    m_window->DoModal();
}

CStdString CGUIDialogSpeakerDistance::GetDistanceLabel(int Format, int ms)
{
  CStdString label;
  switch (Format)
  {
    case DELAY_UNIT_MILLISECONDS: label.Format("%.0f ms", (float)(ms*1000/DELAY_RESOLUTION));                                    break;
    case DELAY_UNIT_METER:        label.Format("%.1f m",  (float)(ms*SPEED_OF_SOUND*1000/DELAY_RESOLUTION/1000));                break;
    case DELAY_UNIT_MILLIMETER:   label.Format("%.0f mm", (float)(ms*SPEED_OF_SOUND*1000/DELAY_RESOLUTION));                     break;
    case DELAY_UNIT_FEET:         label.Format("%.1f ft", (float)(ms*SPEED_OF_SOUND*METER_TO_FEETS*100/DELAY_RESOLUTION/100));   break;
    case DELAY_UNIT_INCHES:       label.Format("%.0f in", (float)(ms*SPEED_OF_SOUND*METER_TO_INCHES*100/DELAY_RESOLUTION/100));  break;
    case DELAY_UNIT_SECONDS:
    default:                      label.Format("%.3f s",  (float)(((float)ms)*1000/DELAY_RESOLUTION/1000));                      break;
  }
  return label;
}

void CGUIDialogSpeakerDistance::SetDistanceSpin(int id, AE_DSP_CHANNEL channel, int Format, bool present)
{
  m_Settings.m_channels[channel].ptrSpinControl = GUI->Control_getSpin(m_window, id);
  m_Settings.m_channels[channel].ptrSpinControl->Clear();

  if (present)
  {
    CStdString label;
    for (unsigned int ms = 0; ms <= M_TO_DELAY(MAX_SPEAKER_DISTANCE_METER+0.25); ms += M_TO_DELAY(0.5))
    {
      m_Settings.m_channels[channel].ptrSpinControl->AddLabel(GetDistanceLabel(Format, ms).c_str(), ms);
    }

    m_Settings.m_channels[channel].ptrSpinControl->SetValue(m_Settings.m_channels[channel].iDistanceCorrection);
  }
  m_Settings.m_channels[channel].ptrSpinControl->SetVisible(present);
}

void CGUIDialogSpeakerDistance::SetDistanceSpins(int Format)
{
  unsigned long presentFlags = g_DSPProcessor.GetOutChannelPresentFlags();
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_FL,   AE_DSP_CH_FL,   Format, presentFlags & AE_DSP_PRSNT_CH_FL);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_FR,   AE_DSP_CH_FR,   Format, presentFlags & AE_DSP_PRSNT_CH_FR);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_FC,   AE_DSP_CH_FC,   Format, presentFlags & AE_DSP_PRSNT_CH_FC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_LFE,  AE_DSP_CH_LFE,  Format, presentFlags & AE_DSP_PRSNT_CH_LFE);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_BL,   AE_DSP_CH_BL,   Format, presentFlags & AE_DSP_PRSNT_CH_BL);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_BR,   AE_DSP_CH_BR,   Format, presentFlags & AE_DSP_PRSNT_CH_BR);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_FLOC, AE_DSP_CH_FLOC, Format, presentFlags & AE_DSP_PRSNT_CH_FLOC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_FROC, AE_DSP_CH_FROC, Format, presentFlags & AE_DSP_PRSNT_CH_FROC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_BC,   AE_DSP_CH_BC,   Format, presentFlags & AE_DSP_PRSNT_CH_BC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_SL,   AE_DSP_CH_SL,   Format, presentFlags & AE_DSP_PRSNT_CH_SL);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_SR,   AE_DSP_CH_SR,   Format, presentFlags & AE_DSP_PRSNT_CH_SR);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_TFL,  AE_DSP_CH_TFL,  Format, presentFlags & AE_DSP_PRSNT_CH_TFL);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_TFR,  AE_DSP_CH_TFR,  Format, presentFlags & AE_DSP_PRSNT_CH_TFR);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_TFC,  AE_DSP_CH_TFC,  Format, presentFlags & AE_DSP_PRSNT_CH_TFC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_TC,   AE_DSP_CH_TC,   Format, presentFlags & AE_DSP_PRSNT_CH_TC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_TBL,  AE_DSP_CH_TBL,  Format, presentFlags & AE_DSP_PRSNT_CH_TBL);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_TBR,  AE_DSP_CH_TBR,  Format, presentFlags & AE_DSP_PRSNT_CH_TBR);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_TBC,  AE_DSP_CH_TBC,  Format, presentFlags & AE_DSP_PRSNT_CH_TBC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_BLOC, AE_DSP_CH_BLOC, Format, presentFlags & AE_DSP_PRSNT_CH_BLOC);
  SetDistanceSpin(SPIN_CONTROL_SPEAKER_CH_BROC, AE_DSP_CH_BROC, Format, presentFlags & AE_DSP_PRSNT_CH_BROC);
}

bool CGUIDialogSpeakerDistance::OnInit()
{
  LoadSettingsData(ID_MENU_SPEAKER_DISTANCE_SETUP);

  m_spinSpeakerDistanceUnit = GUI->Control_getSpin(m_window, SPIN_CONTROL_SPEAKER_DISTANCE_UNIT);
  m_spinSpeakerDistanceUnit->Clear();
  m_spinSpeakerDistanceUnit->AddLabel(KODI->GetLocalizedString(30066), DELAY_UNIT_METER);
  m_spinSpeakerDistanceUnit->AddLabel(KODI->GetLocalizedString(30067), DELAY_UNIT_MILLIMETER);
  m_spinSpeakerDistanceUnit->AddLabel(KODI->GetLocalizedString(30068), DELAY_UNIT_FEET);
  m_spinSpeakerDistanceUnit->AddLabel(KODI->GetLocalizedString(30069), DELAY_UNIT_INCHES);
  m_spinSpeakerDistanceUnit->AddLabel(KODI->GetLocalizedString(30070), DELAY_UNIT_SECONDS);
  m_spinSpeakerDistanceUnit->AddLabel(KODI->GetLocalizedString(30071), DELAY_UNIT_MILLISECONDS);

  m_window->SetControlLabel(SPIN_CONTROL_SPEAKER_DISTANCE_UNIT_INFO, "");

  SetDistanceSpins(DELAY_UNIT_METER);
  return true;
}

bool CGUIDialogSpeakerDistance::OnClick(int controlId)
{
  AE_DSP_CHANNEL channelId = TranslateGUIIdToChannelId(controlId);
  if (channelId != AE_DSP_CH_MAX)
  {
    g_DSPProcessor.SetDelay(channelId, m_Settings.m_channels[channelId].ptrSpinControl->GetValue());
    SetInfoLabel(channelId);
  }
  else
  {
    switch (controlId)
    {
      case SPIN_CONTROL_SPEAKER_DISTANCE_UNIT:
      {
        int unit = m_spinSpeakerDistanceUnit->GetValue();
        SetDistanceSpins(unit);
        break;
      }
      case BUTTON_CANCEL:
      {
        m_window->Close();
        GUI->Control_releaseSpin(m_spinSpeakerDistanceUnit);
        for (int i = 0; i < AE_DSP_CH_MAX; ++i)
        {
          if (m_Settings.m_channels[i].ptrSpinControl)
          {
            if (m_Settings.m_channels[i].ptrSpinControl->GetValue() != m_Settings.m_channels[i].iOldDistanceCorrection)
              g_DSPProcessor.SetDelay((AE_DSP_CHANNEL)i, m_Settings.m_channels[i].iOldDistanceCorrection);
            GUI->Control_releaseSpin(m_Settings.m_channels[i].ptrSpinControl);
          }
        }
        break;
      }
      case BUTTON_OK:
      {
        m_window->Close();
        GUI->Control_releaseSpin(m_spinSpeakerDistanceUnit);
        for (int i = 0; i < AE_DSP_CH_MAX; ++i)
        {
          if (m_Settings.m_channels[i].ptrSpinControl)
          {
            m_Settings.m_channels[i].iDistanceCorrection = m_Settings.m_channels[i].ptrSpinControl->GetValue();
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

bool CGUIDialogSpeakerDistance::OnFocus(int controlId)
{
  AE_DSP_CHANNEL channelId = TranslateGUIIdToChannelId(controlId);
  if (channelId != AE_DSP_CH_MAX)
    SetInfoLabel(channelId);
  else
    m_window->SetControlLabel(SPIN_CONTROL_SPEAKER_DISTANCE_UNIT_INFO, "");

  return true;
}

bool CGUIDialogSpeakerDistance::OnAction(int actionId)
{
  if (actionId == ADDON_ACTION_CLOSE_DIALOG ||
      actionId == ADDON_ACTION_PREVIOUS_MENU ||
      actionId == ACTION_NAV_BACK)
    return OnClick(BUTTON_CANCEL);
  else
    return false;
}

void CGUIDialogSpeakerDistance::SetInfoLabel(AE_DSP_CHANNEL channelId)
{
  int value = m_Settings.m_channels[channelId].ptrSpinControl->GetValue();
  CStdString str;
  str  = KODI->GetLocalizedString(TranslateChannelIdToStringId(channelId));
  str += KODI->GetLocalizedString(30074);
  str += " " + GetDistanceLabel(DELAY_UNIT_METER, value) + ",";
  str += " " + GetDistanceLabel(DELAY_UNIT_FEET, value) + ",";
  str += " " + GetDistanceLabel(DELAY_UNIT_INCHES, value) + ",";
  str += " " + GetDistanceLabel(DELAY_UNIT_SECONDS, value);
  m_window->SetControlLabel(SPIN_CONTROL_SPEAKER_DISTANCE_UNIT_INFO, str.c_str());
}
