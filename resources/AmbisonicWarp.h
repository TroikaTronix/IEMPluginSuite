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
#include "Conversions.h"
#include "ambisonicTools.h"
#include "efficientSHvanilla.h"
#include "tDesignN10.h"
#include <JuceHeader.h>

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
                   ElevationWarpType elWarpType = ElevationWarpType::Poles,
                   float azWarpFactor = 0.0f,
                   float elWarpFactor = 0.0f)
    {
        _azWarpType = azWarpType;
        _elWarpType = elWarpType;

        juce::dsp::Matrix<float> tmpEncoderMatrix (tDesignN, 64);
        for (int p = 0; p < tDesignN; ++p)
            SHEval (maxOrder,
                    tDesignX[p],
                    tDesignY[p],
                    tDesignZ[p],
                    tmpEncoderMatrix.getRawDataPointer() + p * 64,
                    false);
    }

private:
    const int maxOrder = 7;
    const int maxChannels = squares[maxOrder + 1];

    AzimuthWarpType _azWarpType;
    ElevationWarpType _elWarpType;

    float _azWarpFactor, _elWarpFactor;

    juce::dsp::Matrix<float> decoderMatrix = juce::dsp::Matrix<float> (maxChannels, tDesignN);
    juce::dsp::Matrix<float> warpingMatrix = juce::dsp::Matrix<float> (maxChannels, maxChannels);
};