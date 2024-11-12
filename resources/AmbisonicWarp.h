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
        OnePoint, // Warp from/towards front or back
        TwoPoint // Warp from/towards front/back or sides
    };

    enum ElevationWarpType
    {
        Pole, // Warp from/towards pole
        Equator // Warp from/towards equator

    };

    AmbisonicWarp (AzimuthWarpType azWarpType = AzimuthWarpType::TwoPoint,
                   ElevationWarpType elWarpType = ElevationWarpType::Pole,
                   float azWarpFactor = 0.0f,
                   float elWarpFactor = 0.0f,
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

            // Get warped angle and de-emphasis gain
            auto [el_warped, el_g] = (_elWarpType == ElevationWarpType::Pole)
                                         ? warpToPole (el_orig, _elWarpFactor)
                                         : warpToEquator (el_orig, _elWarpFactor);

            // Use same warp functions for azimuth, just modify in/output
            float az_warped, az_g;
            if (_azWarpType == AzimuthWarpType::OnePoint)
            {
                auto [az_tmp, az_g] = warpToEquator (az_orig * 0.5f, -_azWarpFactor);
                az_warped = az_tmp * 2.0f;
            }
            else
            {
                float in_sign = (az_orig < 0.0f) ? -1.0f : 1.0f;
                float az_in_tf = (az_orig - 0.5f * juce::MathConstants<float>::pi) * in_sign;

                auto [az_tmp, az_g] = warpToEquator (az_in_tf, _azWarpFactor);
                az_warped = az_tmp + 0.5f * juce::MathConstants<float>::pi * in_sign;
            }

            // Get cartesian coordinates of warped direction
            juce::Vector3D<float> warped_cart =
                Conversions<float>::sphericalToCartesian (az_warped, el_warped);

            // Calculate SH coefficients for warped direction
            SHEval (maxOrder, warped_cart, YH.getRawDataPointer() + p * 64, false);

            // FIXME: Double-check gain
            // for (int r = 0; r < maxOrder; ++r)
            //     YH (p, r) *= el_g * az_g;
        }

        _T = _Y * YH;
    }

    std::tuple<float, float> warpToPole (const float angleInRad, const float alpha)
    {
        float mu = std::sin (angleInRad);

        float warpedAngle = std::asin ((mu - alpha) / (1.0f - alpha * mu));
        float gain = (1.0f - alpha * mu) / std::sqrt (1.0f - alpha * alpha);

        return { warpedAngle, gain };
    }

    std::tuple<float, float> warpToEquator (const float angleInRad, const float alpha)
    {
        float mu = std::sin (angleInRad);
        float g = 1.0f;

        float warpedAngle;

        if (alpha == 0.0f)
            warpedAngle = angleInRad;

        else
        {
            float absalpha = std::abs (alpha);
            g = ((1.0f - absalpha * mu * mu)
                 / std::sqrt ((1.0f - absalpha) * (1.0f + absalpha * mu * mu)));

            if (alpha < 0.0f)
            {
                warpedAngle = std::asin (((absalpha - 1.0f)
                                          + std::sqrt ((absalpha - 1.0f) * (absalpha - 1.0f)
                                                       + 4.0f * absalpha * mu * mu))
                                         / (2.0f * absalpha * mu));
            }
            else
            {
                warpedAngle = std::asin ((1.0f - absalpha) * mu / (1.0f - absalpha * mu * mu));
                g = 1.0f / g;
            }
        }

        return { warpedAngle, g };
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