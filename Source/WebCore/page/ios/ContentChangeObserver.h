/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#if PLATFORM(IOS_FAMILY)

#include "CSSPropertyNames.h"
#include "PlatformEvent.h"
#include "Timer.h"
#include "WKContentObservation.h"
#include <wtf/HashSet.h>

namespace WebCore {

class Animation;
class DOMTimer;
class Document;
class Element;

class ContentChangeObserver {
public:
    ContentChangeObserver(Document&);

    WEBCORE_EXPORT void startContentObservationForDuration(Seconds duration);
    WEBCORE_EXPORT WKContentChange observedContentChange() const;

    void didInstallDOMTimer(const DOMTimer&, Seconds timeout, bool singleShot);
    void didRemoveDOMTimer(const DOMTimer&);

    void didAddTransition(const Element&, const Animation&);
    void didFinishTransition(const Element&, CSSPropertyID);
    void didRemoveTransition(const Element&, CSSPropertyID);

    WEBCORE_EXPORT void willNotProceedWithClick();
    WEBCORE_EXPORT static void didRecognizeLongPress(Frame& mainFrame);
    WEBCORE_EXPORT static void didPreventDefaultForEvent(Frame& mainFrame);

    void didSuspendActiveDOMObjects();
    void willDetachPage();

    class StyleChangeScope {
    public:
        StyleChangeScope(Document&, const Element&);
        ~StyleChangeScope();

    private:
        bool isConsideredClickable() const;

        ContentChangeObserver& m_contentChangeObserver;
        const Element& m_element;
        bool m_wasHidden { false };
        bool m_hadRenderer { false };
    };

    class TouchEventScope {
    public:
        WEBCORE_EXPORT TouchEventScope(Document&, PlatformEvent::Type);
        WEBCORE_EXPORT ~TouchEventScope();
    private:
        ContentChangeObserver& m_contentChangeObserver;
    };

    class MouseMovedScope {
    public:
        WEBCORE_EXPORT MouseMovedScope(Document&);
        WEBCORE_EXPORT ~MouseMovedScope();
    private:
        ContentChangeObserver& m_contentChangeObserver;
    };

    class StyleRecalcScope {
    public:
        StyleRecalcScope(Document&);
        ~StyleRecalcScope();
    private:
        ContentChangeObserver& m_contentChangeObserver;
    };

    class DOMTimerScope {
    public:
        DOMTimerScope(Document*, const DOMTimer&);
        ~DOMTimerScope();
    private:
        ContentChangeObserver* m_contentChangeObserver { nullptr };
        const DOMTimer& m_domTimer;
    };

private:
    void touchEventDidStart(PlatformEvent::Type);
    void touchEventDidFinish();

    void mouseMovedDidStart();
    void mouseMovedDidFinish();

    void didRecognizeLongPress();

    void contentVisibilityDidChange();

    void setShouldObserveDOMTimerScheduling(bool observe) { m_isObservingDOMTimerScheduling = observe; }
    bool isObservingDOMTimerScheduling() const { return m_isObservingDOMTimerScheduling; }
    void setShouldObserveTransitions(bool observe) { m_isObservingTransitions = observe; }
    bool isObservingTransitions() const { return m_isObservingTransitions; }
    bool isObservedPropertyForTransition(CSSPropertyID propertyId) const { return propertyId == CSSPropertyLeft || propertyId == CSSPropertyOpacity; }
    void domTimerExecuteDidStart(const DOMTimer&);
    void domTimerExecuteDidFinish(const DOMTimer&);
    void registerDOMTimer(const DOMTimer& timer) { m_DOMTimerList.add(&timer); }
    void unregisterDOMTimer(const DOMTimer& timer) { m_DOMTimerList.remove(&timer); }
    void clearObservedDOMTimers() { m_DOMTimerList.clear(); }
    void clearObservedTransitions() { m_elementsWithTransition.clear(); }
    bool containsObservedDOMTimer(const DOMTimer& timer) const { return m_DOMTimerList.contains(&timer); }

    void styleRecalcDidStart();
    void styleRecalcDidFinish();
    void setShouldObserveNextStyleRecalc(bool);
    bool isWaitingForStyleRecalc() const { return m_isWaitingForStyleRecalc; }

    bool isObservingContentChanges() const;

    void stopObservingPendingActivities();
    void reset();

    void setHasIndeterminateState();
    void setHasVisibleChangeState();
    void setHasNoChangeState();

    bool hasVisibleChangeState() const { return observedContentChange() == WKContentVisibilityChange; }
    bool hasObservedDOMTimer() const { return !m_DOMTimerList.isEmpty(); }
    bool hasObservedTransition() const { return !m_elementsWithTransition.isEmpty(); }
    bool hasDeterminateState() const;

    void setIsBetweenTouchEndAndMouseMoved(bool isBetween) { m_isBetweenTouchEndAndMouseMoved = isBetween; }
    bool isBetweenTouchEndAndMouseMoved() const { return m_isBetweenTouchEndAndMouseMoved; }

    bool hasPendingActivity() const { return hasObservedDOMTimer() || hasObservedTransition() || m_isWaitingForStyleRecalc || isObservationTimeWindowActive(); }
    bool isObservationTimeWindowActive() const { return m_contentObservationTimer.isActive(); }

    void completeDurationBasedContentObservation();

    enum class Event {
        StartedTouchStartEventDispatching,
        EndedTouchStartEventDispatching,
        WillNotProceedWithClick,
        StartedMouseMovedEventDispatching,
        EndedMouseMovedEventDispatching,
        InstalledDOMTimer,
        RemovedDOMTimer,
        StartedDOMTimerExecution,
        EndedDOMTimerExecution,
        StartedStyleRecalc,
        EndedStyleRecalc,
        AddedTransition,
        EndedTransition,
        CompletedTransition,
        CanceledTransition,
        StartedFixedObservationTimeWindow,
        EndedFixedObservationTimeWindow,
        ContentVisibilityChanged
    };
    void adjustObservedState(Event);

    Document& m_document;
    Timer m_contentObservationTimer;
    HashSet<const DOMTimer*> m_DOMTimerList;
    // FIXME: Move over to WeakHashSet when it starts supporting const.
    HashSet<const Element*> m_elementsWithTransition;
    bool m_touchEventIsBeingDispatched { false };
    bool m_isWaitingForStyleRecalc { false };
    bool m_isInObservedStyleRecalc { false };
    bool m_isObservingDOMTimerScheduling { false };
    bool m_observedDomTimerIsBeingExecuted { false };
    bool m_mouseMovedEventIsBeingDispatched { false };
    bool m_isBetweenTouchEndAndMouseMoved { false };
    bool m_isObservingTransitions { false };
};

inline void ContentChangeObserver::setHasNoChangeState()
{
    WKSetObservedContentChange(WKContentNoChange);
}

inline void ContentChangeObserver::setHasIndeterminateState()
{
    ASSERT(!hasVisibleChangeState());
    WKSetObservedContentChange(WKContentIndeterminateChange);
}

inline void ContentChangeObserver::setHasVisibleChangeState()
{
    WKSetObservedContentChange(WKContentVisibilityChange);
}

inline bool ContentChangeObserver::isObservingContentChanges() const
{
    return m_touchEventIsBeingDispatched
        || m_isBetweenTouchEndAndMouseMoved
        || m_mouseMovedEventIsBeingDispatched
        || m_observedDomTimerIsBeingExecuted
        || m_isInObservedStyleRecalc
        || m_contentObservationTimer.isActive();
    }
}

#endif
