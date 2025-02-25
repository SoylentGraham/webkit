/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "APIObject.h"
#include "WebsiteAutoplayPolicy.h"
#include "WebsiteAutoplayQuirk.h"
#include "WebsitePopUpPolicy.h"
#include <WebCore/HTTPHeaderField.h>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>

namespace WebKit {
struct WebsitePoliciesData;
}

namespace API {

class WebsiteDataStore;

class WebsitePolicies final : public API::ObjectImpl<API::Object::Type::WebsitePolicies> {
public:
    static Ref<WebsitePolicies> create() { return adoptRef(*new WebsitePolicies); }
    WebsitePolicies();
    ~WebsitePolicies();

    bool contentBlockersEnabled() const { return m_contentBlockersEnabled; }
    void setContentBlockersEnabled(bool enabled) { m_contentBlockersEnabled = enabled; }
    
    OptionSet<WebKit::WebsiteAutoplayQuirk> allowedAutoplayQuirks() const { return m_allowedAutoplayQuirks; }
    void setAllowedAutoplayQuirks(OptionSet<WebKit::WebsiteAutoplayQuirk> quirks) { m_allowedAutoplayQuirks = quirks; }
    
    WebKit::WebsiteAutoplayPolicy autoplayPolicy() const { return m_autoplayPolicy; }
    void setAutoplayPolicy(WebKit::WebsiteAutoplayPolicy policy) { m_autoplayPolicy = policy; }

    const Optional<bool>& deviceOrientationAndMotionAccessState() const { return m_deviceOrientationAndMotionAccessState; }
    void setDeviceOrientationAndMotionAccessState(Optional<bool> state) { m_deviceOrientationAndMotionAccessState = state; }
    
    const Vector<WebCore::HTTPHeaderField>& customHeaderFields() const { return m_customHeaderFields; }
    Vector<WebCore::HTTPHeaderField>&& takeCustomHeaderFields() { return WTFMove(m_customHeaderFields); }
    void setCustomHeaderFields(Vector<WebCore::HTTPHeaderField>&& fields) { m_customHeaderFields = WTFMove(fields); }

    WebKit::WebsitePopUpPolicy popUpPolicy() const { return m_popUpPolicy; }
    void setPopUpPolicy(WebKit::WebsitePopUpPolicy policy) { m_popUpPolicy = policy; }

    WebsiteDataStore* websiteDataStore() const { return m_websiteDataStore.get(); }
    void setWebsiteDataStore(RefPtr<WebsiteDataStore>&&);

    WebKit::WebsitePoliciesData data();

    void setCustomUserAgent(const WTF::String& customUserAgent) { m_customUserAgent = customUserAgent; }
    const WTF::String& customUserAgent() const { return m_customUserAgent; }

    void setCustomJavaScriptUserAgentAsSiteSpecificQuirks(const WTF::String& customUserAgent) { m_customJavaScriptUserAgentAsSiteSpecificQuirks = customUserAgent; }
    const WTF::String& customJavaScriptUserAgentAsSiteSpecificQuirks() const { return m_customJavaScriptUserAgentAsSiteSpecificQuirks; }

    void setCustomNavigatorPlatform(const WTF::String& customNavigatorPlatform) { m_customNavigatorPlatform = customNavigatorPlatform; }
    const WTF::String& customNavigatorPlatform() const { return m_customNavigatorPlatform; }

private:
    WebsitePolicies(bool contentBlockersEnabled, OptionSet<WebKit::WebsiteAutoplayQuirk>, WebKit::WebsiteAutoplayPolicy, Vector<WebCore::HTTPHeaderField>&&, WebKit::WebsitePopUpPolicy, RefPtr<WebsiteDataStore>&&);

    bool m_contentBlockersEnabled { true };
    OptionSet<WebKit::WebsiteAutoplayQuirk> m_allowedAutoplayQuirks;
    WebKit::WebsiteAutoplayPolicy m_autoplayPolicy { WebKit::WebsiteAutoplayPolicy::Default };
    Optional<bool> m_deviceOrientationAndMotionAccessState;
    Vector<WebCore::HTTPHeaderField> m_customHeaderFields;
    WebKit::WebsitePopUpPolicy m_popUpPolicy { WebKit::WebsitePopUpPolicy::Default };
    RefPtr<WebsiteDataStore> m_websiteDataStore;
    WTF::String m_customUserAgent;
    WTF::String m_customJavaScriptUserAgentAsSiteSpecificQuirks;
    WTF::String m_customNavigatorPlatform;
};

} // namespace API
