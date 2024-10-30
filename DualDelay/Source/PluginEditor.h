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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

#include "../../resources/customComponents/DoubleSlider.h"
#include "../../resources/customComponents/ReverseSlider.h"
#include "../../resources/customComponents/SimpleLabel.h"
#include "../../resources/customComponents/TitleBar.h"
#include "../../resources/lookAndFeel/IEM_LaF.h"

typedef ReverseSlider::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

//==============================================================================
/**
*/
class DualDelayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer,
                                      private juce::Button::Listener,
                                      private juce::Slider::Listener,
                                      private juce::ComboBox::Listener
{
public:
    DualDelayAudioProcessorEditor (DualDelayAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~DualDelayAudioProcessorEditor();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;
    void sliderValueChanged (juce::Slider* slider) override;
    void comboBoxChanged (juce::ComboBox* comboBox) override;

    void updateDelayUnit (bool isBPM);

private:
    LaF globalLaF;

    void timerCallback() override;

    DualDelayAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& valueTreeState;

    TitleBar<AmbisonicIOWidget<>, NoIOWidget> title;
    OSCFooter footer;

    std::unique_ptr<ComboBoxAttachment> cbNormalizationAttachement;
    std::unique_ptr<ComboBoxAttachment> cbOrderAttachement;
    int maxPossibleOrder;

    ReverseSlider SlDryGain;
    std::unique_ptr<SliderAttachment> SlDryGainAttachment;

    // elements for left side
    DoubleSlider dblSlLeftFilter;
    ReverseSlider SlLeftYaw, SlLeftPitch, SlLeftRoll;
    ReverseSlider SlLeftDelay, SlLeftDelayMS, SlLeftLfoRate, SlLeftLfoDepth, SlLeftFb,
        SlLeftCrossFb, SlLeftGain;
    juce::ComboBox cbLeftDelayMult;
    juce::ToggleButton tbLeftSync;
    juce::TextButton btLeftTap;

    std::unique_ptr<SliderAttachment> dblSlLeftFilterHpAttachment, dblSlLeftFilterLpAttachment;
    std::unique_ptr<SliderAttachment> SlLeftYawAttachment, SlLeftPitchAttachment,
        SlLeftRollAttachment;
    std::unique_ptr<SliderAttachment> SlLeftDelayAttachment, SlLeftLfoRateAttachment,
        SlLeftLfoDepthAttachment, SlLeftFbAttachment, SlLeftCrossFbAttachment, SlLeftGainAttachment;
    std::unique_ptr<ComboBoxAttachment> cbLeftDelayMultAttachment;
    std::unique_ptr<ButtonAttachment> btLeftSyncAttachment;

    // elements for right side
    DoubleSlider dblSlRightFilter;
    ReverseSlider SlRightYaw, SlRightPitch, SlRightRoll;
    ReverseSlider SlRightDelay, SlRightDelayMS, SlRightLfoRate, SlRightLfoDepth, SlRightFb,
        SlRightCrossFb, SlRightGain;
    juce::ComboBox cbRightDelayMult;
    juce::ToggleButton tbRightSync;
    juce::TextButton btRightTap;

    std::unique_ptr<SliderAttachment> dblSlRightFilterHpAttachment, dblSlRightFilterLpAttachment;
    std::unique_ptr<SliderAttachment> SlRightYawAttachment, SlRightPitchAttachment,
        SlRightRollAttachment;
    std::unique_ptr<SliderAttachment> SlRightDelayAttachment, SlRightLfoRateAttachment,
        SlRightLfoDepthAttachment, SlRightFbAttachment, SlRightCrossFbAttachment,
        SlRightGainAttachment;
    std::unique_ptr<ComboBoxAttachment> cbRightDelayMultAttachment;
    std::unique_ptr<ButtonAttachment> btRightSyncAttachment;

    juce::TextButton btTimeMode;

    // labels and groups
    SimpleLabel lbYawL, lbPitchL, lbRollL, lbDelL, lbFbL, lbXFbL;
    SimpleLabel lbYawR, lbPitchR, lbRollR, lbDelR, lbFbR, lbXFbR;
    SimpleLabel lbGainL, lbGainR, lbGainDry;
    TripleLabel lbLfoL, lbLfoR, lbFilterL, lbFilterR;

    juce::GroupComponent gcRotDelL, gcRotDelR, gcFiltL, gcFiltR, gcFbL, gcFbR, gcOutput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DualDelayAudioProcessorEditor)
};
