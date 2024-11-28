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

#include "PluginProcessor.h"
#include "PluginEditor.h"

static constexpr int filterTypePresets[] = { 1, 4, 4, 4, 4, 5 };
static constexpr float filterFrequencyPresets[] = { 20.0f,   120.0f,  500.0f,
                                                    2200.0f, 8000.0f, 16000.0f };

//==============================================================================
MultiEQAudioProcessor::MultiEQAudioProcessor() :
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
    // get pointers to the parameters
    inputChannelsSetting = parameters.getRawParameterValue ("inputChannelsSetting");

    // add listeners to parameter changes
    parameters.addParameterListener ("inputChannelsSetting", this);

    for (int i = 0; i < numFilterBands; ++i)
    {
        filterEnabled[i] = parameters.getRawParameterValue ("filterEnabled" + juce::String (i));
        filterType[i] = parameters.getRawParameterValue ("filterType" + juce::String (i));
        filterFrequency[i] = parameters.getRawParameterValue ("filterFrequency" + juce::String (i));
        filterQ[i] = parameters.getRawParameterValue ("filterQ" + juce::String (i));
        filterGain[i] = parameters.getRawParameterValue ("filterGain" + juce::String (i));

        parameters.addParameterListener ("filterType" + juce::String (i), this);
        parameters.addParameterListener ("filterFrequency" + juce::String (i), this);
        parameters.addParameterListener ("filterQ" + juce::String (i), this);
        parameters.addParameterListener ("filterGain" + juce::String (i), this);
    }
}

MultiEQAudioProcessor::~MultiEQAudioProcessor()
{
}

//==============================================================================
int MultiEQAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int MultiEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MultiEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MultiEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void MultiEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MultiEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    checkInputAndOutput (this, *inputChannelsSetting, *inputChannelsSetting, true);

    for (int f = 0; f < numFilterBands; ++f)
    {
        filterParameters[f].type = FilterType (static_cast<int> (filterType[f]->load()));
        filterParameters[f].frequency = filterFrequency[f]->load();
        filterParameters[f].q = filterQ[f]->load();
        filterParameters[f].linearGain = juce::Decibels::decibelsToGain (filterGain[f]->load());
    }

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumInputChannels();

    MCFilter.prepare (spec, filterParameters);
}

void MultiEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void MultiEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    checkInputAndOutput (this, *inputChannelsSetting, *inputChannelsSetting, false);
    juce::ScopedNoDenormals noDenormals;

    juce::dsp::AudioBlock<float> ab (buffer);
    juce::dsp::ProcessContextReplacing context (ab);

    MCFilter.process (context);
}

//==============================================================================
bool MultiEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MultiEQAudioProcessor::createEditor()
{
    return new MultiEQAudioProcessorEditor (*this, parameters);
}

//==============================================================================
// TODO: Add parameter conversion when loading old projects
void MultiEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    auto oscConfig = state.getOrCreateChildWithName ("OSCConfig", nullptr);
    oscConfig.copyPropertiesFrom (oscParameterInterface.getConfig(), nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MultiEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
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
        }
}

//==============================================================================
void MultiEQAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    DBG ("Parameter with ID " << parameterID << " has changed. New value: " << newValue);

    if (parameterID == "inputChannelsSetting")
        userChangedIOSettings = true;
    else if (parameterID.startsWith ("filter"))
    {
        const int i = parameterID.getLastCharacters (1).getIntValue();

        filterParameters[i].type = FilterType (static_cast<int> (filterType[i]->load()));
        filterParameters[i].frequency = filterFrequency[i]->load();
        filterParameters[i].q = filterQ[i]->load();
        filterParameters[i].linearGain = juce::Decibels::decibelsToGain (filterGain[i]->load());

        MCFilter.updateFilterParams (filterParameters[i], i);

        repaintFV = true;
        userHasChangedFilterSettings = true;
    }
}

void MultiEQAudioProcessor::updateBuffers()
{
    DBG ("IOHelper:  input size: " << input.getSize());
    DBG ("IOHelper: output size: " << output.getSize());
}

//==============================================================================
std::vector<std::unique_ptr<juce::RangedAudioParameter>>
    MultiEQAudioProcessor::createParameterLayout()
{
    // add your audio parameters here
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "inputChannelsSetting",
        "Number of input channels ",
        "",
        juce::NormalisableRange<float> (0.0f, 64.0f, 1.0f),
        0.0f,
        [] (float value) { return value < 0.5f ? "Auto" : juce::String (value); },
        nullptr));

    int i = 0;
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterEnabled" + juce::String (i),
        "Filter Enablement " + juce::String (i + 1),
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        1.0f,
        [] (float value) { return value < 0.5 ? juce::String ("OFF") : juce::String ("ON"); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterType" + juce::String (i),
        "Filter Type " + juce::String (i + 1),
        "",
        juce::NormalisableRange<float> (0.0f, 3.0f, 1.0f),
        filterTypePresets[i],
        [] (float value)
        {
            if (value < 0.5f)
                return "HP (6dB/oct)";
            else if (value >= 0.5f && value < 1.5f)
                return "HP (12dB/oct)";
            else if (value >= 1.5f && value < 2.5f)
                return "HP (24dB/oct)";
            else
                return "Low-shelf";
        },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterFrequency" + juce::String (i),
        "Filter Frequency " + juce::String (i + 1),
        "Hz",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.4f),
        filterFrequencyPresets[i],
        [] (float value) { return juce::String (value, 0); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterQ" + juce::String (i),
        "Filter Q " + juce::String (i + 1),
        "",
        juce::NormalisableRange<float> (0.05f, 8.0f, 0.05f),
        0.7f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterGain" + juce::String (i),
        "Filter Gain " + juce::String (i + 1),
        "dB",
        juce::NormalisableRange<float> (-60.0f, 15.0f, 0.1f),
        0.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    for (int i = 1; i < numFilterBands - 1; ++i)
    {
        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "filterEnabled" + juce::String (i),
            "Filter Enablement " + juce::String (i + 1),
            "",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
            1.0f,
            [] (float value) { return value < 0.5 ? juce::String ("OFF") : juce::String ("ON"); },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "filterType" + juce::String (i),
            "Filter Type " + juce::String (i + 1),
            "",
            juce::NormalisableRange<float> (3.0f, 5.0f, 1.0f),
            filterTypePresets[i],
            [] (float value)
            {
                if (value < 3.5f)
                    return "Low-shelf";
                else if (value >= 3.5f && value < 4.5f)
                    return "Peak";
                else
                    return "High-shelf";
            },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "filterFrequency" + juce::String (i),
            "Filter Frequency " + juce::String (i + 1),
            "Hz",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.4f),
            filterFrequencyPresets[i],
            [] (float value) { return juce::String (value, 0); },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "filterQ" + juce::String (i),
            "Filter Q " + juce::String (i + 1),
            "",
            juce::NormalisableRange<float> (0.05f, 8.0f, 0.05f),
            0.7f,
            [] (float value) { return juce::String (value, 2); },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "filterGain" + juce::String (i),
            "Filter Gain " + juce::String (i + 1),
            "dB",
            juce::NormalisableRange<float> (-60.0f, 15.0f, 0.1f),
            0.0f,
            [] (float value) { return juce::String (value, 1); },
            nullptr));
    }

    i = numFilterBands - 1;

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterEnabled" + juce::String (i),
        "Filter Enablement " + juce::String (i + 1),
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        1.0f,
        [] (float value) { return value < 0.5 ? juce::String ("OFF") : juce::String ("ON"); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterType" + juce::String (i),
        "Filter Type " + juce::String (i + 1),
        "",
        juce::NormalisableRange<float> (5.0f, 8.0f, 1.0f),
        filterTypePresets[i],
        [] (float value)
        {
            if (value < 5.5f)
                return "High-shelf";
            else if (value >= 5.5f && value < 6.5f)
                return "LP (6dB/Oct)";
            else if (value >= 6.5f && value < 7.5f)
                return "LP (12dB/oct)";
            else
                return "LP (24dB/oct)";
        },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterFrequency" + juce::String (i),
        "Filter Frequency " + juce::String (i + 1),
        "Hz",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.4f),
        filterFrequencyPresets[i],
        [] (float value) { return juce::String (value, 0); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterQ" + juce::String (i),
        "Filter Q " + juce::String (i + 1),
        "",
        juce::NormalisableRange<float> (0.05f, 8.0f, 0.05f),
        0.7f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "filterGain" + juce::String (i),
        "Filter Gain " + juce::String (i + 1),
        "dB",
        juce::NormalisableRange<float> (-60.0f, 15.0f, 0.1f),
        0.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    return params;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultiEQAudioProcessor();
}
