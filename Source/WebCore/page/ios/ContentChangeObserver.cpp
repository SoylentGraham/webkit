/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ContentChangeObserver.h"

#if PLATFORM(IOS_FAMILY)
#include "Chrome.h"
#include "ChromeClient.h"
#include "DOMTimer.h"
#include "Document.h"
#include "HTMLImageElement.h"
#include "Logging.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "RenderDescendantIterator.h"
#include "Settings.h"

namespace WebCore {

static const Seconds maximumDelayForTimers { 300_ms };
static const Seconds maximumDelayForTransitions { 300_ms };

static bool isConsideredHidden(const Element& element)
{
    if (!element.renderStyle())
        return true;

    auto& style = *element.renderStyle();
    if (style.display() == DisplayType::None)
        return true;

    if (style.visibility() == Visibility::Hidden)
        return true;

    if (!style.opacity())
        return true;

    auto width = style.logicalWidth();
    auto height = style.logicalHeight();
    if ((width.isFixed() && !width.value()) || (height.isFixed() && !height.value()))
        return true;

    auto top = style.logicalTop();
    auto left = style.logicalLeft();
    // FIXME: This is trying to check if the element is outside of the viewport. This is incorrect for many reasons.
    if (left.isFixed() && width.isFixed() && -left.value() >= width.value())
        return true;
    if (top.isFixed() && height.isFixed() && -top.value() >= height.value())
        return true;

    // It's a common technique used to position content offscreen.
    if (style.hasOutOfFlowPosition() && left.isFixed() && left.value() <= -999)
        return true;

    // FIXME: Check for other cases like zero height with overflow hidden.
    auto maxHeight = style.maxHeight();
    if (maxHeight.isFixed() && !maxHeight.value())
        return true;

    return false;
}

ContentChangeObserver::ContentChangeObserver(Document& document)
    : m_document(document)
    , m_contentObservationTimer([this] { completeDurationBasedContentObservation(); })
{
}

static void willNotProceedWithClick(Frame& mainFrame)
{
    for (auto* frame = &mainFrame; frame; frame = frame->tree().traverseNext()) {
        if (auto* document = frame->document())
            document->contentChangeObserver().willNotProceedWithClick();
    }
}

void ContentChangeObserver::didRecognizeLongPress(Frame& mainFrame)
{
    LOG(ContentObservation, "didRecognizeLongPress: cancel ongoing content change observing.");
    WebCore::willNotProceedWithClick(mainFrame);
}

void ContentChangeObserver::didPreventDefaultForEvent(Frame& mainFrame)
{
    LOG(ContentObservation, "didPreventDefaultForEvent: cancel ongoing content change observing.");
    WebCore::willNotProceedWithClick(mainFrame);
}

void ContentChangeObserver::startContentObservationForDuration(Seconds duration)
{
    if (!m_document.settings().contentChangeObserverEnabled())
        return;
    ASSERT(!hasVisibleChangeState());
    LOG_WITH_STREAM(ContentObservation, stream << "startContentObservationForDuration: start observing the content for " << duration.milliseconds() << "ms");
    adjustObservedState(Event::StartedFixedObservationTimeWindow);
    m_contentObservationTimer.startOneShot(duration);
}

void ContentChangeObserver::completeDurationBasedContentObservation()
{
    LOG_WITH_STREAM(ContentObservation, stream << "completeDurationBasedContentObservation: complete duration based content observing ");
    adjustObservedState(Event::EndedFixedObservationTimeWindow);
}

void ContentChangeObserver::didAddTransition(const Element& element, const Animation& transition)
{
    if (!m_document.settings().contentChangeObserverEnabled())
        return;
    if (hasVisibleChangeState())
        return;
    if (!isObservingTransitions())
        return;
    if (!transition.isDurationSet() || !transition.isPropertySet())
        return;
    if (!isObservedPropertyForTransition(transition.property()))
        return;
    auto transitionEnd = Seconds { transition.duration() + std::max<double>(0, transition.isDelaySet() ? transition.delay() : 0) };
    if (transitionEnd > maximumDelayForTransitions)
        return;
    if (!isConsideredHidden(element))
        return;
    // In case of multiple transitions, the first tranistion wins (and it has to produce a visible content change in order to show up as hover).
    if (m_elementsWithTransition.contains(&element))
        return;
    LOG_WITH_STREAM(ContentObservation, stream << "didAddTransition: transition created on " << &element << " (" << transitionEnd.milliseconds() << "ms).");

    m_elementsWithTransition.add(&element);
    adjustObservedState(Event::AddedTransition);
}

void ContentChangeObserver::didFinishTransition(const Element& element, CSSPropertyID propertyID)
{
    if (!isObservedPropertyForTransition(propertyID))
        return;
    if (!m_elementsWithTransition.take(&element))
        return;
    LOG_WITH_STREAM(ContentObservation, stream << "didFinishTransition: transition finished (" << &element << ").");

    adjustObservedState(isConsideredHidden(element) ? Event::EndedTransition : Event::CompletedTransition);
}

void ContentChangeObserver::didRemoveTransition(const Element& element, CSSPropertyID propertyID)
{
    if (!isObservedPropertyForTransition(propertyID))
        return;
    if (!m_elementsWithTransition.take(&element))
        return;
    LOG_WITH_STREAM(ContentObservation, stream << "didRemoveTransition: transition got interrupted (" << &element << ").");

    adjustObservedState(Event::CanceledTransition);
}

void ContentChangeObserver::didInstallDOMTimer(const DOMTimer& timer, Seconds timeout, bool singleShot)
{
    if (!m_document.settings().contentChangeObserverEnabled())
        return;
    if (m_document.activeDOMObjectsAreSuspended())
        return;
    if (timeout > maximumDelayForTimers || !singleShot)
        return;
    if (!isObservingDOMTimerScheduling())
        return;
    if (hasVisibleChangeState())
        return;
    LOG_WITH_STREAM(ContentObservation, stream << "didInstallDOMTimer: register this timer: (" << &timer << ") and observe when it fires.");

    registerDOMTimer(timer);
    adjustObservedState(Event::InstalledDOMTimer);
}

void ContentChangeObserver::didRemoveDOMTimer(const DOMTimer& timer)
{
    if (!containsObservedDOMTimer(timer))
        return;
    LOG_WITH_STREAM(ContentObservation, stream << "removeDOMTimer: remove registered timer (" << &timer << ")");

    unregisterDOMTimer(timer);
    adjustObservedState(Event::RemovedDOMTimer);
}

void ContentChangeObserver::willNotProceedWithClick()
{
    LOG(ContentObservation, "willNotProceedWithClick: click will not happen.");
    adjustObservedState(Event::WillNotProceedWithClick);
}

void ContentChangeObserver::domTimerExecuteDidStart(const DOMTimer& timer)
{
    if (!containsObservedDOMTimer(timer))
        return;
    LOG_WITH_STREAM(ContentObservation, stream << "startObservingDOMTimerExecute: start observing (" << &timer << ") timer callback.");

    m_observedDomTimerIsBeingExecuted = true;
    adjustObservedState(Event::StartedDOMTimerExecution);
}

void ContentChangeObserver::domTimerExecuteDidFinish(const DOMTimer& timer)
{
    if (!m_observedDomTimerIsBeingExecuted)
        return;
    LOG_WITH_STREAM(ContentObservation, stream << "stopObservingDOMTimerExecute: stop observing (" << &timer << ") timer callback.");

    m_observedDomTimerIsBeingExecuted = false;
    unregisterDOMTimer(timer);
    adjustObservedState(Event::EndedDOMTimerExecution);
}

void ContentChangeObserver::styleRecalcDidStart()
{
    if (!isWaitingForStyleRecalc())
        return;
    LOG(ContentObservation, "startObservingStyleRecalc: start observing style recalc.");

    m_isInObservedStyleRecalc = true;
    adjustObservedState(Event::StartedStyleRecalc);
}

void ContentChangeObserver::styleRecalcDidFinish()
{
    if (!m_isInObservedStyleRecalc)
        return;
    LOG(ContentObservation, "stopObservingStyleRecalc: stop observing style recalc");

    m_isInObservedStyleRecalc = false;
    adjustObservedState(Event::EndedStyleRecalc);
}

void ContentChangeObserver::stopObservingPendingActivities()
{
    setShouldObserveNextStyleRecalc(false);
    setShouldObserveDOMTimerScheduling(false);
    setShouldObserveTransitions(false);
    clearObservedDOMTimers();
    clearObservedTransitions();
}

void ContentChangeObserver::reset()
{
    stopObservingPendingActivities();
    setHasNoChangeState();
    setIsBetweenTouchEndAndMouseMoved(false);

    m_touchEventIsBeingDispatched = false;
    m_isInObservedStyleRecalc = false;
    m_observedDomTimerIsBeingExecuted = false;
    m_mouseMovedEventIsBeingDispatched = false;

    m_contentObservationTimer.stop();
}

void ContentChangeObserver::didSuspendActiveDOMObjects()
{
    LOG(ContentObservation, "didSuspendActiveDOMObjects");
    reset();
}

void ContentChangeObserver::willDetachPage()
{
    LOG(ContentObservation, "willDetachPage");
    reset();
}

void ContentChangeObserver::contentVisibilityDidChange()
{
    LOG(ContentObservation, "contentVisibilityDidChange: visible content change did happen.");
    adjustObservedState(Event::ContentVisibilityChanged);
}

void ContentChangeObserver::touchEventDidStart(PlatformEvent::Type eventType)
{
#if ENABLE(TOUCH_EVENTS)
    if (!m_document.settings().contentChangeObserverEnabled())
        return;
    if (eventType != PlatformEvent::Type::TouchStart)
        return;
    LOG(ContentObservation, "touchEventDidStart: touch start event started.");
    m_touchEventIsBeingDispatched = true;
    adjustObservedState(Event::StartedTouchStartEventDispatching);
#else
    UNUSED_PARAM(eventType);
#endif
}

void ContentChangeObserver::touchEventDidFinish()
{
#if ENABLE(TOUCH_EVENTS)
    if (!m_touchEventIsBeingDispatched)
        return;
    ASSERT(m_document.settings().contentChangeObserverEnabled());
    LOG(ContentObservation, "touchEventDidFinish: touch start event finished.");
    m_touchEventIsBeingDispatched = false;
    adjustObservedState(Event::EndedTouchStartEventDispatching);
#endif
}

void ContentChangeObserver::mouseMovedDidStart()
{
    if (!m_document.settings().contentChangeObserverEnabled())
        return;
    LOG(ContentObservation, "mouseMovedDidStart: mouseMoved started.");
    m_mouseMovedEventIsBeingDispatched = true;
    adjustObservedState(Event::StartedMouseMovedEventDispatching);
}

void ContentChangeObserver::mouseMovedDidFinish()
{
    if (!m_mouseMovedEventIsBeingDispatched)
        return;
    ASSERT(m_document.settings().contentChangeObserverEnabled());
    LOG(ContentObservation, "mouseMovedDidFinish: mouseMoved finished.");
    adjustObservedState(Event::EndedMouseMovedEventDispatching);
    m_mouseMovedEventIsBeingDispatched = false;
}

WKContentChange ContentChangeObserver::observedContentChange() const
{
    return WKObservedContentChange();
}

void ContentChangeObserver::setShouldObserveNextStyleRecalc(bool shouldObserve)
{
    if (shouldObserve)
        LOG(ContentObservation, "Wait until next style recalc fires.");
    m_isWaitingForStyleRecalc = shouldObserve;
}

bool ContentChangeObserver::hasDeterminateState() const
{
    if (hasVisibleChangeState())
        return true;
    return observedContentChange() == WKContentNoChange && !hasPendingActivity();
}

void ContentChangeObserver::adjustObservedState(Event event)
{
    auto resetToStartObserving = [&] {
        setHasNoChangeState();
        clearObservedDOMTimers();
        clearObservedTransitions();
        setIsBetweenTouchEndAndMouseMoved(false);
        setShouldObserveNextStyleRecalc(false);
        setShouldObserveDOMTimerScheduling(false);
        setShouldObserveTransitions(false);
        ASSERT(!m_isInObservedStyleRecalc);
        ASSERT(!m_observedDomTimerIsBeingExecuted);
    };

    auto adjustStateAndNotifyContentChangeIfNeeded = [&] {
        // Demote to "no change" when there's no pending activity anymore.
        if (observedContentChange() == WKContentIndeterminateChange && !hasPendingActivity())
            setHasNoChangeState();

        // Do not notify the client unless we couldn't make the decision synchronously.
        if (m_mouseMovedEventIsBeingDispatched) {
            LOG(ContentObservation, "adjustStateAndNotifyContentChangeIfNeeded: in mouseMoved call. No need to notify the client.");
            return;
        }
        if (isBetweenTouchEndAndMouseMoved()) {
            LOG(ContentObservation, "adjustStateAndNotifyContentChangeIfNeeded: Not reached mouseMoved yet. No need to notify the client.");
            return;
        }
        if (!hasDeterminateState()) {
            LOG(ContentObservation, "adjustStateAndNotifyContentChangeIfNeeded: not in a determined state yet.");
            return;
        }
        LOG_WITH_STREAM(ContentObservation, stream << "adjustStateAndNotifyContentChangeIfNeeded: sending observedContentChange ->" << observedContentChange());
        ASSERT(m_document.page());
        ASSERT(m_document.frame());
        m_document.page()->chrome().client().observedContentChange(*m_document.frame());
    };

    switch (event) {
    case Event::StartedTouchStartEventDispatching:
        resetToStartObserving();
        setShouldObserveDOMTimerScheduling(true);
        setShouldObserveTransitions(true);
        break;
    case Event::EndedTouchStartEventDispatching:
        setShouldObserveDOMTimerScheduling(false);
        setShouldObserveTransitions(false);
        setIsBetweenTouchEndAndMouseMoved(true);
        break;
    case Event::WillNotProceedWithClick:
        reset();
        break;
    case Event::StartedMouseMovedEventDispatching:
        ASSERT(!m_document.hasPendingStyleRecalc());
        if (!isBetweenTouchEndAndMouseMoved())
            resetToStartObserving();
        setIsBetweenTouchEndAndMouseMoved(false);
        setShouldObserveDOMTimerScheduling(!hasVisibleChangeState());
        setShouldObserveTransitions(!hasVisibleChangeState());
        break;
    case Event::EndedMouseMovedEventDispatching:
        setShouldObserveDOMTimerScheduling(false);
        setShouldObserveTransitions(false);
        break;
    case Event::StartedStyleRecalc:
        setShouldObserveNextStyleRecalc(false);
        FALLTHROUGH;
    case Event::StartedDOMTimerExecution:
        ASSERT(isObservationTimeWindowActive() || observedContentChange() == WKContentIndeterminateChange);
        break;
    case Event::InstalledDOMTimer:
    case Event::StartedFixedObservationTimeWindow:
    case Event::AddedTransition:
        ASSERT(!hasVisibleChangeState());
        setHasIndeterminateState();
        break;
    case Event::EndedDOMTimerExecution:
        setShouldObserveNextStyleRecalc(m_document.hasPendingStyleRecalc());
        FALLTHROUGH;
    case Event::EndedStyleRecalc:
    case Event::RemovedDOMTimer:
    case Event::CanceledTransition:
        if (!isObservationTimeWindowActive())
            adjustStateAndNotifyContentChangeIfNeeded();
        break;
    case Event::EndedTransition:
        // onAnimationEnd can be called while in the middle of resolving the document (synchronously) or
        // asynchronously right before the style update is issued. It also means we don't know whether this animation ends up producing visible content yet. 
        if (m_document.inStyleRecalc()) {
            // We need to start observing this style change synchronously.
            m_isInObservedStyleRecalc = true;
        } else
            setShouldObserveNextStyleRecalc(true);
        break;
    case Event::CompletedTransition:
        // Set visibility flag on and report visible change synchronously or asynchronously depending whether we are in the middle of style recalc.
        contentVisibilityDidChange();
        if (m_document.inStyleRecalc())
            m_isInObservedStyleRecalc = true;
        else if (!isObservationTimeWindowActive())
            adjustStateAndNotifyContentChangeIfNeeded();
        break;
    case Event::EndedFixedObservationTimeWindow:
        adjustStateAndNotifyContentChangeIfNeeded();
        break;
    case Event::ContentVisibilityChanged:
        setHasVisibleChangeState();
        // Stop pending activities. We don't need to observe them anymore.
        stopObservingPendingActivities();
        break;
    }
}

ContentChangeObserver::StyleChangeScope::StyleChangeScope(Document& document, const Element& element)
    : m_contentChangeObserver(document.contentChangeObserver())
    , m_element(element)
    , m_hadRenderer(element.renderer())
{
    if (m_contentChangeObserver.isObservingContentChanges() && !m_contentChangeObserver.hasVisibleChangeState())
        m_wasHidden = isConsideredHidden(m_element);
}

ContentChangeObserver::StyleChangeScope::~StyleChangeScope()
{
    auto changedFromHiddenToVisible = [&] {
        return m_wasHidden && !isConsideredHidden(m_element);
    };

    if (changedFromHiddenToVisible() && isConsideredClickable())
        m_contentChangeObserver.contentVisibilityDidChange();
}

bool ContentChangeObserver::StyleChangeScope::isConsideredClickable() const
{
    if (m_element.isInUserAgentShadowTree())
        return false;

    auto& element = const_cast<Element&>(m_element);
    if (is<HTMLImageElement>(element)) {
        // This is required to avoid HTMLImageElement's touch callout override logic. See rdar://problem/48937767.
        return element.Element::willRespondToMouseClickEvents();
    }

    auto willRespondToMouseClickEvents = element.willRespondToMouseClickEvents();
    if (!m_hadRenderer || willRespondToMouseClickEvents)
        return willRespondToMouseClickEvents;
    // In case when the visible content already had renderers it's not sufficient to check the "newly visible" element only since it might just be the container for the clickable content.  
    ASSERT(m_element.renderer());
    for (auto& descendant : descendantsOfType<RenderElement>(*element.renderer())) {
        if (descendant.element()->willRespondToMouseClickEvents())
            return true;
    }
    return false;
}

#if ENABLE(TOUCH_EVENTS)
ContentChangeObserver::TouchEventScope::TouchEventScope(Document& document, PlatformEvent::Type eventType)
    : m_contentChangeObserver(document.contentChangeObserver())
{
    m_contentChangeObserver.touchEventDidStart(eventType);
}

ContentChangeObserver::TouchEventScope::~TouchEventScope()
{
    m_contentChangeObserver.touchEventDidFinish();
}
#endif

ContentChangeObserver::MouseMovedScope::MouseMovedScope(Document& document)
    : m_contentChangeObserver(document.contentChangeObserver())
{
    m_contentChangeObserver.mouseMovedDidStart();
}

ContentChangeObserver::MouseMovedScope::~MouseMovedScope()
{
    m_contentChangeObserver.mouseMovedDidFinish();
}

ContentChangeObserver::StyleRecalcScope::StyleRecalcScope(Document& document)
    : m_contentChangeObserver(document.contentChangeObserver())
{
    m_contentChangeObserver.styleRecalcDidStart();
}

ContentChangeObserver::StyleRecalcScope::~StyleRecalcScope()
{
    m_contentChangeObserver.styleRecalcDidFinish();
}

ContentChangeObserver::DOMTimerScope::DOMTimerScope(Document* document, const DOMTimer& domTimer)
    : m_contentChangeObserver(document ? &document->contentChangeObserver() : nullptr)
    , m_domTimer(domTimer)
{
    if (m_contentChangeObserver)
        m_contentChangeObserver->domTimerExecuteDidStart(m_domTimer);
}

ContentChangeObserver::DOMTimerScope::~DOMTimerScope()
{
    if (m_contentChangeObserver)
        m_contentChangeObserver->domTimerExecuteDidFinish(m_domTimer);
}

}

#endif // PLATFORM(IOS_FAMILY)
