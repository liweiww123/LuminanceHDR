/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

//! \file FloatRgbToQRgb.cpp
//! \brief This file creates common routines for mapping float RGB values into
//! 8-bits or 16-bits integer RGB (QRgb or quint16)
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \since Luminance HDR 2.3.0-beta1

#include "FloatRgbToQRgb.h"

// #include "arch/math.h"
// #include "Libpfs/vex/sse.h"

#include <algorithm>
#include <cassert>
#include <cmath>

// #undef LUMINANCE_USE_SSE

namespace
{
template<class O_>
inline
O_ scaleAndRound(float value, float MIN_, float MAX_)
{
    value *= MAX_;
    value = std::min(value, MAX_);
    value = std::max(value, MIN_);
    value += 0.5f;

    return static_cast<O_>(value);
}

const float& clamp(const float& value, const float& min_, const float& max_)
{
    if ( value > max_ ) return max_;
    if ( value < min_ ) return min_;
    return value;
}

// useful data structure to implement a triple of float as RGB pixel


const float GAMMA_1_4 = 1.0f/1.4f;
const float GAMMA_1_8 = 1.0f/1.8f;
const float GAMMA_2_2 = 1.0f/2.2f;
const float GAMMA_2_6 = 1.0f/2.6f;
}

FloatRgbToQRgb::FloatRgbToQRgb(float minValue, float maxValue,
                       LumMappingMethod mappingMethod)
        : m_MinValue(minValue)
        , m_MaxValue(maxValue)
        , m_Range(maxValue - minValue)
        , m_LogRange( log2f(maxValue/minValue) )
    {
        setMappingMethod( mappingMethod );
    }

FloatRgbToQRgb::~FloatRgbToQRgb()
{}

void FloatRgbToQRgb::setMinMax(float min, float max)
{
    m_MinValue      = min;
    m_MaxValue      = max;
    m_Range         = max - min;
    m_LogRange      = log2f(max/min);
}

LumMappingMethod FloatRgbToQRgb::getMappingMethod() const
{
    return m_MappingMethod;
}

//float FloatRgbToQRgb::getMinLuminance() const
//{
//    return m_Pimpl->m_MinValue;
//}

//float FloatRgbToQRgb::getMaxLuminance() const
//{
//    return m_Pimpl->m_MaxValue;
//}

void FloatRgbToQRgb::setMappingMethod(LumMappingMethod method)
{
    m_MappingMethod = method;

    switch ( m_MappingMethod )
    {
    case MAP_LINEAR:
        m_MappingFunc = &FloatRgbToQRgb::mappingLinear;
        break;
    case MAP_GAMMA1_4:
        m_MappingFunc = &FloatRgbToQRgb::mappingGamma14;
        break;
    case MAP_GAMMA1_8:
        m_MappingFunc = &FloatRgbToQRgb::mappingGamma18;
        break;
    case MAP_GAMMA2_6:
        m_MappingFunc = &FloatRgbToQRgb::mappingGamma26;
        break;
    case MAP_LOGARITHMIC:
        m_MappingFunc = &FloatRgbToQRgb::mappingLog;
        break;
    default:
    case MAP_GAMMA2_2:
        m_MappingFunc = &FloatRgbToQRgb::mappingGamma22;
        break;
    }
}

FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::buildRgb(float r, float g, float b) const
{
    return FloatRgbToQRgb::RgbF3(clamp((r - m_MinValue)/m_Range, 0.f, 1.f),
                                 clamp((g - m_MinValue)/m_Range, 0.f, 1.f),
                                 clamp((b - m_MinValue)/m_Range, 0.f, 1.f));
}

FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::mappingLinear(float r, float g, float b) const
{
    return buildRgb(r,g,b);
}

FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::mappingGamma14(float r, float g, float b) const
{
    RgbF3 pixel = buildRgb(r,g,b);

    pixel.red = powf(pixel.red, GAMMA_1_4);
    pixel.green = powf(pixel.green, GAMMA_1_4);
    pixel.blue = powf(pixel.blue, GAMMA_1_4);

    return pixel;
}

FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::mappingGamma18(float r, float g, float b) const
{
    RgbF3 pixel = buildRgb(r,g,b);

    pixel.red = powf(pixel.red, GAMMA_1_8);
    pixel.green = powf(pixel.green, GAMMA_1_8);
    pixel.blue = powf(pixel.blue, GAMMA_1_8);

    return pixel;
}

FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::mappingGamma22(float r, float g, float b) const
{
    RgbF3 pixel = buildRgb(r,g,b);

    pixel.red = powf(pixel.red, GAMMA_2_2);
    pixel.green = powf(pixel.green, GAMMA_2_2);
    pixel.blue = powf(pixel.blue, GAMMA_2_2);

    return pixel;
}

FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::mappingGamma26(float r, float g, float b) const
{
    RgbF3 pixel = buildRgb(r,g,b);

    // I have problems with Clang++ in using the powf function
    pixel.red = powf(pixel.red, GAMMA_2_6);
    pixel.green = powf(pixel.green, GAMMA_2_6);
    pixel.blue = powf(pixel.blue, GAMMA_2_6);

    return pixel;
}

FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::mappingLog(float r, float g, float b) const
{
    return RgbF3(log2f(r/m_MinValue)/m_LogRange,
                 log2f(g/m_MinValue)/m_LogRange,
                 log2f(b/m_MinValue)/m_LogRange);
}

inline
FloatRgbToQRgb::RgbF3 FloatRgbToQRgb::get(float r, float g, float b) const
{
    return (this->*m_MappingFunc)(r, g, b);
}

void FloatRgbToQRgb::toQRgb(float r, float g, float b, QRgb& qrgb) const
{
    RgbF3 rgb = get(r,g,b);

    qrgb = qRgb( scaleAndRound<int>(rgb.red, 0.f, 255.f),
                 scaleAndRound<int>(rgb.green, 0.f, 255.f),
                 scaleAndRound<int>(rgb.blue, 0.f, 255.f) );
}

void FloatRgbToQRgb::toUint8(float rI, float gI, float bI,
                             uint8_t& rO, uint8_t& gO, uint8_t& bO) const
{
    RgbF3 rgb = get(rI, gI, bI);

    rO = scaleAndRound<uint8_t>(rgb.red, 0.f, 255.f);
    gO = scaleAndRound<uint8_t>(rgb.green, 0.f, 255.f);
    bO = scaleAndRound<uint8_t>(rgb.blue, 0.f, 255.f);
}

void FloatRgbToQRgb::toFloat(float rI, float gI, float bI,
                             float& rO, float& gO, float& bO) const
{
    RgbF3 rgb = get(rI, gI, bI);

    assert(rgb.red <= 1.0f);
    assert(rgb.red >= 0.0f);
    assert(rgb.green >= 0.0f);
    assert(rgb.green <= 1.0f);
    assert(rgb.blue >= 0.0f);
    assert(rgb.blue <= 1.0f);

    rO = rgb.red;
    gO = rgb.green;
    bO = rgb.blue;
}

void FloatRgbToQRgb::toUint16(float r, float g, float b,
                              quint16& red, quint16& green, quint16& blue) const
{
    RgbF3 rgb = get(r,g,b);

    red = scaleAndRound<quint16>(rgb.red, 0.f, 65535.f);
    green = scaleAndRound<quint16>(rgb.green, 0.f, 65535.f);
    blue = scaleAndRound<quint16>(rgb.blue, 0.f, 65535.f);
}
