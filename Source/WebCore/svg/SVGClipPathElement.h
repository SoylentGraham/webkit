/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#include "SVGExternalResourcesRequired.h"
#include "SVGGraphicsElement.h"
#include "SVGUnitTypes.h"

namespace WebCore {

class RenderObject;

class SVGClipPathElement final : public SVGGraphicsElement, public SVGExternalResourcesRequired {
    WTF_MAKE_ISO_ALLOCATED(SVGClipPathElement);
public:
    static Ref<SVGClipPathElement> create(const QualifiedName&, Document&);

    SVGUnitTypes::SVGUnitType clipPathUnits() const { return m_clipPathUnits->currentValue<SVGUnitTypes::SVGUnitType>(); }
    SVGAnimatedEnumeration& clipPathUnitsAnimated() { return m_clipPathUnits; }

private:
    SVGClipPathElement(const QualifiedName&, Document&);

    using AttributeOwnerProxy = SVGAttributeOwnerProxyImpl<SVGClipPathElement, SVGGraphicsElement, SVGExternalResourcesRequired>;
    const SVGAttributeOwnerProxy& attributeOwnerProxy() const final { return m_attributeOwnerProxy; }

    using PropertyRegistry = SVGPropertyOwnerRegistry<SVGClipPathElement, SVGGraphicsElement, SVGExternalResourcesRequired>;
    const SVGPropertyRegistry& propertyRegistry() const final { return m_propertyRegistry; }

    void parseAttribute(const QualifiedName&, const AtomicString&) final;
    void svgAttributeChanged(const QualifiedName&) final;
    void childrenChanged(const ChildChange&) final;

    bool isValid() const final { return SVGTests::isValid(); }
    bool supportsFocus() const final { return false; }
    bool needsPendingResourceHandling() const final { return false; }

    RenderPtr<RenderElement> createElementRenderer(RenderStyle&&, const RenderTreePosition&) final;

    AttributeOwnerProxy m_attributeOwnerProxy { *this };
    PropertyRegistry m_propertyRegistry { *this };
    Ref<SVGAnimatedEnumeration> m_clipPathUnits { SVGAnimatedEnumeration::create(this, SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) };
};

} // namespace WebCore
