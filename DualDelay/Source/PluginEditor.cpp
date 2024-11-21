/*
 ==============================================================================
 This file is part of the IEM plug-in suite.
 Author: Daniel Rudrich
 Copyright (c) 2017 - Institute of Electronic Music and Acoustics (IEM)
 https://iem.at

 The IEM plug-in suite is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 The IEM plug-in suite is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this software.  If not, see <https://www.gnu.org/licenses/>.
 ==============================================================================
 */

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
DualDelayAudioProcessorEditor::DualDelayAudioProcessorEditor (
    DualDelayAudioProcessor& p,
    juce::AudioProcessorValueTreeState& vts) :
    juce::AudioProcessorEditor (&p),
    processor (p),
    valueTreeState (vts),
    footer (p.getOSCParameterInterface())
{
    setLookAndFeel (&globalLaF);

    addAndMakeVisible (&title);
    title.setTitle (juce::String ("Dual"), juce::String ("Delay"));
    title.setFont (globalLaF.robotoBold, globalLaF.robotoLight);
    addAndMakeVisible (&footer);

    cbNormalizationAttachement.reset (
        new ComboBoxAttachment (valueTreeState,
                                "useSN3D",
                                *title.getInputWidgetPtr()->getNormCbPointer()));
    cbOrderAttachement.reset (
        new ComboBoxAttachment (valueTreeState,
                                "orderSetting",
                                *title.getInputWidgetPtr()->getOrderCbPointer()));

    addAndMakeVisible (&SlDryGain);
    SlDryGainAttachment.reset (new SliderAttachment (valueTreeState, "dryGain", SlDryGain));
    SlDryGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlDryGain.setTextValueSuffix (" dB");
    SlDryGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlDryGain.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);

    auto delayMSRange = juce::NormalisableRange<double> (
        60000.0f
            / (valueTreeState.getParameterRange ("delayBPML").end
               * valueTreeState.getParameterRange ("delayMultL").end),
        60000.0f
            / (valueTreeState.getParameterRange ("delayBPML").start
               * valueTreeState.getParameterRange ("delayMultL").start),
        0.1f);

    // =========================== LEFT SIDE ==============================================================

    addAndMakeVisible (&cbLeftTransfromMode);
    cbLeftTransfromMode.setJustificationType (juce::Justification::centred);
    cbLeftTransfromMode.addSectionHeading ("Mode");
    cbLeftTransfromMode.addItem ("Rotate", 1);
    cbLeftTransfromMode.addItem ("Warp", 2);
    cbLeftTransfromModeAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "transformModeL", cbLeftTransfromMode));
    cbLeftTransfromMode.addListener (this);

    bool leftTrMode =
        (valueTreeState.getParameter ("transformModeL")->getValue() > 0.5f) ? true : false;

    SlLeftYaw.setVisible (! leftTrMode);
    addChildComponent (&SlLeftYaw);
    SlLeftYawAttachment.reset (new SliderAttachment (valueTreeState, "yawL", SlLeftYaw));
    SlLeftYaw.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftYaw.setReverse (true);
    SlLeftYaw.setTextValueSuffix (" deg");
    SlLeftYaw.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftYaw.setRotaryParameters (juce::MathConstants<float>::pi,
                                   3 * juce::MathConstants<float>::pi,
                                   false);
    SlLeftYaw.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    SlLeftYaw.addListener (this);

    SlLeftPitch.setVisible (! leftTrMode);
    addChildComponent (&SlLeftPitch);
    SlLeftPitchAttachment.reset (new SliderAttachment (valueTreeState, "pitchL", SlLeftPitch));
    SlLeftPitch.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftPitch.setReverse (true);
    SlLeftPitch.setTextValueSuffix (" deg");
    SlLeftPitch.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftPitch.setRotaryParameters (0.5 * juce::MathConstants<float>::pi,
                                     2.5 * juce::MathConstants<float>::pi,
                                     false);
    SlLeftPitch.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    SlLeftPitch.addListener (this);

    SlLeftRoll.setVisible (! leftTrMode);
    addChildComponent (&SlLeftRoll);
    SlLeftRollAttachment.reset (new SliderAttachment (valueTreeState, "rollL", SlLeftRoll));
    SlLeftRoll.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftRoll.setReverse (false);
    SlLeftRoll.setTextValueSuffix (" deg");
    SlLeftRoll.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftRoll.setRotaryParameters (juce::MathConstants<float>::pi,
                                    3 * juce::MathConstants<float>::pi,
                                    false);
    SlLeftRoll.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    SlLeftRoll.addListener (this);

    cbLeftWarpTypeAz.setVisible (leftTrMode);
    addChildComponent (&cbLeftWarpTypeAz);
    cbLeftWarpTypeAz.setJustificationType (juce::Justification::centred);
    cbLeftWarpTypeAz.addSectionHeading ("Warp type azimuth");
    cbLeftWarpTypeAz.addItem ("One Point", 1);
    cbLeftWarpTypeAz.addItem ("Two Point", 2);
    cbLeftWarpTypeAzAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "warpModeAzL", cbLeftWarpTypeAz));

    cbLeftWarpTypeEl.setVisible (leftTrMode);
    addChildComponent (&cbLeftWarpTypeEl);
    cbLeftWarpTypeEl.setJustificationType (juce::Justification::centred);
    cbLeftWarpTypeEl.addSectionHeading ("Warp type elevation");
    cbLeftWarpTypeEl.addItem ("Pole", 1);
    cbLeftWarpTypeEl.addItem ("Equator", 2);
    cbLeftWarpTypeElAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "warpModeElL", cbLeftWarpTypeEl));

    SlLeftWarpFactorAz.setVisible (leftTrMode);
    addChildComponent (&SlLeftWarpFactorAz);
    SlLeftWarpFactorAzAttachment.reset (
        new SliderAttachment (valueTreeState, "warpFactorAzL", SlLeftWarpFactorAz));
    SlLeftWarpFactorAz.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftWarpFactorAz.setTextValueSuffix ("");
    SlLeftWarpFactorAz.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    // SlLeftWarpFactorAz.setRotaryParameters (juce::MathConstants<float>::pi,
    //                                         3 * juce::MathConstants<float>::pi,
    //                                         false);
    SlLeftWarpFactorAz.setColour (juce::Slider::rotarySliderOutlineColourId,
                                  globalLaF.ClWidgetColours[0]);

    SlLeftWarpFactorEl.setVisible (leftTrMode);
    addChildComponent (&SlLeftWarpFactorEl);
    SlLeftWarpFactorElAttachment.reset (
        new SliderAttachment (valueTreeState, "warpFactorElL", SlLeftWarpFactorEl));
    SlLeftWarpFactorEl.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftWarpFactorEl.setTextValueSuffix ("");
    SlLeftWarpFactorEl.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    // SlLeftWarpFactorEl.setRotaryParameters (juce::MathConstants<float>::pi,
    //                                         3 * juce::MathConstants<float>::pi,
    //                                         false);
    SlLeftWarpFactorEl.setColour (juce::Slider::rotarySliderOutlineColourId,
                                  globalLaF.ClWidgetColours[0]);

    addAndMakeVisible (&SlLeftDelay);
    SlLeftDelayAttachment.reset (new SliderAttachment (valueTreeState, "delayBPML", SlLeftDelay));
    SlLeftDelay.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftDelay.setTextValueSuffix (" BPM");
    SlLeftDelay.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftDelay.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[1]);
    SlLeftDelay.addListener (this);

    SlLeftDelayMS.setVisible (false);
    addChildComponent (&SlLeftDelayMS);
    SlLeftDelayMS.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftDelayMS.setNormalisableRange (delayMSRange);
    SlLeftDelayMS.setTextValueSuffix (" ms");
    SlLeftDelayMS.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftDelayMS.setColour (juce::Slider::rotarySliderOutlineColourId,
                             globalLaF.ClWidgetColours[1]);
    SlLeftDelayMS.setValue ((60000.0f / *valueTreeState.getRawParameterValue ("delayBPML"))
                            / *valueTreeState.getRawParameterValue ("delayMultL"));
    SlLeftDelayMS.addListener (this);

    addAndMakeVisible (&tbLeftSync);
    btLeftSyncAttachment.reset (new ButtonAttachment (valueTreeState, "syncL", tbLeftSync));
    tbLeftSync.setButtonText ("Sync");
    tbLeftSync.setColour (juce::ToggleButton::tickColourId, globalLaF.ClWidgetColours[1]);
    tbLeftSync.addListener (this);

    addAndMakeVisible (&btLeftTap);
    btLeftTap.setButtonText ("Tap");
    btLeftTap.setColour (juce::TextButton::buttonColourId, globalLaF.ClWidgetColours[1]);
    btLeftTap.addListener (this);

    addAndMakeVisible (&SlLeftDelayMult);
    SlLeftDelayAttachment.reset (
        new SliderAttachment (valueTreeState, "delayMultL", SlLeftDelayMult));
    SlLeftDelayMult.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftDelayMult.setTextValueSuffix ("");
    SlLeftDelayMult.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftDelayMult.setColour (juce::Slider::rotarySliderOutlineColourId,
                               globalLaF.ClWidgetColours[1]);
    SlLeftDelayMult.addListener (this);

    addAndMakeVisible (&SlLeftLfoRate);
    SlLeftLfoRateAttachment.reset (
        new SliderAttachment (valueTreeState, "lfoRateL", SlLeftLfoRate));
    SlLeftLfoRate.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftLfoRate.setTextValueSuffix (" Hz");
    SlLeftLfoRate.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftLfoRate.setColour (juce::Slider::rotarySliderOutlineColourId,
                             globalLaF.ClWidgetColours[2]);

    addAndMakeVisible (&SlLeftLfoDepth);
    SlLeftLfoDepthAttachment.reset (
        new SliderAttachment (valueTreeState, "lfoDepthL", SlLeftLfoDepth));
    SlLeftLfoDepth.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftLfoDepth.setTextValueSuffix (" ms");
    SlLeftLfoDepth.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftLfoDepth.setColour (juce::Slider::rotarySliderOutlineColourId,
                              globalLaF.ClWidgetColours[2]);

    addAndMakeVisible (&dblSlLeftFilter);
    dblSlLeftFilterHpAttachment.reset (
        new SliderAttachment (valueTreeState,
                              "HPcutOffL",
                              *dblSlLeftFilter.getLeftSliderAddress()));
    dblSlLeftFilterLpAttachment.reset (
        new SliderAttachment (valueTreeState,
                              "LPcutOffL",
                              *dblSlLeftFilter.getRightSliderAddress()));
    dblSlLeftFilter.setRangeAndPosition (valueTreeState.getParameterRange ("HPcutOffL"),
                                         valueTreeState.getParameterRange ("LPcutOffL"));
    dblSlLeftFilter.setColour (globalLaF.ClWidgetColours[1]);

    addAndMakeVisible (&SlLeftFb);
    SlLeftFbAttachment.reset (new SliderAttachment (valueTreeState, "feedbackL", SlLeftFb));
    SlLeftFb.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftFb.setTextValueSuffix (" dB");
    SlLeftFb.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftFb.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[3]);

    addAndMakeVisible (&SlLeftCrossFb);
    SlLeftCrossFbAttachment.reset (
        new SliderAttachment (valueTreeState, "xfeedbackL", SlLeftCrossFb));
    SlLeftCrossFb.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftCrossFb.setTextValueSuffix (" dB");
    SlLeftCrossFb.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftCrossFb.setColour (juce::Slider::rotarySliderOutlineColourId,
                             globalLaF.ClWidgetColours[3]);

    addAndMakeVisible (&SlLeftGain);
    SlLeftGainAttachment.reset (new SliderAttachment (valueTreeState, "wetGainL", SlLeftGain));
    SlLeftGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlLeftGain.setTextValueSuffix (" dB");
    SlLeftGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlLeftGain.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);

    addAndMakeVisible (&btTimeMode);
    btTimeMode.setButtonText ("BPM");
    btTimeMode.setToggleable (true);
    btTimeMode.setClickingTogglesState (true);
    btTimeMode.setColour (juce::TextButton::buttonColourId, globalLaF.ClWidgetColours[1]);
    btTimeMode.setColour (juce::TextButton::buttonOnColourId, globalLaF.ClWidgetColours[1]);
    btTimeMode.addListener (this);

    // =========================== RIGHT SIDE ================================================================

    addAndMakeVisible (&cbRightTransfromMode);
    cbRightTransfromMode.setJustificationType (juce::Justification::centred);
    cbRightTransfromMode.addSectionHeading ("Mode");
    cbRightTransfromMode.addItem ("Rotate", 1);
    cbRightTransfromMode.addItem ("Warp", 2);
    cbRightTransfromModeAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "transformModeR", cbRightTransfromMode));
    cbRightTransfromMode.addListener (this);

    bool rightTrMode =
        (valueTreeState.getParameter ("transformModeR")->getValue() > 0.5f) ? true : false;

    SlRightYaw.setVisible (! rightTrMode);
    addChildComponent (&SlRightYaw);
    SlRightYawAttachment.reset (new SliderAttachment (valueTreeState, "yawR", SlRightYaw));
    SlRightYaw.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightYaw.setReverse (true);
    SlRightYaw.setTextValueSuffix (" deg");
    SlRightYaw.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightYaw.setRotaryParameters (juce::MathConstants<float>::pi,
                                    3 * juce::MathConstants<float>::pi,
                                    false);
    SlRightYaw.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    SlRightYaw.addListener (this);

    SlRightPitch.setVisible (! rightTrMode);
    addChildComponent (&SlRightPitch);
    SlRightPitchAttachment.reset (new SliderAttachment (valueTreeState, "pitchR", SlRightPitch));
    SlRightPitch.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightPitch.setReverse (true);
    SlRightPitch.setTextValueSuffix (" deg");
    SlRightPitch.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightPitch.setRotaryParameters (0.5 * juce::MathConstants<float>::pi,
                                      2.5 * juce::MathConstants<float>::pi,
                                      false);
    SlRightPitch.setColour (juce::Slider::rotarySliderOutlineColourId,
                            globalLaF.ClWidgetColours[0]);
    SlRightPitch.addListener (this);

    SlRightRoll.setVisible (! rightTrMode);
    addChildComponent (&SlRightRoll);
    SlRightRollAttachment.reset (new SliderAttachment (valueTreeState, "rollR", SlRightRoll));
    SlRightRoll.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightRoll.setReverse (false);
    SlRightRoll.setTextValueSuffix (" deg");
    SlRightRoll.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightRoll.setRotaryParameters (juce::MathConstants<float>::pi,
                                     3 * juce::MathConstants<float>::pi,
                                     false);
    SlRightRoll.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    SlRightRoll.addListener (this);

    cbRightWarpTypeAz.setVisible (rightTrMode);
    addChildComponent (&cbRightWarpTypeAz);
    cbRightWarpTypeAz.setJustificationType (juce::Justification::centred);
    cbRightWarpTypeAz.addSectionHeading ("Warp type azimuth");
    cbRightWarpTypeAz.addItem ("One Point", 1);
    cbRightWarpTypeAz.addItem ("Two Point", 2);
    cbRightWarpTypeAzAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "warpModeAzR", cbRightWarpTypeAz));

    cbRightWarpTypeEl.setVisible (rightTrMode);
    addChildComponent (&cbRightWarpTypeEl);
    cbRightWarpTypeEl.setJustificationType (juce::Justification::centred);
    cbRightWarpTypeEl.addSectionHeading ("Warp type elevation");
    cbRightWarpTypeEl.addItem ("Pole", 1);
    cbRightWarpTypeEl.addItem ("Equator", 2);
    cbRightWarpTypeElAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "warpModeElR", cbRightWarpTypeEl));

    SlRightWarpFactorAz.setVisible (rightTrMode);
    addChildComponent (&SlRightWarpFactorAz);
    SlRightWarpFactorAzAttachment.reset (
        new SliderAttachment (valueTreeState, "warpFactorAzR", SlRightWarpFactorAz));
    SlRightWarpFactorAz.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightWarpFactorAz.setTextValueSuffix ("");
    SlRightWarpFactorAz.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    // SlRightWarpFactorAz.setRotaryParameters (juce::MathConstants<float>::pi,
    //                                          3 * juce::MathConstants<float>::pi,
    //                                          false);
    SlRightWarpFactorAz.setColour (juce::Slider::rotarySliderOutlineColourId,
                                   globalLaF.ClWidgetColours[0]);

    SlRightWarpFactorEl.setVisible (rightTrMode);
    addChildComponent (&SlRightWarpFactorEl);
    SlRightWarpFactorElAttachment.reset (
        new SliderAttachment (valueTreeState, "warpFactorElR", SlRightWarpFactorEl));
    SlRightWarpFactorEl.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightWarpFactorEl.setTextValueSuffix ("");
    SlRightWarpFactorEl.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    // SlRightWarpFactorEl.setRotaryParameters (juce::MathConstants<float>::pi,
    //                                          3 * juce::MathConstants<float>::pi,
    //                                          false);
    SlRightWarpFactorEl.setColour (juce::Slider::rotarySliderOutlineColourId,
                                   globalLaF.ClWidgetColours[0]);

    addAndMakeVisible (&SlRightDelay);
    SlRightDelayAttachment.reset (new SliderAttachment (valueTreeState, "delayBPMR", SlRightDelay));
    SlRightDelay.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightDelay.setTextValueSuffix (" BPM");
    SlRightDelay.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightDelay.setColour (juce::Slider::rotarySliderOutlineColourId,
                            globalLaF.ClWidgetColours[1]);
    SlRightDelay.addListener (this);

    SlRightDelayMS.setVisible (false);
    addChildComponent (&SlRightDelayMS);
    SlRightDelayMS.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightDelayMS.setNormalisableRange (delayMSRange);
    SlRightDelayMS.setTextValueSuffix (" ms");
    SlRightDelayMS.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightDelayMS.setColour (juce::Slider::rotarySliderOutlineColourId,
                              globalLaF.ClWidgetColours[1]);
    SlRightDelayMS.setValue ((60000.0f / *valueTreeState.getRawParameterValue ("delayBPMR"))
                             / *valueTreeState.getRawParameterValue ("delayMultR"));
    SlRightDelayMS.addListener (this);

    addAndMakeVisible (&tbRightSync);
    btRightSyncAttachment.reset (new ButtonAttachment (valueTreeState, "syncR", tbRightSync));
    tbRightSync.setButtonText ("Sync");
    tbRightSync.setColour (juce::ToggleButton::tickColourId, globalLaF.ClWidgetColours[1]);
    tbRightSync.addListener (this);

    addAndMakeVisible (&btRightTap);
    btRightTap.setButtonText ("Tap");
    btRightTap.setColour (juce::TextButton::buttonColourId, globalLaF.ClWidgetColours[1]);
    // btLeftTap.setColour (juce::TextButton::buttonOnColourId, globalLaF.ClWidgetColours[1]);
    btRightTap.addListener (this);

    addAndMakeVisible (&SlRightDelayMult);
    SlRightDelayAttachment.reset (
        new SliderAttachment (valueTreeState, "delayMultR", SlRightDelayMult));
    SlRightDelayMult.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightDelayMult.setTextValueSuffix ("");
    SlRightDelayMult.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightDelayMult.setColour (juce::Slider::rotarySliderOutlineColourId,
                                globalLaF.ClWidgetColours[1]);
    SlRightDelayMult.addListener (this);

    addAndMakeVisible (&SlRightLfoRate);
    SlRightLfoRateAttachment.reset (
        new SliderAttachment (valueTreeState, "lfoRateR", SlRightLfoRate));
    SlRightLfoRate.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightLfoRate.setTextValueSuffix (" Hz");
    SlRightLfoRate.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightLfoRate.setColour (juce::Slider::rotarySliderOutlineColourId,
                              globalLaF.ClWidgetColours[2]);

    addAndMakeVisible (&SlRightLfoDepth);
    SlRightLfoDepthAttachment.reset (
        new SliderAttachment (valueTreeState, "lfoDepthR", SlRightLfoDepth));
    SlRightLfoDepth.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightLfoDepth.setTextValueSuffix (" ms");
    SlRightLfoDepth.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightLfoDepth.setColour (juce::Slider::rotarySliderOutlineColourId,
                               globalLaF.ClWidgetColours[2]);

    addAndMakeVisible (&dblSlRightFilter);
    dblSlRightFilterHpAttachment.reset (
        new SliderAttachment (valueTreeState,
                              "HPcutOffR",
                              *dblSlRightFilter.getLeftSliderAddress()));
    dblSlRightFilterLpAttachment.reset (
        new SliderAttachment (valueTreeState,
                              "LPcutOffR",
                              *dblSlRightFilter.getRightSliderAddress()));

    dblSlRightFilter.setRangeAndPosition (valueTreeState.getParameterRange ("HPcutOffR"),
                                          valueTreeState.getParameterRange ("LPcutOffR"));
    dblSlRightFilter.getLeftSliderAddress()->setTextValueSuffix (" Hz");
    dblSlRightFilter.getRightSliderAddress()->setTextValueSuffix (" Hz");
    dblSlRightFilter.setColour (globalLaF.ClWidgetColours[1]);

    addAndMakeVisible (&SlRightFb);
    SlRightFbAttachment.reset (new SliderAttachment (valueTreeState, "feedbackR", SlRightFb));
    SlRightFb.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightFb.setTextValueSuffix (" dB");
    SlRightFb.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightFb.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[3]);

    addAndMakeVisible (&SlRightCrossFb);
    SlRightCrossFbAttachment.reset (
        new SliderAttachment (valueTreeState, "xfeedbackR", SlRightCrossFb));
    SlRightCrossFb.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightCrossFb.setTextValueSuffix (" dB");
    SlRightCrossFb.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightCrossFb.setColour (juce::Slider::rotarySliderOutlineColourId,
                              globalLaF.ClWidgetColours[3]);

    addAndMakeVisible (&SlRightGain);
    SlRightGainAttachment.reset (new SliderAttachment (valueTreeState, "wetGainR", SlRightGain));
    SlRightGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    SlRightGain.setTextValueSuffix (" dB");
    SlRightGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    SlRightGain.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);

    // ============ GROUPCOMPONENTS =========
    addAndMakeVisible (&gcRotDelL);
    gcRotDelL.setText ("Rotation & Delay");
    gcRotDelL.setTextLabelPosition (juce::Justification::centred);

    addAndMakeVisible (&gcRotDelR);
    gcRotDelR.setText ("Rotation & Delay");
    gcRotDelR.setTextLabelPosition (juce::Justification::centred);

    addAndMakeVisible (&gcFiltL);
    gcFiltL.setText ("Spectral Filter");
    gcFiltL.setTextLabelPosition (juce::Justification::centred);

    addAndMakeVisible (&gcFiltR);
    gcFiltR.setText ("Spectral Filter");
    gcFiltR.setTextLabelPosition (juce::Justification::centred);

    addAndMakeVisible (&gcFbL);
    gcFbL.setText ("Feedback");
    gcFbL.setTextLabelPosition (juce::Justification::centred);

    addAndMakeVisible (&gcFbR);
    gcFbR.setText ("Feedback");
    gcFbR.setTextLabelPosition (juce::Justification::centred);

    addAndMakeVisible (&gcOutput);
    gcOutput.setText ("Output Mix");
    gcOutput.setTextLabelPosition (juce::Justification::centred);

    // ============ LABELS =========
    lbYawL.setVisible (! leftTrMode);
    addChildComponent (&lbYawL);
    lbYawL.setText ("Yaw");

    lbPitchL.setVisible (! leftTrMode);
    addChildComponent (&lbPitchL);
    lbPitchL.setText ("Pitch");

    lbRollL.setVisible (! leftTrMode);
    addChildComponent (&lbRollL);
    lbRollL.setText ("Roll");

    addAndMakeVisible (&lbDelL);
    lbDelL.setText ("Delay");

    addAndMakeVisible (&lbDelMultL);
    lbDelMultL.setText ("Mult");

    addAndMakeVisible (&lbFbL);
    lbFbL.setText ("Self");

    addAndMakeVisible (&lbXFbL);
    lbXFbL.setText ("Cross");

    lbWarpFactorAzL.setVisible (leftTrMode);
    addChildComponent (&lbWarpFactorAzL);
    lbWarpFactorAzL.setText ("Warp Az");

    lbWarpFactorElL.setVisible (leftTrMode);
    addChildComponent (&lbWarpFactorElL);
    lbWarpFactorElL.setText ("Warp El");

    lbAzModeL.setVisible (leftTrMode);
    addChildComponent (&lbAzModeL);
    lbAzModeL.setText ("Az:");

    lbElModeL.setVisible (leftTrMode);
    addChildComponent (&lbElModeL);
    lbElModeL.setText ("El:");

    addAndMakeVisible (&lbYawR);
    lbYawR.setText ("Yaw");

    addAndMakeVisible (&lbPitchR);
    lbPitchR.setText ("Pitch");

    addAndMakeVisible (&lbRollR);
    lbRollR.setText ("Roll");

    addAndMakeVisible (&lbDelR);
    lbDelR.setText ("Delay");

    addAndMakeVisible (&lbDelMultR);
    lbDelMultR.setText ("Mult");

    addAndMakeVisible (&lbFbR);
    lbFbR.setText ("Self");

    addAndMakeVisible (&lbXFbR);
    lbXFbR.setText ("Cross");

    lbWarpFactorAzR.setVisible (rightTrMode);
    addChildComponent (&lbWarpFactorAzR);
    lbWarpFactorAzR.setText ("Warp Az");

    lbWarpFactorElR.setVisible (rightTrMode);
    addChildComponent (&lbWarpFactorElR);
    lbWarpFactorElR.setText ("Warp El");

    lbAzModeR.setVisible (rightTrMode);
    addChildComponent (&lbAzModeR);
    lbAzModeR.setText ("Az:");

    lbElModeR.setVisible (rightTrMode);
    addChildComponent (&lbElModeR);
    lbElModeR.setText ("El:");

    addAndMakeVisible (&lbGainL);
    lbGainL.setText ("Delay I");

    addAndMakeVisible (&lbGainR);
    lbGainR.setText ("Delay II");

    addAndMakeVisible (&lbGainDry);
    lbGainDry.setText ("Dry");

    addAndMakeVisible (&lbLfoL);
    lbLfoL.setText ("Rate", "LFO", "Depth", false, true, false);

    addAndMakeVisible (&lbLfoR);
    lbLfoR.setText ("Rate", "LFO", "Depth", false, true, false);

    addAndMakeVisible (&lbFilterL);
    lbFilterL.setText ("HighPass", "Cutoff Frequency", "LowPass", false, true, false);

    addAndMakeVisible (&lbFilterR);
    lbFilterR.setText ("HighPass", "Cutoff Frequency", "LowPass", false, true, false);

    setSize (780, 550);

    startTimer (20);
}

DualDelayAudioProcessorEditor::~DualDelayAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void DualDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (globalLaF.ClBackground);
}

void DualDelayAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    const int leftRightMargin = 30;
    const int headerHeight = 60;
    const int footerHeight = 25;
    const int textHeight = 14;
    const int sliderHeight = 70;
    const int sliderWidth = 55;
    const int sliderSpacing = 10;

    updateDelayUnit (! btTimeMode.getToggleState());

    juce::Rectangle<int> area (getLocalBounds());
    juce::Rectangle<int> groupArea;
    juce::Rectangle<int> sliderRow;

    juce::Rectangle<int> footerArea (area.removeFromBottom (footerHeight));
    footer.setBounds (footerArea);

    area.removeFromLeft (leftRightMargin);
    area.removeFromRight (leftRightMargin);
    juce::Rectangle<int> headerArea = area.removeFromTop (headerHeight);
    title.setBounds (headerArea);
    area.removeFromTop (10);

    juce::Rectangle<int> tempArea;

    // ======== BEGIN: Rotations and Delays =========
    tempArea = area.removeFromTop (40 + 2 * sliderHeight + 2 * textHeight);

    // ------ left side ---------
    groupArea = tempArea.removeFromLeft (5 * sliderWidth + 4 * sliderSpacing);
    gcRotDelL.setBounds (groupArea);
    groupArea.removeFromTop (30);

    // First row
    sliderRow = groupArea.removeFromTop (sliderHeight);
    auto tmp_componentArea = sliderRow.removeFromLeft (sliderWidth);
    btLeftTap.setBounds (tmp_componentArea.removeFromTop (45));
    tmp_componentArea.removeFromTop (5);
    tbLeftSync.setBounds (tmp_componentArea.removeFromTop (20));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    tmp_componentArea = sliderRow.removeFromLeft (sliderWidth);
    SlLeftDelay.setBounds (tmp_componentArea);
    SlLeftDelayMS.setBounds (tmp_componentArea);
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftDelayMult.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftLfoRate.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftLfoDepth.setBounds (sliderRow.removeFromLeft (sliderWidth));

    // First row labels
    sliderRow = groupArea.removeFromTop (textHeight);
    sliderRow.removeFromLeft (sliderWidth + sliderSpacing);
    lbDelL.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbDelMultL.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbLfoL.setBounds (sliderRow.removeFromLeft (2 * sliderWidth + 10).reduced (15, 0));

    // Second row
    groupArea.removeFromTop (sliderSpacing);
    sliderRow = groupArea.removeFromTop (sliderHeight);
    tmp_componentArea = sliderRow.removeFromLeft (2 * sliderWidth + sliderSpacing);
    cbLeftTransfromMode.setBounds (tmp_componentArea.removeFromTop (20));
    tmp_componentArea.removeFromTop (5);
    auto tmp_cbArea = tmp_componentArea.removeFromTop (20);
    lbAzModeL.setBounds (tmp_cbArea.removeFromLeft (15));
    tmp_cbArea.removeFromLeft (5);
    cbLeftWarpTypeAz.setBounds (tmp_cbArea);
    tmp_componentArea.removeFromTop (5);
    tmp_cbArea = tmp_componentArea.removeFromTop (20);
    lbElModeL.setBounds (tmp_cbArea.removeFromLeft (15));
    tmp_cbArea.removeFromLeft (5);
    cbLeftWarpTypeEl.setBounds (tmp_cbArea);
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    tmp_componentArea = sliderRow;
    SlLeftYaw.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftPitch.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftRoll.setBounds (sliderRow);

    tmp_componentArea.reduce ((sliderWidth + sliderSpacing) * 0.5f, 0);
    SlLeftWarpFactorAz.setBounds (tmp_componentArea.removeFromLeft (sliderWidth));
    tmp_componentArea.removeFromLeft (sliderSpacing);
    SlLeftWarpFactorEl.setBounds (tmp_componentArea);

    // Second Row Labels
    sliderRow = groupArea.removeFromTop (textHeight);
    sliderRow.removeFromLeft (2 * sliderWidth + 2 * sliderSpacing);
    tmp_componentArea = sliderRow;
    lbYawL.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbPitchL.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbRollL.setBounds (sliderRow);

    tmp_componentArea.reduce ((sliderWidth + sliderSpacing) * 0.5f, 0);
    lbWarpFactorAzL.setBounds (tmp_componentArea.removeFromLeft (sliderWidth));
    tmp_componentArea.removeFromLeft (sliderSpacing);
    lbWarpFactorElL.setBounds (tmp_componentArea);

    // ------ right side --------
    groupArea = tempArea.removeFromRight (5 * sliderWidth + 4 * sliderSpacing);
    gcRotDelR.setBounds (groupArea);
    groupArea.removeFromTop (30);

    // First row
    sliderRow = groupArea.removeFromTop (sliderHeight);
    tmp_componentArea = sliderRow.removeFromLeft (sliderWidth);
    btRightTap.setBounds (tmp_componentArea.removeFromTop (45));
    tmp_componentArea.removeFromTop (5);
    tbRightSync.setBounds (tmp_componentArea.removeFromTop (20));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    tmp_componentArea = sliderRow.removeFromLeft (sliderWidth);
    SlRightDelay.setBounds (tmp_componentArea);
    SlRightDelayMS.setBounds (tmp_componentArea);
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightDelayMult.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightLfoRate.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightLfoDepth.setBounds (sliderRow.removeFromLeft (sliderWidth));

    // First row labels
    sliderRow = groupArea.removeFromTop (textHeight);
    sliderRow.removeFromLeft (sliderWidth + sliderSpacing);
    lbDelR.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbDelMultR.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbLfoR.setBounds (sliderRow.removeFromLeft (2 * sliderWidth + 10).reduced (15, 0));

    // Second row
    groupArea.removeFromTop (sliderSpacing);
    sliderRow = groupArea.removeFromTop (sliderHeight);
    tmp_componentArea = sliderRow.removeFromLeft (2 * sliderWidth + sliderSpacing);
    cbRightTransfromMode.setBounds (tmp_componentArea.removeFromTop (20));
    tmp_componentArea.removeFromTop (5);
    tmp_cbArea = tmp_componentArea.removeFromTop (20);
    lbAzModeR.setBounds (tmp_cbArea.removeFromLeft (15));
    tmp_cbArea.removeFromLeft (5);
    cbRightWarpTypeAz.setBounds (tmp_cbArea);
    tmp_componentArea.removeFromTop (5);
    tmp_cbArea = tmp_componentArea.removeFromTop (20);
    lbElModeR.setBounds (tmp_cbArea.removeFromLeft (15));
    tmp_cbArea.removeFromLeft (5);
    cbRightWarpTypeEl.setBounds (tmp_cbArea);
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    tmp_componentArea = sliderRow;
    SlRightYaw.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightPitch.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightRoll.setBounds (sliderRow);

    tmp_componentArea.reduce ((sliderWidth + sliderSpacing) * 0.5f, 0);
    SlRightWarpFactorAz.setBounds (tmp_componentArea.removeFromLeft (sliderWidth));
    tmp_componentArea.removeFromLeft (sliderSpacing);
    SlRightWarpFactorEl.setBounds (tmp_componentArea);

    // Second Row Labels
    sliderRow = groupArea.removeFromTop (textHeight);
    sliderRow.removeFromLeft (2 * sliderWidth + 2 * sliderSpacing);
    tmp_componentArea = sliderRow;
    lbYawR.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbPitchR.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbRollR.setBounds (sliderRow);

    tmp_componentArea.reduce ((sliderWidth + sliderSpacing) * 0.5f, 0);
    lbWarpFactorAzR.setBounds (tmp_componentArea.removeFromLeft (sliderWidth));
    tmp_componentArea.removeFromLeft (sliderSpacing);
    lbWarpFactorElR.setBounds (tmp_componentArea);
    // ======== END: Rotations and Delays =================

    // ======== BEGIN: BPM/MS button ===========
    int actualWidth = tempArea.getWidth();
    int wantedWidth = 40;
    tempArea.removeFromLeft (juce::roundToInt ((actualWidth - wantedWidth) / 2));
    tempArea.removeFromTop (25 + textHeight);
    btTimeMode.setBounds (tempArea.removeFromLeft (wantedWidth).removeFromTop (25));
    // ======== BEGIN: BPM/MS button ===========

    area.removeFromTop (30); // spacing

    // ======== BEGIN: Filters =============
    tempArea = area.removeFromTop (60 + textHeight);

    // ----- left side ------
    groupArea = tempArea.removeFromLeft (330);
    gcFiltL.setBounds (groupArea);
    groupArea.removeFromTop (30);

    dblSlLeftFilter.setBounds (groupArea.removeFromTop (30));
    lbFilterL.setBounds (groupArea.reduced (9, 0));

    // ----- right side ------
    groupArea = tempArea.removeFromRight (330);
    gcFiltR.setBounds (groupArea);
    groupArea.removeFromTop (30);

    dblSlRightFilter.setBounds (groupArea.removeFromTop (30));
    lbFilterR.setBounds (groupArea.reduced (9, 0));
    // ======== END: Filters ===============

    area.removeFromTop (30); // spacing

    // ======== BEGIN: Feedback =============

    tempArea = area.removeFromTop (30 + sliderHeight + textHeight);

    // ------ left side -------------
    groupArea = tempArea.removeFromLeft (120);
    gcFbL.setBounds (groupArea);
    groupArea.removeFromTop (30);

    sliderRow = groupArea.removeFromTop (sliderHeight);

    SlLeftFb.setBounds (sliderRow.removeFromLeft (55));
    sliderRow.removeFromLeft (10);
    SlLeftCrossFb.setBounds (sliderRow.removeFromLeft (55));

    lbFbL.setBounds (groupArea.removeFromLeft (55));
    groupArea.removeFromLeft (10);
    lbXFbL.setBounds (groupArea.removeFromLeft (55));

    // ------ right side -------------
    groupArea = tempArea.removeFromRight (120);
    gcFbR.setBounds (groupArea);
    groupArea.removeFromTop (30);

    sliderRow = groupArea.removeFromTop (sliderHeight);

    SlRightFb.setBounds (sliderRow.removeFromLeft (55));
    sliderRow.removeFromLeft (10);
    SlRightCrossFb.setBounds (sliderRow.removeFromLeft (55));

    lbFbR.setBounds (groupArea.removeFromLeft (55));
    groupArea.removeFromLeft (10);
    lbXFbR.setBounds (groupArea.removeFromLeft (55));

    // ======== END: Feedback ===============

    // ======== BEGIN: Output Mix ===========
    actualWidth = tempArea.getWidth();
    wantedWidth = 186;
    tempArea.removeFromLeft (juce::roundToInt ((actualWidth - wantedWidth) / 2));
    tempArea.setWidth (wantedWidth);

    gcOutput.setBounds (tempArea);
    tempArea.removeFromTop (30);

    sliderRow = tempArea.removeFromTop (sliderHeight);

    SlLeftGain.setBounds (sliderRow.removeFromLeft (55));
    sliderRow.removeFromLeft (10); // spacing between rotary sliders
    SlDryGain.setBounds (sliderRow.removeFromLeft (55));
    sliderRow.removeFromLeft (10); // spacing between rotary sliders
    SlRightGain.setBounds (sliderRow.removeFromLeft (55));
    sliderRow.removeFromLeft (10); // spacing between rotary sliders

    lbGainL.setBounds (tempArea.removeFromLeft (55));
    tempArea.removeFromLeft (10); // spacing between rotary sliders
    lbGainDry.setBounds (tempArea.removeFromLeft (55));
    tempArea.removeFromLeft (10); // spacing between rotary sliders
    lbGainR.setBounds (tempArea.removeFromLeft (55));

    // ======== END: Output Mix =============
}

void DualDelayAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if ((button == &btLeftTap) || (button == &btRightTap))
    {
        double currentTap = juce::Time::getMillisecondCounterHiRes();
        double rawTapInterval = currentTap - lastTap;

        if (rawTapInterval > maxTapIntervalMS)
        {
            tapIntervalMS = 0.0;
            lastTap = currentTap;
        }

        else if (rawTapInterval > minTapIntervalMS)
        {
            if (tapIntervalMS > 0.0)
            {
                tapIntervalMS = tapBeta * tapIntervalMS + (1.0 - tapBeta) * rawTapInterval;
            }
            else
            {
                tapIntervalMS = rawTapInterval;
            }

            auto [bpm, mult] = processor.msToBPM (tapIntervalMS);

            if (button == &btRightTap)
            {
                SlRightDelayMS.setValue (tapIntervalMS, juce::dontSendNotification);
                SlRightDelay.setValue (bpm, juce::sendNotification);
                SlRightDelayMult.setValue (mult, juce::sendNotification);
            }
            else
            {
                SlLeftDelayMS.setValue (tapIntervalMS, juce::dontSendNotification);
                SlLeftDelay.setValue (bpm, juce::sendNotification);
                SlLeftDelayMult.setValue (mult, juce::sendNotification);
            }

            lastTap = currentTap;
        }
    }
    else if (button == &btTimeMode)
    {
        updateDelayUnit (! btTimeMode.getToggleState());
    }

    if (button == &tbLeftSync)
    {
        if (tbLeftSync.getToggleState())
        {
            SlLeftDelayMS.setVisible (false);
            SlLeftDelay.setVisible (false);
            SlLeftDelayMult.setVisible (true);
            lbDelL.setVisible (false);
        }
        else
        {
            SlLeftDelayMS.setVisible (false);
            SlLeftDelay.setVisible (true);
            SlLeftDelayMult.setVisible (true);
            lbDelL.setVisible (true);
        }
    }
    else if (button == &tbRightSync)
    {
        if (tbRightSync.getToggleState())
        {
            SlRightDelayMS.setVisible (false);
            SlRightDelay.setVisible (false);
            SlRightDelayMult.setVisible (true);
            lbDelR.setVisible (false);
        }
        else
        {
            SlRightDelayMS.setVisible (false);
            SlRightDelay.setVisible (true);
            SlRightDelayMult.setVisible (true);
            lbDelR.setVisible (true);
        }
    }
}

void DualDelayAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &SlLeftDelayMS)
    {
        if (~btTimeMode.getToggleState())
        {
            auto [bpm, mult] = processor.msToBPM (SlLeftDelayMS.getValue());
            SlLeftDelay.setValue (bpm, juce::sendNotification);
            SlLeftDelayMult.setValue (mult, juce::sendNotification);
        }
    }
    else if (slider == &SlRightDelayMS)
    {
        if (~btTimeMode.getToggleState())
        {
            auto [bpm, mult] = processor.msToBPM (SlLeftDelayMS.getValue());
            SlLeftDelay.setValue (bpm, juce::sendNotification);
            SlLeftDelayMult.setValue (mult, juce::sendNotification);
        }
    }
    else if (slider == &SlLeftDelay)
    {
        float ms = (60000.0f / SlLeftDelay.getValue())
                   / *valueTreeState.getRawParameterValue ("delayMultL");
        SlLeftDelayMS.setValue (ms, juce::dontSendNotification);
    }
    else if (slider == &SlRightDelay)
    {
        float ms = (60000.0f / SlRightDelay.getValue())
                   / *valueTreeState.getRawParameterValue ("delayMultR");
        SlRightDelayMS.setValue (ms, juce::dontSendNotification);
    }
    else if (slider == &SlLeftDelayMult)
    {
        float ms = (60000.0f / SlLeftDelay.getValue()) / slider->getValue();
        SlLeftDelayMS.setValue (ms, juce::dontSendNotification);
    }
    else if (slider == &SlRightDelayMult)
    {
        float ms = (60000.0f / SlRightDelay.getValue()) / slider->getValue();
        SlRightDelayMS.setValue (ms, juce::dontSendNotification);
    }
}

void DualDelayAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox == &cbLeftTransfromMode)
        updateTransformMode (0);
    else if (comboBox == &cbRightTransfromMode)
        updateTransformMode (1);
}

void DualDelayAudioProcessorEditor::updateDelayUnit (bool isBPM)
{
    if (! tbLeftSync.getToggleState())
    {
        SlLeftDelayMS.setVisible (! isBPM);
        SlLeftDelay.setVisible (isBPM);
        SlLeftDelayMult.setVisible (isBPM);
        lbDelMultL.setVisible (isBPM);
    }
    if (! tbRightSync.getToggleState())
    {
        SlRightDelayMS.setVisible (! isBPM);
        SlRightDelay.setVisible (isBPM);
        SlRightDelayMult.setVisible (isBPM);
        lbDelMultR.setVisible (isBPM);
    }
    btTimeMode.setButtonText (isBPM ? "BPM" : "ms");
}

void DualDelayAudioProcessorEditor::updateTransformMode (bool side)
{
    if (side)
    {
        bool rightTrMode = cbRightTransfromMode.getSelectedId() == 1;
        SlRightYaw.setVisible (rightTrMode);
        SlRightPitch.setVisible (rightTrMode);
        SlRightRoll.setVisible (rightTrMode);
        lbYawR.setVisible (rightTrMode);
        lbPitchR.setVisible (rightTrMode);
        lbRollR.setVisible (rightTrMode);

        cbRightWarpTypeAz.setVisible (! rightTrMode);
        cbRightWarpTypeEl.setVisible (! rightTrMode);
        SlRightWarpFactorAz.setVisible (! rightTrMode);
        SlRightWarpFactorEl.setVisible (! rightTrMode);
        lbWarpFactorAzR.setVisible (! rightTrMode);
        lbWarpFactorElR.setVisible (! rightTrMode);
        lbAzModeR.setVisible (! rightTrMode);
        lbElModeR.setVisible (! rightTrMode);
    }
    else
    {
        bool leftTrMode = cbLeftTransfromMode.getSelectedId() == 1;
        SlLeftYaw.setVisible (leftTrMode);
        SlLeftPitch.setVisible (leftTrMode);
        SlLeftRoll.setVisible (leftTrMode);
        lbYawL.setVisible (leftTrMode);
        lbPitchL.setVisible (leftTrMode);
        lbRollL.setVisible (leftTrMode);

        cbLeftWarpTypeAz.setVisible (! leftTrMode);
        cbLeftWarpTypeEl.setVisible (! leftTrMode);
        SlLeftWarpFactorAz.setVisible (! leftTrMode);
        SlLeftWarpFactorEl.setVisible (! leftTrMode);
        lbWarpFactorAzL.setVisible (! leftTrMode);
        lbWarpFactorElL.setVisible (! leftTrMode);
        lbAzModeL.setVisible (! leftTrMode);
        lbElModeL.setVisible (! leftTrMode);
    }
}

void DualDelayAudioProcessorEditor::timerCallback()
{
    // === update titleBar widgets according to available input/output channel counts
    auto sizes = processor.getMaxSize();
    sizes.first = juce::jmin (sizes.first, sizes.second);
    sizes.second = sizes.first;
    title.setMaxSize (sizes);
    // ==========================================
}
