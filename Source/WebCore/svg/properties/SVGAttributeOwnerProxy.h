/*
 * Copyright (C) 2018 Apple Inc.  All rights reserved.
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

#include "QualifiedName.h"
#include "SVGAnimatedPropertyType.h"
#include <wtf/WeakPtr.h>

namespace WebCore {

class SVGAttribute;
class SVGElement;
class SVGLegacyAnimatedProperty;

class SVGAttributeOwnerProxy {
public:
    SVGAttributeOwnerProxy(SVGElement&);

    virtual ~SVGAttributeOwnerProxy() = default;

    SVGElement& element() const;

    virtual void synchronizeAttributes() const = 0;
    virtual void synchronizeAttribute(const QualifiedName&) const = 0;

    virtual Vector<AnimatedPropertyType> animatedTypes(const QualifiedName&) const = 0;

    virtual RefPtr<SVGLegacyAnimatedProperty> lookupOrCreateAnimatedProperty(const SVGAttribute&) const = 0;
    virtual RefPtr<SVGLegacyAnimatedProperty> lookupAnimatedProperty(const SVGAttribute&) const = 0;
    virtual Vector<RefPtr<SVGLegacyAnimatedProperty>> lookupOrCreateAnimatedProperties(const QualifiedName&) const = 0;

protected:
    WeakPtr<SVGElement> m_element;
};

}
