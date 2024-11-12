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
/*
This class is based on the Ambix warp plugin by Matthias Kronlachner:
https://github.com/kronihias/ambix/
*/
#include <JuceHeader.h>

#include "Conversions.h"
#include "ambisonicTools.h"
#include "efficientSHvanilla.h"
#include "tDesignN10.h"

class AmbisonicWarp
{
public:
    enum AzimuthWarpType
    {
        Sides, // Warp from/towards sides
        FrontBack // Warp from/towards front/back
    };

    enum ElevationWarpType
    {
        Poles, // Warp from/towards poles
        Equator // Warp from/towards equator

    };

    AmbisonicWarp (AzimuthWarpType azWarpType = AzimuthWarpType::Sides,
                   ElevationWarpType elWarpType = ElevationWarpType::Equator,
                   float azWarpFactor = 0.0f,
                   float elWarpFactor = -0.5f,
                   int workingOrder = 7)
    {
        _azWarpType = azWarpType;
        _elWarpType = elWarpType;

        _azWarpFactor = azWarpFactor;
        _elWarpFactor = elWarpFactor;

        _workingOrder = workingOrder;
        _usedChannels = squares[_workingOrder + 1];

        // Calculate decoder matrix
        float tmp_SH[maxChannels];
        for (int p = 0; p < tDesignN; ++p)
        {
            SHEval (maxOrder, tDesignX[p], tDesignY[p], tDesignZ[p], tmp_SH, false);

            for (int r = 0; r < maxChannels; ++r)
                _Y (r, p) = tmp_SH[r];
        }
        _Y *= 1.0f / decodeCorrection (maxOrder);

        calculateWarpingMatrix();

        copyBuffer.setSize (maxChannels, maxChannels);
    }

    void setWorkingOrder (int order)
    {
        _workingOrder = order;
        _usedChannels = squares[_workingOrder + 1];
    }

    int getWorkingOrder() const { return _workingOrder; }

    void process (juce::AudioBuffer<float>* bufferToWarp)
    {
        const int samples = bufferToWarp->getNumSamples();
        const int bufferChannels = bufferToWarp->getNumChannels();

        int workingOrder = juce::jmin (isqrt (bufferChannels) - 1, _workingOrder);
        int nCh = squares[workingOrder + 1];

        // // Resize copyBuffer if necessary
        if ((copyBuffer.getNumChannels() != nCh) || (copyBuffer.getNumSamples() != samples))
            copyBuffer.setSize (nCh, samples);

        // make copy of input
        for (int ch = 0; ch < nCh; ++ch)
        {
            copyBuffer.copyFrom (ch, 0, bufferToWarp->getReadPointer (ch), samples);
            bufferToWarp->clear (ch, 0, samples);

            for (int ch2 = 0; ch2 < nCh; ++ch2)
                bufferToWarp->addFrom (ch,
                                       0,
                                       copyBuffer.getReadPointer (ch2),
                                       samples,
                                       _T (ch, ch2));
        }
    }

private:
    void calculateWarpingMatrix()
    {
        // TODO: Implement warping to equator
        // TODO: Implement azimuth warping
        // TODO: Use dsp context
        juce::dsp::Matrix<float> YH = juce::dsp::Matrix<float> (tDesignN, maxChannels);

        for (int p = 0; p < tDesignN; ++p)
        {
            float az_orig, el_orig;
            Conversions<float>::cartesianToSpherical (tDesignX[p],
                                                      tDesignY[p],
                                                      tDesignZ[p],
                                                      az_orig,
                                                      el_orig);

            float el_warped = el_orig;
            float az_warped = az_orig;
            float g = 1.0f;

            float mu = std::sin (el_orig);
            if (_elWarpType == ElevationWarpType::Poles)
            {
                // Get warped elevation
                el_warped = std::asin ((mu - _elWarpFactor) / (1.0f - _elWarpFactor * mu));

                // Get post-emphasis gain
                g = (1.0f - _elWarpFactor * mu) / std::sqrt (1.0f - _elWarpFactor * _elWarpFactor);
            }
            else if (_elWarpType == ElevationWarpType::Equator)
            {
                if (_elWarpFactor == 0.0f)
                    el_warped = el_orig;

                else
                {
                    float absalpha = std::abs (_elWarpFactor);
                    g = ((1.0f - absalpha * mu * mu)
                         / std::sqrt ((1.0f - absalpha) * (1.0f + absalpha * mu * mu)));

                    if (_elWarpFactor < 0.0f)
                    {
                        el_warped = std::asin (((absalpha - 1.0f)
                                                + std::sqrt ((absalpha - 1.0f) * (absalpha - 1.0f)
                                                             + 4.0f * absalpha * mu * mu))
                                               / (2.0f * absalpha * mu));
                    }
                    else
                    {
                        el_warped =
                            std::asin ((1.0f - absalpha) * mu / (1.0f - absalpha * mu * mu));
                        g = 1.0f / g;
                    }
                }
            }

            juce::Vector3D<float> warped_cart =
                Conversions<float>::sphericalToCartesian (az_warped, el_warped);

            SHEval (maxOrder, warped_cart, YH.getRawDataPointer() + p * 64, false);

            for (int r = 0; r < maxOrder; ++r)
                YH (p, r) *= g;
        }

        _T = _Y * YH;
    }

    const int maxOrder = 7;
    const int maxChannels = squares[maxOrder + 1];

    int _workingOrder, _usedChannels;

    AzimuthWarpType _azWarpType;
    ElevationWarpType _elWarpType;

    float _azWarpFactor, _elWarpFactor;

    juce::dsp::Matrix<float> _Y = juce::dsp::Matrix<float> (maxChannels, tDesignN);
    juce::dsp::Matrix<float> _T = juce::dsp::Matrix<float> (maxChannels, maxChannels);

    juce::AudioBuffer<float> copyBuffer;
};