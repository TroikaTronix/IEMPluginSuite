/*
 ==============================================================================
 This file is part of the IEM plug-in suite.
 Authors: Felix Holzmüller
 Copyright (c) 2024 - Institute of Electronic Music and Acoustics (IEM)
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
// Based on the MultiEQ by Daniel Rudrich

#pragma once

#include <JuceHeader.h>

template <int numFilterBands, int maxChannels>
class MultiChannelFilter
{
#if JUCE_USE_SIMD
    using IIRfloat = juce::dsp::SIMDRegister<float>;
    static constexpr int IIRfloat_elements = juce::dsp::SIMDRegister<float>::size();
    static constexpr int nSIMDFilters = std::ceil (maxChannels / IIRfloat_elements);
#else /* !JUCE_USE_SIMD */
    using IIRfloat = float;
    static constexpr int IIRfloat_elements = 1;
    static constexpr int nSIMDFilters = maxChannels;
#endif /* JUCE_USE_SIMD */

public:
    enum RegularFilterType
    {
        FirstOrderHighPass,
        SecondOrderHighPass,
        LinkwitzRileyHighPass,
        LowShelf,
        PeakFilter,
        HighShelf,
        FirstOrderLowPass,
        SecondOrderLowPass,
        LinkwitzRileyLowPass,
        AllPass
    };

    struct FilterParameters
    {
        RegularFilterType type = AllPass;
        float frequency = 1000.0f;
        float linearGain = 1.0f;
        float q = 0.707f;
    };

    MultiChannelFilter() {}
    ~MultiChannelFilter() {}

    void prepare (const juce::dsp::ProcessSpec spec)
    {
        sampleRate = spec.sampleRate;

        for (int i = 0; i < 2; ++i)
        {
            additionalTempCoefficients[i] =
                juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, 20.0f);
            additionalProcessorCoefficients[i] =
                juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, 20.0f);
        }

        for (int i = 0; i < numFilterBands; ++i)
        {
            processorCoefficients[i] = IIR::Coefficients<float>::makeAllPass (48000.0, 20.0f);
        }

        copyFilterCoefficientsToProcessor();

        for (int i = 0; i < numFilterBands; ++i)
        {
            filterArrays[i].clear();
            for (int ch = 0; ch < nSIMDFilters; ++ch)
                filterArrays[i].add (new IIR::Filter<IIRfloat> (processorCoefficients[i]));
        }

        for (int i = 0; i < 2; ++i)
        {
            additionalFilterArrays[i].clear();
            for (int ch = 0; ch < nSIMDFilters; ++ch)
                additionalFilterArrays[i].add (
                    new IIR::Filter<IIRfloat> (additionalProcessorCoefficients[0]));
        }
    }

private:
    float sampleRate { 0.0f };

    // filter dummy for GUI
    juce::dsp::IIR::Coefficients<double>::Ptr guiCoefficients[numFilterBands];

    juce::dsp::IIR::Coefficients<float>::Ptr processorCoefficients[numFilterBands];
    juce::dsp::IIR::Coefficients<float>::Ptr additionalProcessorCoefficients[2];

    juce::dsp::IIR::Coefficients<float>::Ptr tempCoefficients[numFilterBands];
    juce::dsp::IIR::Coefficients<float>::Ptr additionalTempCoefficients[2];

    // data for interleaving audio
    juce::HeapBlock<char> interleavedBlockData[nSIMDFilters], zeroData;
    juce::OwnedArray<juce::dsp::AudioBlock<IIRfloat>> interleavedData;
    juce::dsp::AudioBlock<float> zero;

    FilterParameters filterParameters[numFilterBands];

    // filters for processing
    juce::OwnedArray<juce::dsp::IIR::Filter<IIRfloat>> filterArrays[numFilterBands];
    juce::OwnedArray<juce::dsp::IIR::Filter<IIRfloat>> additionalFilterArrays[2];

    juce::Atomic<bool> userHasChangedFilterSettings = true;
};