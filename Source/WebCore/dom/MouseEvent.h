/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003-2016 Apple Inc. All rights reserved.
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
 *
 */

#pragma once

#include "MouseEventInit.h"
#include "MouseRelatedEvent.h"

#if ENABLE(TOUCH_EVENTS) && PLATFORM(IOS_FAMILY)
#include "PlatformTouchEventIOS.h"
#endif

namespace WebCore {

class DataTransfer;
class Node;
class PlatformMouseEvent;

class MouseEvent : public MouseRelatedEvent {
public:
    WEBCORE_EXPORT static Ref<MouseEvent> create(const AtomicString& type, CanBubble, IsCancelable, IsComposed, MonotonicTime timestamp, RefPtr<WindowProxy>&&, int detail,
        const IntPoint& screenLocation, const IntPoint& windowLocation, const IntPoint& movementDelta, OptionSet<Modifier>, unsigned short button, unsigned short buttons,
        EventTarget* relatedTarget, double force, unsigned short syntheticClickType, DataTransfer* = nullptr, IsSimulated = IsSimulated::No, IsTrusted = IsTrusted::Yes);

    WEBCORE_EXPORT static Ref<MouseEvent> create(const AtomicString& eventType, RefPtr<WindowProxy>&&, const PlatformMouseEvent&, int detail, Node* relatedTarget);

    static Ref<MouseEvent> create(const AtomicString& eventType, CanBubble, IsCancelable, IsComposed, RefPtr<WindowProxy>&&, int detail,
        int screenX, int screenY, int clientX, int clientY, OptionSet<Modifier>, unsigned short button, unsigned short buttons,
        unsigned short syntheticClickType, EventTarget* relatedTarget);

    static Ref<MouseEvent> createForBindings() { return adoptRef(*new MouseEvent); }

    static Ref<MouseEvent> create(const AtomicString& eventType, const MouseEventInit&);

#if ENABLE(TOUCH_EVENTS) && PLATFORM(IOS_FAMILY)
    static Ref<MouseEvent> create(const PlatformTouchEvent&, unsigned touchIndex, Ref<WindowProxy>&&);
#endif

    virtual ~MouseEvent();

    WEBCORE_EXPORT void initMouseEvent(const AtomicString& type, bool canBubble, bool cancelable, RefPtr<WindowProxy>&&,
        int detail, int screenX, int screenY, int clientX, int clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
        unsigned short button, EventTarget* relatedTarget);

    void initMouseEventQuirk(JSC::ExecState&, ScriptExecutionContext&, const AtomicString& type, bool canBubble, bool cancelable, RefPtr<WindowProxy>&&,
        int detail, int screenX, int screenY, int clientX, int clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
        unsigned short button, JSC::JSValue relatedTarget);

    unsigned short button() const { return m_button; }
    unsigned short buttons() const { return m_buttons; }
    unsigned short syntheticClickType() const { return m_syntheticClickType; }
    bool buttonDown() const { return m_buttonDown; }
    EventTarget* relatedTarget() const final { return m_relatedTarget.get(); }
    double force() const { return m_force; }
    void setForce(double force) { m_force = force; }

    WEBCORE_EXPORT RefPtr<Node> toElement() const;
    WEBCORE_EXPORT RefPtr<Node> fromElement() const;

    DataTransfer* dataTransfer() const { return isDragEvent() ? m_dataTransfer.get() : nullptr; }

    static bool canTriggerActivationBehavior(const Event&);

    int which() const final;

protected:
    MouseEvent(const AtomicString& type, CanBubble, IsCancelable, IsComposed, MonotonicTime timestamp, RefPtr<WindowProxy>&&, int detail,
        const IntPoint& screenLocation, const IntPoint& windowLocation, const IntPoint& movementDelta, OptionSet<Modifier>, unsigned short button, unsigned short buttons,
        EventTarget* relatedTarget, double force, unsigned short syntheticClickType, DataTransfer*, IsSimulated, IsTrusted);

    MouseEvent(const AtomicString& type, CanBubble, IsCancelable, IsComposed, RefPtr<WindowProxy>&&, int detail,
        const IntPoint& screenLocation, const IntPoint& clientLocation, OptionSet<Modifier>, unsigned short button, unsigned short buttons,
        unsigned short syntheticClickType, EventTarget* relatedTarget);

    MouseEvent(const AtomicString& type, const MouseEventInit&);

    MouseEvent();

private:
    bool isMouseEvent() const final;
    EventInterface eventInterface() const override;

    bool isDragEvent() const;

    void setRelatedTarget(EventTarget& relatedTarget) final { m_relatedTarget = &relatedTarget; }

    unsigned short m_button { 0 };
    unsigned short m_buttons { 0 };
    unsigned short m_syntheticClickType { 0 };
    bool m_buttonDown { false };
    RefPtr<EventTarget> m_relatedTarget;
    double m_force { 0 };
    RefPtr<DataTransfer> m_dataTransfer;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_EVENT(MouseEvent)
