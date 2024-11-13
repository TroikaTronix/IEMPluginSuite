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

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DualDelayAudioProcessor::DualDelayAudioProcessor() :
    AudioProcessorBase (
#ifndef JucePlugin_PreferredChannelConfigurations
        BusesProperties()
    #if ! JucePlugin_IsMidiEffect
        #if ! JucePlugin_IsSynth
            .withInput ("Input",
                        ((juce::PluginHostType::getPluginLoadedAs()
                          == juce::AudioProcessor::wrapperType_VST3)
                             ? juce::AudioChannelSet::ambisonic (1)
                             : juce::AudioChannelSet::ambisonic (7)),
                        true)
        #endif
            .withOutput ("Output",
                         ((juce::PluginHostType::getPluginLoadedAs()
                           == juce::AudioProcessor::wrapperType_VST3)
                              ? juce::AudioChannelSet::ambisonic (1)
                              : juce::AudioChannelSet::ambisonic (7)),
                         true)
    #endif
            ,
#endif
        createParameterLayout())
{
    dryGain = parameters.getRawParameterValue ("dryGain");
    orderSetting = parameters.getRawParameterValue ("orderSetting");

    for (int i = 0; i < 2; ++i)
    {
        juce::String side = (i == 0) ? "L" : "R";
        wetGain[i] = parameters.getRawParameterValue ("wetGain" + side);
        delayBPM[i] = parameters.getRawParameterValue ("delayBPM" + side);
        delayMult[i] = parameters.getRawParameterValue ("delayMult" + side);
        sync[i] = parameters.getRawParameterValue ("sync" + side);
        transformMode[i] = parameters.getRawParameterValue ("transformMode" + side);
        yaw[i] = parameters.getRawParameterValue ("yaw" + side);
        pitch[i] = parameters.getRawParameterValue ("pitch" + side);
        roll[i] = parameters.getRawParameterValue ("roll" + side);
        warpModeAz[i] = parameters.getRawParameterValue ("warpModeAz" + side);
        warpModeEl[i] = parameters.getRawParameterValue ("warpModeEl" + side);
        warpFactorAz[i] = parameters.getRawParameterValue ("warpFactorAz" + side);
        warpFactorEl[i] = parameters.getRawParameterValue ("warpFactorEl" + side);
        HPcutOff[i] = parameters.getRawParameterValue ("HPcutOff" + side);
        LPcutOff[i] = parameters.getRawParameterValue ("LPcutOff" + side);
        feedback[i] = parameters.getRawParameterValue ("feedback" + side);
        xfeedback[i] = parameters.getRawParameterValue ("xfeedback" + side);
        lfoRate[i] = parameters.getRawParameterValue ("lfoRate" + side);
        lfoDepth[i] = parameters.getRawParameterValue ("lfoDepth" + side);

        parameters.addParameterListener ("yaw" + side, this);
        parameters.addParameterListener ("pitch" + side, this);
        parameters.addParameterListener ("roll" + side, this);
        parameters.addParameterListener ("warpModeAz" + side, this);
        parameters.addParameterListener ("warpModeEl" + side, this);
        parameters.addParameterListener ("warpFactorAz" + side, this);
        parameters.addParameterListener ("warpFactorEl" + side, this);
        parameters.addParameterListener ("lfoRate" + side, this);
        parameters.addParameterListener ("HPcutOff" + side, this);
        parameters.addParameterListener ("LPcutOff" + side, this);

        LFO[i].initialise ([] (float phi) { return std::sin (phi); });

        rotator[i].updateParams (*yaw[i], *pitch[i], *roll[i], static_cast<int> (*orderSetting));
        warp[i].updateParams (
            AmbisonicWarp::AzimuthWarpType (juce::roundToInt (warpModeAz[i]->load())),
            AmbisonicWarp::ElevationWarpType (juce::roundToInt (warpModeEl[i]->load())),
            *warpFactorAz[i],
            *warpFactorEl[i]);
        warp[i].setWorkingOrder (static_cast<int> (*orderSetting));
    }

    parameters.addParameterListener ("orderSetting", this);
}

DualDelayAudioProcessor::~DualDelayAudioProcessor()
{
}

//==============================================================================
int DualDelayAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int DualDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DualDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DualDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void DualDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DualDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    checkInputAndOutput (this, *orderSetting, *orderSetting, true);

    const int maxLfoDepth = static_cast<int> (ceil (
        parameters.getParameterRange ("lfoDepthL").getRange().getEnd() * sampleRate / 500.0f));

    const int maxDelay =
        static_cast<int> (ceil (sampleRate * 60.0f
                                    / (parameters.getParameterRange ("delayBPML").start
                                       * parameters.getParameterRange ("delayMultL").start)
                                + maxLfoDepth));

    juce::dsp::ProcessSpec specDelay;
    specDelay.sampleRate = sampleRate;
    specDelay.numChannels = numberOfOutputChannels;
    specDelay.maximumBlockSize = samplesPerBlock;

    juce::dsp::ProcessSpec specLFO;
    specLFO.sampleRate = sampleRate;
    specLFO.numChannels = 1;
    specLFO.maximumBlockSize = samplesPerBlock;

    for (int i = 0; i < 2; ++i)
    {
        LFO[i].prepare (specLFO);
        LFO[i].setFrequency (*lfoRate[i], true);

        for (int j = lowPassFilters[i].size(); --j >= 0;)
        {
            lowPassFilters[i][j]->reset();
            highPassFilters[i][j]->reset();

            updateFilterCoeffs = true;
        }

        delayBuffer[i].clear();
        delayOutBuffer[i].clear();

        delayLine[i].reset();
        delayLine[i].prepare (specDelay);

        delayLine[i].setMaximumDelayInSamples (maxDelay);
        delayLine[i].setDelay (sampleRate * 60.0f / (*delayBPM[0] * *delayMult[0]));

        delayTimeInterp[i].setFrequency (0.5f, sampleRate);
    }
}

void DualDelayAudioProcessor::releaseResources()
{
}

void DualDelayAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    checkInputAndOutput (this, *orderSetting, *orderSetting);

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int workingOrder =
        juce::jmin (isqrt (buffer.getNumChannels()) - 1, input.getOrder(), output.getOrder());
    const int nCh = squares[workingOrder + 1];
    const double fs = getSampleRate();
    const int spb = buffer.getNumSamples();
    const float msToSamples = fs / 1000.0f;

    // Sync delay time to host BPM
    juce::Optional<double> hostBPM;
    juce::AudioPlayHead* playHead = getPlayHead();

    if (updateFilterCoeffs)
    {
        for (int i = 0; i < 2; ++i)
            updateFilters (i);
    }

    if (playHead != nullptr && (*sync[0] > 0.5f || *sync[1] > 0.5f))
    {
        hostBPM = playHead->getPosition()->getBpm();

        if (hostBPM.hasValue())
        {
            auto hostBPMMapped = parameters.getParameter ("delayBPML")->convertTo0to1 (*hostBPM);
            if (*sync[0] > 0.5f)
                parameters.getParameter ("delayBPML")
                    ->setValue (static_cast<float> (hostBPMMapped));
            if (*sync[1] > 0.5f)
                parameters.getParameter ("delayBPMR")
                    ->setValue (static_cast<float> (hostBPMMapped));
        }
    }
    //clear not used channels
    for (int channel = nCh; channel < totalNumInputChannels; ++channel)
        buffer.clear (channel, 0, spb);

    for (int i = 0; i < 2; ++i)
    {
        int otherSide = (i + 1) % 2;

        // Update rotator to current working order if it has changed
        if (rotator[i].getOrder() != workingOrder)
        {
            rotator[i].updateParams (*yaw[i], *pitch[i], *roll[i], workingOrder);
        }

        // Setting up delay interpolator
        const float newDelay = fs * 60.0f / (*delayBPM[i] * *delayMult[i]);
        delayTimeInterp[i].setTargetValue (newDelay);

        // ========== Fill input buffer for delay line ==========
        // Copy dry input to delay buffer
        delayBuffer[i].makeCopyOf (buffer, true);

        // Copy feedback and crossfeed to delay buffer
        for (int channel = 0; channel < nCh; ++channel)
        {
            delayBuffer[i].addFrom (channel,
                                    0,
                                    delayOutBuffer[i],
                                    channel,
                                    0,
                                    spb,
                                    juce::Decibels::decibelsToGain (feedback[i]->load(), -59.91f));
            delayBuffer[i].addFrom (
                channel,
                0,
                delayOutBuffer[otherSide],
                channel,
                0,
                spb,
                juce::Decibels::decibelsToGain (xfeedback[otherSide]->load(), -59.91f));

            // Apply filter to delayed signal
            lowPassFilters[i][channel]->processSamples (delayBuffer[i].getWritePointer (channel),
                                                        spb);
            highPassFilters[i][channel]->processSamples (delayBuffer[i].getWritePointer (channel),
                                                         spb);
        }

        // Rotate/warp delayed signal
        if (*transformMode[i] > 0.5f)
            rotator[i].process (&delayBuffer[i]);
        else
            warp[i].process (&delayBuffer[i]);
    }

    // ========== Add and get samples from delay line ==========
    for (int i = 0; i < 2; ++i)
    {
        LFO[i].setFrequency (*lfoRate[i]);

        // Cycle through it on sample basis for smooth delay time changes
        for (int sample = 0; sample < spb; ++sample)
        {
            float currentDelay = delayTimeInterp[i].process()
                                 + LFO[i].processSample (1.0f) * msToSamples * *lfoDepth[i];

            for (int channel = 0; channel < nCh; ++channel)
            {
                // Push samples into delay line
                delayLine[i].pushSample (channel, delayBuffer[i].getSample (channel, sample));

                // Pop samples from delay line to output buffer
                delayOutBuffer[i].setSample (channel,
                                             sample,
                                             delayLine[i].popSample (channel, currentDelay));
            }
        }
    }

    // ========== Calculate output ==========
    // Apply gain to direct signal
    buffer.applyGain (juce::Decibels::decibelsToGain (dryGain->load(), -59.91f));

    // Add delayed signal to output buffer
    for (int i = 0; i < 2; ++i)
    {
        for (int channel = 0; channel < nCh; ++channel)
        {
            buffer.addFrom (channel,
                            0,
                            delayOutBuffer[i],
                            channel,
                            0,
                            spb,
                            juce::Decibels::decibelsToGain (wetGain[i]->load(), -59.91f));
        }
    }
}

//==============================================================================
bool DualDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DualDelayAudioProcessor::createEditor()
{
    return new DualDelayAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void DualDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    auto oscConfig = state.getOrCreateChildWithName ("OSCConfig", nullptr);
    oscConfig.copyPropertiesFrom (oscParameterInterface.getConfig(), nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DualDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            if (parameters.state.hasProperty ("OSCPort")) // legacy
            {
                oscParameterInterface.getOSCReceiver().connect (
                    parameters.state.getProperty ("OSCPort", juce::var (-1)));
                parameters.state.removeProperty ("OSCPort", nullptr);
            }

            auto oscConfig = parameters.state.getChildWithName ("OSCConfig");
            if (oscConfig.isValid())
                oscParameterInterface.setConfig (oscConfig);

            // Add compatibility layer for old delay parameters
            juce::String ch[2] = { "L", "R" };

            for (int i = 0; i < 2; ++i)
            {
                // rotation to yaw
                auto oldTimeParameter = xmlState->getChildByAttribute ("id", "rotation" + ch[i]);
                if (oldTimeParameter != nullptr)
                {
                    auto parameterState =
                        parameters.state.getChildWithProperty ("id", "yaw" + ch[i]);

                    if (parameterState.isValid())
                    {
                        parameterState.setProperty (
                            "value",
                            oldTimeParameter->getStringAttribute ("value").getFloatValue(),
                            nullptr);
                    }
                }

                // ms to BPM
                oldTimeParameter = xmlState->getChildByAttribute ("id", "delayTime" + ch[i]);
                if (oldTimeParameter != nullptr)
                {
                    float value =
                        60000.0f / oldTimeParameter->getStringAttribute ("value").getFloatValue();
                    auto delayState =
                        parameters.state.getChildWithProperty ("id", "delayBPM" + ch[i]);

                    const float maxBPM =
                        parameters.getParameterRange ("delayBPM" + ch[i]).getRange().getEnd();
                    const float maxMult =
                        parameters.getParameterRange ("delayMult" + ch[i]).getRange().getEnd();

                    // Set multiplicator if BPM is too high
                    if (value > maxBPM)
                    {
                        float mult = static_cast<float> (
                            juce::nextPowerOfTwo (static_cast<int> (ceil (value / maxBPM))));
                        value /= mult;

                        // Limiting multiplicator to maximum value, so it's at least somehow on beat
                        mult = juce::jmin (mult, maxMult);

                        auto multState =
                            parameters.state.getChildWithProperty ("id", "delayMult" + ch[i]);
                        if (multState.isValid())
                            multState.setProperty ("value", mult, nullptr);
                    }

                    if (delayState.isValid())
                        delayState.setProperty ("value", value, nullptr);
                }
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DualDelayAudioProcessor();
}

void DualDelayAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "orderSetting")
    {
        userChangedIOSettings = true;
        rotator[0].updateParams (*yaw[0], *pitch[0], *roll[0], static_cast<int> (newValue));
        rotator[1].updateParams (*yaw[1], *pitch[1], *roll[1], static_cast<int> (newValue));

        warp[1].setWorkingOrder (static_cast<int> (newValue));
        warp[0].setWorkingOrder (static_cast<int> (newValue));
    }
    const int currentOrder = static_cast<int> (*orderSetting);

    bool sideIdx = parameterID.endsWith ("L") ? 0 : 1;
    juce::String side = (sideIdx == 0) ? "L" : "R";

    if (parameterID == "yaw" + side || parameterID == "pitch" + side
        || parameterID == "roll" + side)
        rotator[sideIdx].updateParams (*yaw[sideIdx],
                                       *pitch[sideIdx],
                                       *roll[sideIdx],
                                       currentOrder);

    else if (parameterID == "warpModeAz" + side || parameterID == "warpModeEl" + side
             || parameterID == "warpFactorAz" + side || parameterID == "warpFactorEl" + side)
        warp[sideIdx].updateParams (
            AmbisonicWarp::AzimuthWarpType (juce::roundToInt (warpModeAz[sideIdx]->load())),
            AmbisonicWarp::ElevationWarpType (juce::roundToInt (warpModeEl[sideIdx]->load())),
            *warpFactorAz[sideIdx],
            *warpFactorEl[sideIdx]);

    else if (parameterID.startsWith ("HPcutOff") || parameterID.startsWith ("LPcutOff"))
    {
        updateFilters (sideIdx);
    }
}

void DualDelayAudioProcessor::updateBuffers()
{
    DBG ("IOHelper:  input size: " << input.getSize());
    DBG ("IOHelper: output size: " << output.getSize());

    const int nChannels = juce::jmin (input.getNumberOfChannels(), output.getNumberOfChannels());
    const int _nChannels =
        juce::jmin (input.getPreviousNumberOfChannels(), output.getPreviousNumberOfChannels());
    const int samplesPerBlock = getBlockSize();

    const double sampleRate = getSampleRate();

    const int maxLfoDepth = static_cast<int> (ceil (
        parameters.getParameterRange ("lfoDepthL").getRange().getEnd() * sampleRate / 500.0f));

    const int maxDelay =
        static_cast<int> (ceil (sampleRate * 60.0f
                                    / (parameters.getParameterRange ("delayBPML").start
                                       * parameters.getParameterRange ("delayMultL").start)
                                + maxLfoDepth));

    for (int i = 0; i < 2; ++i)
    {
        delayBuffer[i].setSize (nChannels, samplesPerBlock);
        delayBuffer[i].clear();

        delayOutBuffer[i].setSize (nChannels, samplesPerBlock);
        delayOutBuffer[i].clear();

        if (nChannels > _nChannels)
        {
            for (int channel = _nChannels; channel < nChannels; ++channel)
            {
                lowPassFilters[i].add (new juce::IIRFilter());
                highPassFilters[i].add (new juce::IIRFilter());
            }
        }
        else
        {
            const int diff = _nChannels - nChannels;
            lowPassFilters[i].removeRange (nChannels, diff);
            highPassFilters[i].removeRange (nChannels, diff);
        }
    }

    updateFilterCoeffs = true;
}

void DualDelayAudioProcessor::updateFilters (const int endIdx)
{
    const int nCh = lowPassFilters[endIdx].size();
    const double fs = getSampleRate();

    for (int channel = 0; channel < nCh; ++channel)
    {
        lowPassFilters[endIdx][channel]->setCoefficients (juce::IIRCoefficients::makeLowPass (
            fs,
            juce::jmin (fs / 2.0, static_cast<double> (LPcutOff[endIdx]->load()))));
        highPassFilters[endIdx][channel]->setCoefficients (juce::IIRCoefficients::makeHighPass (
            fs,
            juce::jmin (fs / 2.0, static_cast<double> (HPcutOff[endIdx]->load()))));
    }

    updateFilterCoeffs = false;
}

std::tuple<float, float> DualDelayAudioProcessor::msToBPM (const float ms)
{
    float transformedBPM = 60000.0f / ms;
    float mult = 1.0f;

    const float maxBPM = parameters.getParameterRange ("delayBPML").getRange().getEnd();
    const float minBPM = parameters.getParameterRange ("delayBPML").getRange().getStart();
    const float maxMult = parameters.getParameterRange ("delayMultL").getRange().getEnd();
    const float minMult = parameters.getParameterRange ("delayMultL").getRange().getStart();

    if (transformedBPM > maxBPM)
    {
        mult = static_cast<float> (
            juce::nextPowerOfTwo (static_cast<int> (ceil (transformedBPM / maxBPM))));
        transformedBPM /= mult;
        mult = juce::jmin (mult, maxMult);
    }
    else if (transformedBPM < minBPM)
    {
        float transformedBPM_tmp = juce::jmax (transformedBPM / minMult,
                                               minBPM); // Clip values to avoid division by zero

        mult = static_cast<float> (
            juce::nextPowerOfTwo (static_cast<int> (floor (transformedBPM / minBPM))));
        mult = juce::jmax (mult, 1.0f) * minMult;
        transformedBPM /= mult;
    }

    return { transformedBPM, mult };
}

//==============================================================================
std::vector<std::unique_ptr<juce::RangedAudioParameter>>
    DualDelayAudioProcessor::createParameterLayout()
{
    // add your audio parameters here
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "orderSetting",
        "Ambisonics Order",
        "",
        juce::NormalisableRange<float> (0.0f, 8.0f, 1.0f),
        0.0f,
        [] (float value)
        {
            if (value >= 0.5f && value < 1.5f)
                return "0th";
            else if (value >= 1.5f && value < 2.5f)
                return "1st";
            else if (value >= 2.5f && value < 3.5f)
                return "2nd";
            else if (value >= 3.5f && value < 4.5f)
                return "3rd";
            else if (value >= 4.5f && value < 5.5f)
                return "4th";
            else if (value >= 5.5f && value < 6.5f)
                return "5th";
            else if (value >= 6.5f && value < 7.5f)
                return "6th";
            else if (value >= 7.5f)
                return "7th";
            else
                return "Auto";
        },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "useSN3D",
        "Normalization",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        1.0f,
        [] (float value)
        {
            if (value >= 0.5f)
                return "SN3D";
            else
                return "N3D";
        },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "dryGain",
        "Dry amount",
        "dB",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        0.0f,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "wetGainL",
        "Wet amount left",
        "dB",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -6.0f,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "wetGainR",
        "Wet amount right",
        "dB",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -6.0f,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "delayBPML",
        "delay left",
        "BPM",
        juce::NormalisableRange<float> (46.0f, 320.0f, 0.001f),
        100.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "delayBPMR",
        "delay right",
        "BPM",
        juce::NormalisableRange<float> (46.0f, 320.0f, 0.001f),
        120.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "delayMultL",
        "delay multiplicator left",
        "",
        juce::NormalisableRange<float> (1.0f, 16.0f, 1.0f, 0.6f),
        1.0f,
        [] (float value) { return juce::String (std::round (value), 0); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "delayMultR",
        "delay multiplicator right",
        "",
        juce::NormalisableRange<float> (1.0f, 16.0f, 1.0f, 0.6f),
        1.0f,
        [] (float value) { return juce::String (std::round (value), 0); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "syncL",
        "sync BPM left",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value >= 0.5f) ? "on" : "off"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "syncR",
        "sync BPM right",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value >= 0.5f) ? "on" : "off"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "transformModeL",
        "transform mode left",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value > 0.5f) ? "warp" : "rotate"; },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "transformModeR",
        "transform mode right",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value > 0.5f) ? "warp" : "rotate"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "yawL",
        "yaw left",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f),
        10.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "yawR",
        "yaw right",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f),
        -7.5f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "pitchL",
        "pitch left",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f),
        0.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "pitchR",
        "pitch right",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f),
        0.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "rollL",
        "roll left",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f),
        0.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "rollR",
        "roll right",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f),
        0.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpModeAzL",
        "Warp mode azimuth left",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value > 0.5f) ? "One Point" : "Two Point"; },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpModeElL",
        "warp mode elevation left",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value > 0.5f) ? "Pole" : "Equator"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpModeAzR",
        "Warp mode azimuth right",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value > 0.5f) ? "One Point" : "Two Point"; },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpModeElR",
        "warp mode elevation right",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value > 0.5f) ? "Pole" : "Equator"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpFactorAzL",
        "warp factor azimuth left",
        "",
        juce::NormalisableRange<float> (-0.9f, 0.9f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpFactorElL",
        "warp factor elevation left",
        "",
        juce::NormalisableRange<float> (-0.9f, 0.9f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpFactorAzR",
        "warp factor azimuth right",
        "",
        juce::NormalisableRange<float> (-0.9f, 0.9f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "warpFactorElR",
        "warp factor elevation right",
        "",
        juce::NormalisableRange<float> (-0.9f, 0.9f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "LPcutOffL",
        "lowpass frequency left",
        "Hz",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.2),
        20000.0f,
        [] (float value) { return juce::String (value, 0); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "LPcutOffR",
        "lowpass frequency right",
        "Hz",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.2),
        20000.0f,
        [] (float value) { return juce::String (value, 0); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "HPcutOffL",
        "highpass frequency left",
        "Hz",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.2),
        100.0f,
        [] (float value) { return juce::String (value, 0); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "HPcutOffR",
        "highpass frequency right",
        "Hz",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.2),
        100.0f,
        [] (float value) { return juce::String (value, 0); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "feedbackL",
        "feedback left",
        "dB",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -8.0f,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "feedbackR",
        "feedback right",
        "dB",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -8.0f,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "xfeedbackL",
        "cross feedback left",
        "dB",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -20.0f,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "xfeedbackR",
        "cross feedback right",
        "dB",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -20.0f,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "lfoRateL",
        "LFO left rate",
        "Hz",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "lfoRateR",
        "LFO right rate",
        "Hz",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "lfoDepthL",
        "LFO left depth",
        "ms",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "lfoDepthR",
        "LFO right depth",
        "ms",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));

    return params;
}
