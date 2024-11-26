/*
 ==============================================================================
 This file is part of the IEM plug-in suite.
 Author: Daniel Rudrich, Felix Holzmüller
 Copyright (c) 2017, 2024 - Institute of Electronic Music and Acoustics (IEM)
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

#include <JuceHeader.h>

#include "../../resources/AmbisonicRotator.h"
#include "../../resources/AmbisonicWarp.h"
#include "../../resources/AudioProcessorBase.h"
#include "../../resources/OnePoleFilter.h"
#include "../../resources/ambisonicTools.h"

#define ProcessorClass DualDelayAudioProcessor
#define OneOverSqrt2 0.7071067811865475f

//==============================================================================
/**
*/
class DualDelayAudioProcessor
    : public AudioProcessorBase<IOTypes::Ambisonics<>, IOTypes::Ambisonics<>, true>
{
public:
    constexpr static int numberOfInputChannels = 64;
    constexpr static int numberOfOutputChannels = 64;
    //==============================================================================
    DualDelayAudioProcessor();
    ~DualDelayAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioSampleBuffer&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void updateBuffers() override;
    void updateFilters (int idx);

    std::tuple<float, float> msToBPM (float ms);

    //======= Parameters ===========================================================
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> createParameterLayout();
    //==============================================================================

private:
    //==============================================================================
    // parameters
    std::atomic<float>* dryGain;
    std::atomic<float>* wetGain[2];
    std::atomic<float>* delayBPM[2];
    std::atomic<float>* delayMult[2];
    std::atomic<float>* sync[2];
    std::atomic<float>* transformMode[2];
    std::atomic<float>* yaw[2];
    std::atomic<float>* pitch[2];
    std::atomic<float>* roll[2];
    std::atomic<float>* warpModeAz[2];
    std::atomic<float>* warpModeEl[2];
    std::atomic<float>* warpFactorAz[2];
    std::atomic<float>* warpFactorEl[2];
    std::atomic<float>* LPcutOff[2];
    std::atomic<float>* HPcutOff[2];
    std::atomic<float>* feedback[2];
    std::atomic<float>* xfeedback[2];
    std::atomic<float>* lfoRate[2];
    std::atomic<float>* lfoDepth[2];
    std::atomic<float>* orderSetting;

    juce::dsp::Oscillator<float> LFO[2];
    AmbisonicRotator rotator[2];
    AmbisonicWarp warp[2];

    juce::OwnedArray<juce::IIRFilter> lowPassFilters[2];
    juce::OwnedArray<juce::IIRFilter> highPassFilters[2];

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine[2];
    juce::AudioBuffer<float> delayBuffer[2];
    juce::AudioBuffer<float> delayOutBuffer[2];
    OnePoleFilter<float> delayTimeInterp[2];

    bool updateFilterCoeffs = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DualDelayAudioProcessor)
};
