/*
 * Copyright (C) 2018-2019 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "SVGAngle.h"
#include "SVGAnimatedDecoratedProperty.h"
#include "SVGAnimatedLength.h"
#include "SVGAnimatedLengthList.h"
#include "SVGAnimatedPrimitiveProperty.h"
#include "SVGAnimatedPropertyList.h"
#include "SVGAnimatedTransformList.h"
#include "SVGAnimatedValueProperty.h"
#include "SVGDecoratedEnumeration.h"
#include "SVGMarkerTypes.h"
#include "SVGNumberList.h"
#include "SVGPointList.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGRect.h"

namespace WebCore {

using SVGAnimatedBoolean = SVGAnimatedPrimitiveProperty<bool>;
using SVGAnimatedInteger = SVGAnimatedPrimitiveProperty<int>;
using SVGAnimatedNumber = SVGAnimatedPrimitiveProperty<float>;
using SVGAnimatedString = SVGAnimatedPrimitiveProperty<String>;

using SVGAnimatedEnumeration = SVGAnimatedDecoratedProperty<SVGDecoratedEnumeration, unsigned>;

using SVGAnimatedAngle = SVGAnimatedValueProperty<SVGAngle>;
using SVGAnimatedRect = SVGAnimatedValueProperty<SVGRect>;
using SVGAnimatedPreserveAspectRatio = SVGAnimatedValueProperty<SVGPreserveAspectRatio>;

using SVGAnimatedNumberList = SVGAnimatedPropertyList<SVGNumberList>;
using SVGAnimatedPointList = SVGAnimatedPropertyList<SVGPointList>;

class SVGAnimatedOrientType : public SVGAnimatedEnumeration {
public:
    using SVGAnimatedEnumeration::SVGAnimatedEnumeration;

    static Ref<SVGAnimatedOrientType> create(SVGElement* contextElement, SVGMarkerOrientType baseValue)
    {
        return SVGAnimatedEnumeration::create<SVGMarkerOrientType, SVGAnimatedOrientType>(contextElement, baseValue);
    }
};

}
