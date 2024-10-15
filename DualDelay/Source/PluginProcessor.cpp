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
        createParameterLayout()),
    LFOLeft ([] (float phi) { return std::sin (phi); }),
    LFORight ([] (float phi) { return std::sin (phi); })
{
    dryGain = parameters.getRawParameterValue ("dryGain");
    wetGainL = parameters.getRawParameterValue ("wetGainL");
    wetGainR = parameters.getRawParameterValue ("wetGainR");
    delayBPML = parameters.getRawParameterValue ("delayBPML");
    delayBPMR = parameters.getRawParameterValue ("delayBPMR");
    delayMultL = parameters.getRawParameterValue ("delayMultL");
    delayMultR = parameters.getRawParameterValue ("delayMultR");
    yawL = parameters.getRawParameterValue ("yawL");
    yawR = parameters.getRawParameterValue ("yawR");
    pitchL = parameters.getRawParameterValue ("pitchL");
    pitchR = parameters.getRawParameterValue ("pitchR");
    rollL = parameters.getRawParameterValue ("rollL");
    rollR = parameters.getRawParameterValue ("rollR");
    HPcutOffL = parameters.getRawParameterValue ("HPcutOffL");
    HPcutOffR = parameters.getRawParameterValue ("HPcutOffR");
    LPcutOffL = parameters.getRawParameterValue ("LPcutOffL");
    LPcutOffR = parameters.getRawParameterValue ("LPcutOffR");
    feedbackL = parameters.getRawParameterValue ("feedbackL");
    feedbackR = parameters.getRawParameterValue ("feedbackR");
    xfeedbackL = parameters.getRawParameterValue ("xfeedbackL");
    xfeedbackR = parameters.getRawParameterValue ("xfeedbackR");
    lfoRateL = parameters.getRawParameterValue ("lfoRateL");
    lfoRateR = parameters.getRawParameterValue ("lfoRateR");
    lfoDepthL = parameters.getRawParameterValue ("lfoDepthL");
    lfoDepthR = parameters.getRawParameterValue ("lfoDepthR");
    orderSetting = parameters.getRawParameterValue ("orderSetting");

    parameters.addParameterListener ("yawL", this);
    parameters.addParameterListener ("yawR", this);
    parameters.addParameterListener ("pitchL", this);
    parameters.addParameterListener ("pitchR", this);
    parameters.addParameterListener ("rollL", this);
    parameters.addParameterListener ("rollR", this);
    parameters.addParameterListener ("orderSetting", this);

    // cos_z.resize (8);
    // sin_z.resize (8);
    // cos_z.set (0, 1.f);
    // sin_z.set (0, 0.f);

    rotator[0].updateParams (*yawL, *pitchL, *rollL, static_cast<int> (*orderSetting));
    rotator[1].updateParams (*yawR, *pitchR, *rollR, static_cast<int> (*orderSetting));
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

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.numChannels = 1;
    spec.maximumBlockSize = samplesPerBlock;
    LFOLeft.prepare (spec);
    LFORight.prepare (spec);
    LFOLeft.setFrequency (*lfoRateL, true);
    LFORight.setFrequency (*lfoRateR, true);

    for (int i = lowPassFiltersLeft.size(); --i >= 0;)
    {
        lowPassFiltersLeft[i]->reset();
        lowPassFiltersRight[i]->reset();
        highPassFiltersLeft[i]->reset();
        highPassFiltersRight[i]->reset();
    }

    delayBufferLeft.clear();
    delayBufferRight.clear();

    writeOffsetLeft = 0;
    writeOffsetRight = 0;
    readOffsetLeft = 0;
    readOffsetRight = 0;

    delay.resize (samplesPerBlock);
    interpCoeffIdx.resize (samplesPerBlock);
    idx.resize (samplesPerBlock);

    //AudioIN.setSize(AudioIN.getNumChannels(), samplesPerBlock);
    //delayOutLeft.setSize(delayOutLeft.getNumChannels(), samplesPerBlock);
    //delayOutRight.setSize(delayOutRight.getNumChannels(), samplesPerBlock);
    delayOutLeft.clear();
    delayOutRight.clear();

    //delayInLeft.setSize(delayInLeft.getNumChannels(), samplesPerBlock);
    //delayInRight.setSize(delayInRight.getNumChannels(), samplesPerBlock);
    delayInLeft.clear();
    delayInRight.clear();

    _delayL = (60000.0f / (*delayBPML * *delayMultL)) * sampleRate / 1000.0 * 128;
    _delayR = (60000.0f / (*delayBPMR * *delayMultR)) * sampleRate / 1000.0 * 128;
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
    const float msToFractSmpls = fs / 1000.0 * 128.0;

    const int delayBufferLength = fs * 6;
    const int spb = buffer.getNumSamples();

    // Update rotator to current working order if it has changed
    if (rotator[0].getOrder() != workingOrder)
    {
        rotator[0].updateParams (*yawL, *pitchL, *rollL, workingOrder);
        rotator[1].updateParams (*yawR, *pitchR, *rollR, workingOrder);
    }

    //clear not used channels
    for (int channel = nCh; channel < totalNumInputChannels; ++channel)
        buffer.clear (channel, 0, spb);

    LFOLeft.setFrequency (*lfoRateL);
    LFORight.setFrequency (*lfoRateR);

    for (int i = 0; i < nCh; ++i)
    {
        lowPassFiltersLeft[i]->setCoefficients (juce::IIRCoefficients::makeLowPass (
            fs,
            juce::jmin (fs / 2.0, static_cast<double> (*LPcutOffL))));
        lowPassFiltersRight[i]->setCoefficients (juce::IIRCoefficients::makeLowPass (
            fs,
            juce::jmin (fs / 2.0, static_cast<double> (*LPcutOffR))));
        highPassFiltersLeft[i]->setCoefficients (juce::IIRCoefficients::makeHighPass (
            fs,
            juce::jmin (fs / 2.0, static_cast<double> (*HPcutOffL))));
        highPassFiltersRight[i]->setCoefficients (juce::IIRCoefficients::makeHighPass (
            fs,
            juce::jmin (fs / 2.0, static_cast<double> (*HPcutOffR))));
    }

    // ==================== MAKE COPY OF INPUT BUFFER==============================
    for (int channel = 0; channel < nCh; ++channel)
    {
        AudioIN.copyFrom (channel, 0, buffer, channel, 0, spb);
    }

    // ==================== READ FROM DELAYLINE AND GENERTE OUTPUT SIGNAL ===========
    // LEFT CHANNEL
    if (readOffsetLeft + spb >= delayBufferLength)
    { // overflow
        int nFirstRead = delayBufferLength - readOffsetLeft;

        for (int channel = 0; channel < nCh; ++channel)
        {
            delayOutLeft
                .copyFrom (channel, 0, delayBufferLeft, channel, readOffsetLeft, nFirstRead);
            delayOutLeft
                .copyFrom (channel, nFirstRead, delayBufferLeft, channel, 0, spb - nFirstRead);
        }
        delayBufferLeft.clear (readOffsetLeft, nFirstRead);
        delayBufferLeft.clear (0, spb - nFirstRead);

        readOffsetLeft += spb;
        readOffsetLeft -= delayBufferLength;
    }
    else
    { //noverflow
        for (int channel = 0; channel < nCh; ++channel)
        {
            delayOutLeft.copyFrom (channel, 0, delayBufferLeft, channel, readOffsetLeft, spb);
        }
        delayBufferLeft.clear (readOffsetLeft, spb);
        readOffsetLeft += spb;
    }

    // RIGHT CHANNEL
    if (readOffsetRight + spb >= delayBufferLength)
    { // overflow
        int nFirstRead = delayBufferLength - readOffsetRight;

        for (int channel = 0; channel < nCh; ++channel)
        {
            delayOutRight
                .copyFrom (channel, 0, delayBufferRight, channel, readOffsetRight, nFirstRead);
            delayOutRight
                .copyFrom (channel, nFirstRead, delayBufferRight, channel, 0, spb - nFirstRead);
        }
        delayBufferRight.clear (readOffsetRight, nFirstRead);
        delayBufferRight.clear (0, spb - nFirstRead);

        readOffsetRight += spb;
        readOffsetRight -= delayBufferLength;
    }
    else
    { //noverflow
        for (int channel = 0; channel < nCh; ++channel)
        {
            delayOutRight.copyFrom (channel, 0, delayBufferRight, channel, readOffsetRight, spb);
        }
        delayBufferRight.clear (readOffsetRight, spb);
        readOffsetRight += spb;
    }

    // ========== OUTPUT
    buffer.applyGain (juce::Decibels::decibelsToGain (dryGain->load(), -59.91f)); //dry signal
    for (int channel = 0; channel < nCh; ++channel)
    {
        buffer.addFrom (channel,
                        0,
                        delayOutLeft,
                        channel,
                        0,
                        spb,
                        juce::Decibels::decibelsToGain (wetGainL->load(), -59.91f)); //wet signal
        buffer.addFrom (channel,
                        0,
                        delayOutRight,
                        channel,
                        0,
                        spb,
                        juce::Decibels::decibelsToGain (wetGainR->load(), -59.91f)); //wet signal
    }

    // ================ ADD INPUT AND FED BACK OUTPUT WITH PROCESSING ===========

    for (int channel = 0; channel < nCh; ++channel) // should be optimizable with SIMD
    {
        delayInLeft.copyFrom (channel, 0, AudioIN.getReadPointer (channel), spb); // input
        delayInLeft.addFrom (
            channel,
            0,
            delayOutLeft.getReadPointer (channel),
            spb,
            juce::Decibels::decibelsToGain (feedbackL->load(), -59.91f)); // feedback gain
        delayInLeft.addFrom (
            channel,
            0,
            delayOutRight.getReadPointer (channel),
            spb,
            juce::Decibels::decibelsToGain (xfeedbackR->load(), -59.91f)); // feedback bleed gain
        lowPassFiltersLeft[channel]->processSamples (delayInLeft.getWritePointer (channel),
                                                     spb); //filter
        highPassFiltersLeft[channel]->processSamples (delayInLeft.getWritePointer (channel),
                                                      spb); //filter

        delayInRight.copyFrom (channel, 0, AudioIN.getReadPointer (channel), spb); // input
        delayInRight.addFrom (
            channel,
            0,
            delayOutRight.getReadPointer (channel),
            spb,
            juce::Decibels::decibelsToGain (feedbackR->load(), -59.91f)); // feedback gain
        delayInRight.addFrom (
            channel,
            0,
            delayOutLeft.getReadPointer (channel),
            spb,
            juce::Decibels::decibelsToGain (xfeedbackL->load(), -59.91f)); // feedback bleed gain
        lowPassFiltersRight[channel]->processSamples (delayInRight.getWritePointer (channel),
                                                      spb); //filter
        highPassFiltersRight[channel]->processSamples (delayInRight.getWritePointer (channel),
                                                       spb); //filter
    }

    // Apply rotation
    rotator[0].process (&delayInLeft);
    rotator[1].process (&delayInRight);

    // =============== UPDATE DELAY PARAMETERS =====
    float delayL = (60000.0f / (*delayBPML * *delayMultL)) * msToFractSmpls;
    float delayR = (60000.0f / (*delayBPMR * *delayMultR)) * msToFractSmpls;

    int firstIdx, copyL;

    // ============= WRITE INTO DELAYLINE ========================
    // ===== LEFT CHANNEL

    float delayStep = (delayL - _delayL) / spb;
    //calculate firstIdx and copyL
    for (int i = 0; i < spb; ++i)
    {
        delay.set (i,
                   i * 128 + _delayL + i * delayStep
                       + *lfoDepthL * msToFractSmpls * LFOLeft.processSample (1.0f));
    }
    firstIdx =
        (((int) *std::min_element (delay.getRawDataPointer(), delay.getRawDataPointer() + spb))
         >> interpShift)
        - interpOffset;
    int lastIdx =
        (((int) *std::max_element (delay.getRawDataPointer(), delay.getRawDataPointer() + spb))
         >> interpShift)
        - interpOffset;
    copyL = abs (firstIdx - lastIdx) + interpLength;

    delayTempBuffer.clear (0, copyL);
    //delayTempBuffer.clear();
    const float* const* readPtrArr = delayInLeft.getArrayOfReadPointers();

    for (int i = 0; i < spb; ++i)
    {
        float integer;
        float fraction = modff (delay[i], &integer);
        int delayInt = (int) integer;

        int interpCoeffIdx = delayInt & interpMask;
        delayInt = delayInt >> interpShift;
        int idx = delayInt - interpOffset - firstIdx;

#if JUCE_USE_SSE_INTRINSICS
        __m128 interp = getInterpolatedLagrangeWeights (interpCoeffIdx, fraction);

        for (int ch = 0; ch < nCh; ++ch)
        {
            float* dest = delayTempBuffer.getWritePointer (ch, idx);

            __m128 destSamples = _mm_loadu_ps (dest);
            __m128 srcSample = _mm_set1_ps (readPtrArr[ch][i]);
            destSamples = _mm_add_ps (destSamples, _mm_mul_ps (interp, srcSample));
            _mm_storeu_ps (dest, destSamples);
        }
#else /* !JUCE_USE_SSE_INTRINSICS */
        float interp[4];
        getInterpolatedLagrangeWeights (interpCoeffIdx, fraction, interp);

        for (int ch = 0; ch < nCh; ++ch)
        {
            float* dest = delayTempBuffer.getWritePointer (ch, idx);
            float src = readPtrArr[ch][i];
            dest[0] += interp[0] * src;
            dest[1] += interp[1] * src;
            dest[2] += interp[2] * src;
            dest[3] += interp[3] * src;
        }
#endif /* JUCE_USE_SSE_INTRINSICS */
    }
    writeOffsetLeft = readOffsetLeft + firstIdx;
    if (writeOffsetLeft >= delayBufferLength)
        writeOffsetLeft -= delayBufferLength;

    if (writeOffsetLeft + copyL >= delayBufferLength)
    { // overflow
        int firstNumCopy = delayBufferLength - writeOffsetLeft;
        int secondNumCopy = copyL - firstNumCopy;

        for (int channel = 0; channel < nCh; ++channel)
        {
            delayBufferLeft
                .addFrom (channel, writeOffsetLeft, delayTempBuffer, channel, 0, firstNumCopy);
            delayBufferLeft
                .addFrom (channel, 0, delayTempBuffer, channel, firstNumCopy, secondNumCopy);
        }
    }
    else
    { // no overflow
        for (int channel = 0; channel < nCh; ++channel)
        {
            delayBufferLeft.addFrom (channel, writeOffsetLeft, delayTempBuffer, channel, 0, copyL);
        }
    }

    // ===== Right CHANNEL

    delayStep = (delayR - _delayR) / spb;
    //calculate firstIdx and copyL
    for (int i = 0; i < spb; ++i)
    {
        delay.set (i,
                   i * 128 + _delayR + i * delayStep
                       + *lfoDepthR * msToFractSmpls * LFORight.processSample (1.0f));
    }
    firstIdx =
        (((int) *std::min_element (delay.getRawDataPointer(), delay.getRawDataPointer() + spb))
         >> interpShift)
        - interpOffset;
    lastIdx =
        (((int) *std::max_element (delay.getRawDataPointer(), delay.getRawDataPointer() + spb))
         >> interpShift)
        - interpOffset;
    copyL = abs (firstIdx - lastIdx) + interpLength;

    delayTempBuffer.clear (0, copyL);

    const float* const* readPtrArrR = delayInRight.getArrayOfReadPointers();

    for (int i = 0; i < spb; ++i)
    {
        float integer;
        float fraction = modff (delay[i], &integer);
        int delayInt = (int) integer;

        int interpCoeffIdx = delayInt & interpMask;
        delayInt = delayInt >> interpShift;
        int idx = delayInt - interpOffset - firstIdx;

#if JUCE_USE_SSE_INTRINSICS
        __m128 interp = getInterpolatedLagrangeWeights (interpCoeffIdx, fraction);

        for (int ch = 0; ch < nCh; ++ch)
        {
            float* dest = delayTempBuffer.getWritePointer (ch, idx);

            __m128 destSamples = _mm_loadu_ps (dest);
            __m128 srcSample = _mm_set1_ps (readPtrArrR[ch][i]);
            destSamples = _mm_add_ps (destSamples, _mm_mul_ps (interp, srcSample));
            _mm_storeu_ps (dest, destSamples);
        }
#else /* !JUCE_USE_SSE_INTRINSICS */
        float interp[4];
        getInterpolatedLagrangeWeights (interpCoeffIdx, fraction, interp);

        for (int ch = 0; ch < nCh; ++ch)
        {
            float* dest = delayTempBuffer.getWritePointer (ch, idx);
            float src = readPtrArrR[ch][i];
            dest[0] += interp[0] * src;
            dest[1] += interp[1] * src;
            dest[2] += interp[2] * src;
            dest[3] += interp[3] * src;
        }
#endif /* JUCE_USE_SSE_INTRINSICS */
    }
    writeOffsetRight = readOffsetRight + firstIdx;
    if (writeOffsetRight >= delayBufferLength)
        writeOffsetRight -= delayBufferLength;

    if (writeOffsetRight + copyL >= delayBufferLength)
    { // overflow
        int firstNumCopy = delayBufferLength - writeOffsetRight;
        int secondNumCopy = copyL - firstNumCopy;

        for (int channel = 0; channel < nCh; ++channel)
        {
            delayBufferRight
                .addFrom (channel, writeOffsetRight, delayTempBuffer, channel, 0, firstNumCopy);
            delayBufferRight
                .addFrom (channel, 0, delayTempBuffer, channel, firstNumCopy, secondNumCopy);
        }
    }
    else
    { // no overflow
        for (int channel = 0; channel < nCh; ++channel)
        {
            delayBufferRight
                .addFrom (channel, writeOffsetRight, delayTempBuffer, channel, 0, copyL);
        }
    }

    // =============== UPDATE DELAY PARAMETERS =====
    _delayL = delayL;
    _delayR = delayR;
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
        rotator[0].updateParams (*yawL, *pitchL, *rollL, static_cast<int> (newValue));
        rotator[1].updateParams (*yawR, *pitchR, *rollR, static_cast<int> (newValue));
    }

    const int currentOrder = static_cast<int> (*orderSetting);
    if (parameterID == "yawL" || parameterID == "pitchL" || parameterID == "rollL")
        rotator[0].updateParams (*yawL, *pitchL, *rollL, currentOrder);

    if (parameterID == "yawR" || parameterID == "pitchR" || parameterID == "rollR")
        rotator[1].updateParams (*yawR, *pitchR, *rollR, currentOrder);
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
    if (nChannels > _nChannels)
    {
        for (int i = _nChannels; i < nChannels; ++i)
        {
            lowPassFiltersLeft.add (new juce::IIRFilter());
            lowPassFiltersRight.add (new juce::IIRFilter());
            highPassFiltersLeft.add (new juce::IIRFilter());
            highPassFiltersRight.add (new juce::IIRFilter());
        }
    }
    else
    {
        const int diff = _nChannels - nChannels;
        lowPassFiltersLeft.removeRange (nChannels, diff);
        lowPassFiltersRight.removeRange (nChannels, diff);
        highPassFiltersLeft.removeRange (nChannels, diff);
        highPassFiltersRight.removeRange (nChannels, diff);
    }

    AudioIN.setSize (nChannels, samplesPerBlock);
    AudioIN.clear();

    const int maxLfoDepth = static_cast<int> (ceil (
        parameters.getParameterRange ("lfoDepthL").getRange().getEnd() * sampleRate / 500.0f));

    delayBufferLeft.setSize (nChannels,
                             samplesPerBlock + interpOffset - 1 + maxLfoDepth + sampleRate * 6);
    delayBufferRight.setSize (nChannels,
                              samplesPerBlock + interpOffset - 1 + maxLfoDepth + sampleRate * 6);
    delayBufferLeft.clear();
    delayBufferRight.clear();

    delayTempBuffer.setSize (nChannels,
                             samplesPerBlock + interpOffset - 1 + maxLfoDepth + sampleRate * 3);

    delayOutLeft.setSize (nChannels, samplesPerBlock);
    delayOutRight.setSize (nChannels, samplesPerBlock);
    delayOutLeft.clear();
    delayOutRight.clear();

    delayInLeft.setSize (nChannels, samplesPerBlock);
    delayInRight.setSize (nChannels, samplesPerBlock);
    delayInLeft.clear();
    delayInRight.clear();
}

//==============================================================================
std::vector<std::unique_ptr<juce::RangedAudioParameter>>
    DualDelayAudioProcessor::createParameterLayout()
{
    // Remaping functions for delay multiplicator
    using ValueRemapFunction =
        std::function<float (float rangeStart, float rangeEnd, float valueToRemap)>;

    ValueRemapFunction snapToMult = [] (float rangeStart, float rangeEnd, float valueToRemap)
    { return std::exp2f (std::round (std::log2f (valueToRemap))); };

    ValueRemapFunction remapMultTo0to1 = [] (float rangeStart, float rangeEnd, float valueToRemap)
    {
        return (std::log2f (valueToRemap) - std::log2f (rangeStart))
               / (std::log2f (rangeEnd) - std::log2f (rangeStart));
    };

    ValueRemapFunction remap0to1ToMult = [] (float rangeStart, float rangeEnd, float valueToRemap)
    {
        return std::exp2f (valueToRemap * (std::log2f (rangeEnd) - std::log2f (rangeStart))
                           + std::log2f (rangeStart));
    };

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
        juce::NormalisableRange<float> (45.0f, 320.0f, 0.001f),
        100.0f,
        [] (float value) { return juce::String (value, 0); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "delayBPMR",
        "delay right",
        "BPM",
        juce::NormalisableRange<float> (45.0f, 320.0f, 0.001f),
        120.0f,
        [] (float value) { return juce::String (value, 0); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "delayMultL",
        "delay multiplicator left",
        "",
        juce::NormalisableRange<float> (0.5f, 8.0f, remap0to1ToMult, remapMultTo0to1, snapToMult),
        1.0f,
        [] (float value) { return juce::String ("1/") + juce::String (value * 4.0f, 0); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "delayMultR",
        "delay multiplicator right",
        "",
        juce::NormalisableRange<float> (0.5f, 8.0f, remap0to1ToMult, remapMultTo0to1, snapToMult),
        1.0f,
        [] (float value) { return juce::String ("1/") + juce::String (value * 4.0f, 0); },
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
