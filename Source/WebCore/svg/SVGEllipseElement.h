/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2018-2019 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "SVGAnimatedLength.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGGeometryElement.h"
#include "SVGNames.h"

namespace WebCore {

class SVGEllipseElement final : public SVGGeometryElement, public SVGExternalResourcesRequired {
    WTF_MAKE_ISO_ALLOCATED(SVGEllipseElement);
public:
    static Ref<SVGEllipseElement> create(const QualifiedName&, Document&);

    const SVGLengthValue& cx() const { return m_cx.currentValue(attributeOwnerProxy()); }
    const SVGLengthValue& cy() const { return m_cy.currentValue(attributeOwnerProxy()); }
    const SVGLengthValue& rx() const { return m_rx.currentValue(attributeOwnerProxy()); }
    const SVGLengthValue& ry() const { return m_ry.currentValue(attributeOwnerProxy()); }

    RefPtr<SVGAnimatedLength> cxAnimated() { return m_cx.animatedProperty(attributeOwnerProxy()); }
    RefPtr<SVGAnimatedLength> cyAnimated() { return m_cy.animatedProperty(attributeOwnerProxy()); }
    RefPtr<SVGAnimatedLength> rxAnimated() { return m_rx.animatedProperty(attributeOwnerProxy()); }
    RefPtr<SVGAnimatedLength> ryAnimated() { return m_ry.animatedProperty(attributeOwnerProxy()); }

private:
    SVGEllipseElement(const QualifiedName&, Document&);

    using AttributeOwnerProxy = SVGAttributeOwnerProxyImpl<SVGEllipseElement, SVGGeometryElement, SVGExternalResourcesRequired>;
    static AttributeOwnerProxy::AttributeRegistry& attributeRegistry() { return AttributeOwnerProxy::attributeRegistry(); }
    static void registerAttributes();
    const SVGAttributeOwnerProxy& attributeOwnerProxy() const final { return m_attributeOwnerProxy; }
    
    using PropertyRegistry = SVGPropertyOwnerRegistry<SVGEllipseElement, SVGGeometryElement, SVGExternalResourcesRequired>;
    const SVGPropertyRegistry& propertyRegistry() const final { return m_propertyRegistry; }

    static bool isKnownAttribute(const QualifiedName& attributeName)
    {
        return AttributeOwnerProxy::isKnownAttribute(attributeName) || PropertyRegistry::isKnownAttribute(attributeName);
    }

    void parseAttribute(const QualifiedName&, const AtomicString&) final;
    void svgAttributeChanged(const QualifiedName&) final;

    bool isValid() const final { return SVGTests::isValid(); }
    bool selfHasRelativeLengths() const final { return true; }

    RenderPtr<RenderElement> createElementRenderer(RenderStyle&&, const RenderTreePosition&) final;

    AttributeOwnerProxy m_attributeOwnerProxy { *this };
    PropertyRegistry m_propertyRegistry { *this };
    SVGAnimatedLengthAttribute m_cx { LengthModeWidth };
    SVGAnimatedLengthAttribute m_cy { LengthModeHeight };
    SVGAnimatedLengthAttribute m_rx { LengthModeWidth };
    SVGAnimatedLengthAttribute m_ry { LengthModeHeight };
};

} // namespace WebCore
