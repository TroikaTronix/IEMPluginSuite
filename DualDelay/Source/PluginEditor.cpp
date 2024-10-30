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

    addAndMakeVisible (&SlLeftYaw);
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

    addAndMakeVisible (&SlLeftPitch);
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

    addAndMakeVisible (&SlLeftRoll);
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

    addAndMakeVisible (&cbLeftDelayMult);
    cbLeftDelayMult.setJustificationType (juce::Justification::centred);
    cbLeftDelayMult.addSectionHeading ("Note value");
    cbLeftDelayMult.addItem ("1/2", 1);
    cbLeftDelayMult.addItem ("1/4", 2);
    cbLeftDelayMult.addItem ("1/8", 3);
    cbLeftDelayMult.addItem ("1/16", 4);
    cbLeftDelayMult.addItem ("1/32", 5);
    cbLeftDelayMult.addItem ("1/64", 6);
    cbLeftDelayMultAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "delayMultL", cbLeftDelayMult));
    cbLeftDelayMult.addListener (this);

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

    addAndMakeVisible (&SlRightYaw);
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

    addAndMakeVisible (&SlRightPitch);
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

    addAndMakeVisible (&SlRightRoll);
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

    addAndMakeVisible (&cbRightDelayMult);
    cbRightDelayMult.setJustificationType (juce::Justification::centred);
    cbRightDelayMult.addSectionHeading ("Note value");
    cbRightDelayMult.addItem ("1/2", 1);
    cbRightDelayMult.addItem ("1/4", 2);
    cbRightDelayMult.addItem ("1/8", 3);
    cbRightDelayMult.addItem ("1/16", 4);
    cbRightDelayMult.addItem ("1/32", 5);
    cbRightDelayMult.addItem ("1/64", 6);
    cbRightDelayMultAttachment.reset (
        new ComboBoxAttachment (valueTreeState, "delayMultR", cbRightDelayMult));
    cbRightDelayMult.addListener (this);

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
    addAndMakeVisible (&lbYawL);
    lbYawL.setText ("Yaw");

    addAndMakeVisible (&lbPitchL);
    lbPitchL.setText ("Pitch");

    addAndMakeVisible (&lbRollL);
    lbRollL.setText ("Roll");

    addAndMakeVisible (&lbDelL);
    lbDelL.setText ("Delay");

    addAndMakeVisible (&lbFbL);
    lbFbL.setText ("Self");

    addAndMakeVisible (&lbXFbL);
    lbXFbL.setText ("Cross");

    addAndMakeVisible (&lbYawR);
    lbYawR.setText ("Yaw");

    addAndMakeVisible (&lbPitchR);
    lbPitchR.setText ("Pitch");

    addAndMakeVisible (&lbRollR);
    lbRollR.setText ("Roll");

    addAndMakeVisible (&lbDelR);
    lbDelR.setText ("Delay");

    addAndMakeVisible (&lbFbR);
    lbFbR.setText ("Self");

    addAndMakeVisible (&lbXFbR);
    lbXFbR.setText ("Cross");

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

    setSize (630, 550);

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
    groupArea = tempArea.removeFromLeft (4 * sliderWidth + 3 * sliderSpacing);
    gcRotDelL.setBounds (groupArea);
    groupArea.removeFromTop (30);

    // juce::Rectangle<int> sliderRowLeft = groupArea.removeFromTop (sliderHeight * 2 + 10);
    sliderRow = groupArea.removeFromTop (sliderHeight);
    SlLeftYaw.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftPitch.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftRoll.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftDelay.setBounds (sliderRow);
    SlLeftDelayMS.setBounds (sliderRow);

    sliderRow = groupArea.removeFromTop (textHeight); // spacing between rotary sliders
    lbYawL.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbPitchL.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbRollL.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbDelL.setBounds (sliderRow.removeFromLeft (sliderWidth));

    groupArea.removeFromTop (sliderSpacing);
    sliderRow = groupArea.removeFromTop (sliderHeight);
    SlLeftLfoRate.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlLeftLfoDepth.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    btLeftTap.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    cbLeftDelayMult.setBounds (sliderRow.removeFromTop (25));
    sliderRow.removeFromTop (5);
    tbLeftSync.setBounds (sliderRow.removeFromTop (20));

    sliderRow = groupArea.removeFromTop (textHeight);
    lbLfoL.setBounds (sliderRow.removeFromLeft (2 * sliderWidth + 10).reduced (15, 0));

    // ------ right side --------
    groupArea = tempArea.removeFromRight (4 * sliderWidth + 3 * sliderSpacing);
    gcRotDelR.setBounds (groupArea);
    groupArea.removeFromTop (30);

    // juce::Rectangle<int> sliderRowLeft = groupArea.removeFromTop (sliderHeight * 2 + 10);
    sliderRow = groupArea.removeFromTop (sliderHeight);
    SlRightYaw.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightPitch.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightRoll.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightDelay.setBounds (sliderRow);
    SlRightDelayMS.setBounds (sliderRow);

    sliderRow = groupArea.removeFromTop (textHeight); // spacing between rotary sliders
    lbYawR.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbPitchR.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbRollR.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing);
    lbDelR.setBounds (sliderRow.removeFromLeft (sliderWidth));

    groupArea.removeFromTop (sliderSpacing);
    sliderRow = groupArea.removeFromTop (sliderHeight);
    SlRightLfoRate.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    SlRightLfoDepth.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    btRightTap.setBounds (sliderRow.removeFromLeft (sliderWidth));
    sliderRow.removeFromLeft (sliderSpacing); // spacing between rotary sliders
    cbRightDelayMult.setBounds (sliderRow.removeFromTop (25));
    sliderRow.removeFromTop (5);
    tbRightSync.setBounds (sliderRow.removeFromTop (25));

    sliderRow = groupArea.removeFromTop (textHeight);
    lbLfoR.setBounds (sliderRow.removeFromLeft (2 * sliderWidth + 10).reduced (15, 0));
    // ======== END: Rotations and Delays =================

    // ======== BEGIN: BPM/MS button ===========
    int actualWidth = tempArea.getWidth();
    int wantedWidth = 40;
    tempArea.removeFromLeft (juce::roundToInt ((actualWidth - wantedWidth) / 2));
    tempArea.removeFromTop (35 + textHeight);
    btTimeMode.setBounds (tempArea.removeFromLeft (wantedWidth).removeFromTop (20));
    // ======== BEGIN: BPM/MS button ===========

    area.removeFromTop (30); // spacing

    // ======== BEGIN: Filters =============
    tempArea = area.removeFromTop (60 + textHeight);

    // ----- left side ------
    groupArea = tempArea.removeFromLeft (270);
    gcFiltL.setBounds (groupArea);
    groupArea.removeFromTop (30);

    dblSlLeftFilter.setBounds (groupArea.removeFromTop (30));
    lbFilterL.setBounds (groupArea.reduced (9, 0));

    // ----- right side ------
    groupArea = tempArea.removeFromRight (270);
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

                int multIdx = static_cast<int> (std::log2 (
                                  mult / valueTreeState.getParameterRange ("delayMultR").start))
                              + 1;
                SlRightDelay.setValue (bpm, juce::sendNotification);
                cbRightDelayMult.setSelectedId (multIdx, juce::sendNotification);
            }
            else
            {
                SlLeftDelayMS.setValue (tapIntervalMS, juce::dontSendNotification);

                int multIdx = static_cast<int> (std::log2 (
                                  mult / valueTreeState.getParameterRange ("delayMultL").start))
                              + 1;
                SlLeftDelay.setValue (bpm, juce::sendNotification);
                cbLeftDelayMult.setSelectedId (multIdx, juce::sendNotification);
            }

            lastTap = currentTap;
        }
    }
    else if (button == &btTimeMode)
    {
        updateDelayUnit (! btTimeMode.getToggleState());
    }
}

void DualDelayAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &SlLeftDelayMS)
    {
        auto [bpm, mult] = processor.msToBPM (SlLeftDelayMS.getValue());
        int multIdx = static_cast<int> (
                          std::log2 (mult / valueTreeState.getParameterRange ("delayMultL").start))
                      + 1;
        SlLeftDelay.setValue (bpm, juce::sendNotification);
        cbLeftDelayMult.setSelectedId (multIdx, juce::sendNotification);
    }
    else if (slider == &SlRightDelayMS)
    {
        auto [bpm, mult] = processor.msToBPM (SlRightDelayMS.getValue());
        int multIdx = static_cast<int> (
                          std::log2 (mult / valueTreeState.getParameterRange ("delayMultR").start))
                      + 1;
        SlRightDelay.setValue (bpm, juce::sendNotification);
        cbRightDelayMult.setSelectedId (multIdx, juce::sendNotification);
    }
    else if (slider == &SlLeftDelay)
    {
        float ms = (60000.0f / SlLeftDelay.getValue())
                   / *valueTreeState.getRawParameterValue ("delayMultL");
        SlLeftDelayMS.setValue (ms, juce::sendNotification);
    }
    else if (slider == &SlRightDelay)
    {
        float ms = (60000.0f / SlRightDelay.getValue())
                   / *valueTreeState.getRawParameterValue ("delayMultR");
        SlRightDelayMS.setValue (ms, juce::sendNotification);
    }
}

void DualDelayAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox == &cbLeftDelayMult)
    {
        float ms = (60000.0f / SlLeftDelay.getValue())
                   / std::exp2f (comboBox->getItemId (comboBox->getSelectedItemIndex()) - 2);
        SlLeftDelayMS.setValue (ms, juce::sendNotification);
    }
    else if (comboBox == &cbRightDelayMult)
    {
        float ms = (60000.0f / SlRightDelay.getValue())
                   / std::exp2f (comboBox->getItemId (comboBox->getSelectedItemIndex()) - 2);
        SlRightDelayMS.setValue (ms, juce::sendNotification);
    }
}

void DualDelayAudioProcessorEditor::updateDelayUnit (bool isBPM)
{
    SlLeftDelay.setVisible (isBPM);
    SlRightDelay.setVisible (isBPM);
    cbLeftDelayMult.setVisible (isBPM);
    cbRightDelayMult.setVisible (isBPM);
    SlRightDelayMS.setVisible (! isBPM);
    SlLeftDelayMS.setVisible (! isBPM);
    btTimeMode.setButtonText (isBPM ? "BPM" : "ms");
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
