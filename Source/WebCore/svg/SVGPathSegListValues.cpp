/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#include "config.h"
#include "SVGPathSegListValues.h"

#include "SVGPathElement.h"
#include "SVGPathSegWithContext.h"
#include "SVGPathUtilities.h"

namespace WebCore {

String SVGPathSegListValues::valueAsString() const
{
    String pathString;
    buildStringFromSVGPathSegListValues(*this, pathString, UnalteredParsing);
    return pathString;
}

void SVGPathSegListValues::commitChange(SVGElement& contextElement, ListModification listModification)
{
    downcast<SVGPathElement>(contextElement).pathSegListChanged(m_role, listModification);
}

void SVGPathSegListValues::clearItemContextAndRole(unsigned index)
{
    auto& item = at(index);
    static_cast<SVGPathSegWithContext&>(*item).setContextAndRole(nullptr, PathSegUndefinedRole);
}
    
void SVGPathSegListValues::clearContextAndRoles()
{
    auto count = size();
    while (count--)
        clearItemContextAndRole(count);
}
    
}
