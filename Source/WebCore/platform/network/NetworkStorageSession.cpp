/*
 * Copyright (C) 2016-2018 Apple Inc.  All rights reserved.
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
#include "NetworkStorageSession.h"

#include "RuntimeApplicationChecks.h"
#include <pal/SessionID.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ProcessPrivilege.h>

#if ENABLE(RESOURCE_LOAD_STATISTICS)
#include "ResourceRequest.h"
#if ENABLE(PUBLIC_SUFFIX_LIST)
#include "PublicSuffix.h"
#endif
#endif

namespace WebCore {

bool NetworkStorageSession::m_processMayUseCookieAPI = false;

bool NetworkStorageSession::processMayUseCookieAPI()
{
    return m_processMayUseCookieAPI;
}

void NetworkStorageSession::permitProcessToUseCookieAPI(bool value)
{
    m_processMayUseCookieAPI = value;
    if (m_processMayUseCookieAPI)
        addProcessPrivilege(ProcessPrivilege::CanAccessRawCookies);
    else
        removeProcessPrivilege(ProcessPrivilege::CanAccessRawCookies);
}

#if ENABLE(RESOURCE_LOAD_STATISTICS)

bool NetworkStorageSession::shouldBlockThirdPartyCookies(const RegistrableDomain& registrableDomain) const
{
    if (registrableDomain.isEmpty())
        return false;

    return m_registrableDomainsToBlockCookieFor.contains(registrableDomain);
}

bool NetworkStorageSession::shouldBlockCookies(const ResourceRequest& request, Optional<uint64_t> frameID, Optional<uint64_t> pageID) const
{
    return shouldBlockCookies(request.firstPartyForCookies(), request.url(), frameID, pageID);
}
    
bool NetworkStorageSession::shouldBlockCookies(const URL& firstPartyForCookies, const URL& resource, Optional<uint64_t> frameID, Optional<uint64_t> pageID) const
{
    RegistrableDomain firstPartyDomain { firstPartyForCookies };
    if (firstPartyDomain.isEmpty())
        return false;

    RegistrableDomain resourceDomain { resource };
    if (resourceDomain.isEmpty())
        return false;

    if (firstPartyDomain == resourceDomain)
        return false;

    if (pageID && hasStorageAccess(resourceDomain, firstPartyDomain, frameID, pageID.value()))
        return false;

    return shouldBlockThirdPartyCookies(resourceDomain);
}

Optional<Seconds> NetworkStorageSession::maxAgeCacheCap(const ResourceRequest& request)
{
    if (m_cacheMaxAgeCapForPrevalentResources && shouldBlockCookies(request, WTF::nullopt, WTF::nullopt))
        return m_cacheMaxAgeCapForPrevalentResources;
    return WTF::nullopt;
}

void NetworkStorageSession::setAgeCapForClientSideCookies(Optional<Seconds> seconds)
{
    m_ageCapForClientSideCookies = seconds;
    m_ageCapForClientSideCookiesShort = seconds ? Seconds { seconds->seconds() / 7. } : seconds;
}

void NetworkStorageSession::setPrevalentDomainsToBlockCookiesFor(const Vector<RegistrableDomain>& domains)
{
    m_registrableDomainsToBlockCookieFor.clear();
    m_registrableDomainsToBlockCookieFor.add(domains.begin(), domains.end());
}

void NetworkStorageSession::removePrevalentDomains(const Vector<RegistrableDomain>& domains)
{
    for (auto& domain : domains)
        m_registrableDomainsToBlockCookieFor.remove(domain);
}

bool NetworkStorageSession::hasStorageAccess(const RegistrableDomain& resourceDomain, const RegistrableDomain& firstPartyDomain, Optional<uint64_t> frameID, uint64_t pageID) const
{
    if (frameID) {
        auto framesGrantedIterator = m_framesGrantedStorageAccess.find(pageID);
        if (framesGrantedIterator != m_framesGrantedStorageAccess.end()) {
            auto it = framesGrantedIterator->value.find(frameID.value());
            if (it != framesGrantedIterator->value.end() && it->value == resourceDomain)
                return true;
        }
    }

    if (!firstPartyDomain.isEmpty()) {
        auto pagesGrantedIterator = m_pagesGrantedStorageAccess.find(pageID);
        if (pagesGrantedIterator != m_pagesGrantedStorageAccess.end()) {
            auto it = pagesGrantedIterator->value.find(firstPartyDomain);
            if (it != pagesGrantedIterator->value.end() && it->value == resourceDomain)
                return true;
        }
    }

    return false;
}

Vector<String> NetworkStorageSession::getAllStorageAccessEntries() const
{
    Vector<String> entries;
    for (auto& innerMap : m_framesGrantedStorageAccess.values()) {
        for (auto& value : innerMap.values())
            entries.append(value.string());
    }
    return entries;
}
    
void NetworkStorageSession::grantStorageAccess(const RegistrableDomain& resourceDomain, const RegistrableDomain& firstPartyDomain, Optional<uint64_t> frameID, uint64_t pageID)
{
    if (!frameID) {
        if (firstPartyDomain.isEmpty())
            return;
        auto pagesGrantedIterator = m_pagesGrantedStorageAccess.find(pageID);
        if (pagesGrantedIterator == m_pagesGrantedStorageAccess.end()) {
            HashMap<RegistrableDomain, RegistrableDomain> entry;
            entry.add(firstPartyDomain, resourceDomain);
            m_pagesGrantedStorageAccess.add(pageID, entry);
        } else {
            auto firstPartyDomainIterator = pagesGrantedIterator->value.find(firstPartyDomain);
            if (firstPartyDomainIterator == pagesGrantedIterator->value.end())
                pagesGrantedIterator->value.add(firstPartyDomain, resourceDomain);
            else
                firstPartyDomainIterator->value = resourceDomain;
        }
        return;
    }

    auto pagesGrantedIterator = m_framesGrantedStorageAccess.find(pageID);
    if (pagesGrantedIterator == m_framesGrantedStorageAccess.end()) {
        HashMap<uint64_t, RegistrableDomain, DefaultHash<uint64_t>::Hash, WTF::UnsignedWithZeroKeyHashTraits<uint64_t>> entry;
        entry.add(frameID.value(), resourceDomain);
        m_framesGrantedStorageAccess.add(pageID, entry);
    } else {
        auto framesGrantedIterator = pagesGrantedIterator->value.find(frameID.value());
        if (framesGrantedIterator == pagesGrantedIterator->value.end())
            pagesGrantedIterator->value.add(frameID.value(), resourceDomain);
        else
            framesGrantedIterator->value = resourceDomain;
    }
}

void NetworkStorageSession::removeStorageAccessForFrame(uint64_t frameID, uint64_t pageID)
{
    auto iteration = m_framesGrantedStorageAccess.find(pageID);
    if (iteration == m_framesGrantedStorageAccess.end())
        return;

    iteration->value.remove(frameID);
}

void NetworkStorageSession::clearPageSpecificDataForResourceLoadStatistics(uint64_t pageID)
{
    m_pagesGrantedStorageAccess.remove(pageID);
    m_framesGrantedStorageAccess.remove(pageID);
    if (!m_navigationWithLinkDecorationTestMode)
        m_navigatedToWithLinkDecorationByPrevalentResource.remove(pageID);
}

void NetworkStorageSession::removeAllStorageAccess()
{
    m_pagesGrantedStorageAccess.clear();
    m_framesGrantedStorageAccess.clear();
}

void NetworkStorageSession::setCacheMaxAgeCapForPrevalentResources(Seconds seconds)
{
    m_cacheMaxAgeCapForPrevalentResources = seconds;
}
    
void NetworkStorageSession::resetCacheMaxAgeCapForPrevalentResources()
{
    m_cacheMaxAgeCapForPrevalentResources = WTF::nullopt;
}

void NetworkStorageSession::committedCrossSiteLoadWithLinkDecoration(const RegistrableDomain& fromDomain, const RegistrableDomain& toDomain, uint64_t pageID)
{
    if (shouldBlockThirdPartyCookies(fromDomain))
        m_navigatedToWithLinkDecorationByPrevalentResource.add(pageID, toDomain);
}

void NetworkStorageSession::resetCrossSiteLoadsWithLinkDecorationForTesting()
{
    m_navigatedToWithLinkDecorationByPrevalentResource.clear();
    m_navigationWithLinkDecorationTestMode = true;
}

Optional<Seconds> NetworkStorageSession::clientSideCookieCap(const RegistrableDomain& firstParty, Optional<uint64_t> pageID) const
{
    if (!m_ageCapForClientSideCookies || !pageID || m_navigatedToWithLinkDecorationByPrevalentResource.isEmpty())
        return m_ageCapForClientSideCookies;

    auto domainIterator = m_navigatedToWithLinkDecorationByPrevalentResource.find(*pageID);
    if (domainIterator == m_navigatedToWithLinkDecorationByPrevalentResource.end())
        return m_ageCapForClientSideCookies;

    if (domainIterator->value == firstParty)
        return m_ageCapForClientSideCookiesShort;

    return m_ageCapForClientSideCookies;
}
#endif // ENABLE(RESOURCE_LOAD_STATISTICS)

}
