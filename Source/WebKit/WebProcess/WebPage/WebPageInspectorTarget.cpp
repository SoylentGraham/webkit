/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebPageInspectorTarget.h"

#include "WebPage.h"
#include <WebCore/InspectorController.h>
#include <WebCore/Page.h>

namespace WebKit {

using namespace Inspector;

WebPageInspectorTarget::WebPageInspectorTarget(WebPage& page)
    : m_page(page)
{
}

String WebPageInspectorTarget::identifier() const
{
    return makeString("page-", m_page.pageID());
}

void WebPageInspectorTarget::connect(Inspector::FrontendChannel& channel)
{
    if (m_page.corePage())
        m_page.corePage()->inspectorController().connectFrontend(channel);
}

void WebPageInspectorTarget::disconnect(Inspector::FrontendChannel& channel)
{
    if (m_page.corePage())
        m_page.corePage()->inspectorController().disconnectFrontend(channel);
}

void WebPageInspectorTarget::sendMessageToTargetBackend(const String& message)
{
    if (m_page.corePage())
        m_page.corePage()->inspectorController().dispatchMessageFromFrontend(message);
}

} // namespace WebKit
