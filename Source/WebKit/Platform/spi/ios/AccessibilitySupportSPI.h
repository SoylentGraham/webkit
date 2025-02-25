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

#pragma once

#if HAVE(ACCESSIBILITY_SUPPORT)

#if USE(APPLE_INTERNAL_SDK)

// FIXME (46432011): We shouldn't need to wrap this include in extern "C".
WTF_EXTERN_C_BEGIN
#include <AccessibilitySupport.h>
WTF_EXTERN_C_END

#else

WTF_EXTERN_C_BEGIN

#if PLATFORM(IOS_FAMILY)
extern Boolean _AXSKeyRepeatEnabled();
extern Boolean _AXSApplicationAccessibilityEnabled();
extern CFStringRef kAXSApplicationAccessibilityEnabledNotification;
#endif

#if ENABLE(ACCESSIBILITY_EVENTS)
extern CFStringRef kAXSWebAccessibilityEventsEnabledNotification;
extern Boolean _AXSWebAccessibilityEventsEnabled();
#endif

#if PLATFORM(IOS_FAMILY) && ENABLE(FULL_KEYBOARD_ACCESS)
extern CFStringRef kAXSFullKeyboardAccessEnabledNotification;
extern Boolean _AXSFullKeyboardAccessEnabled();
#endif

WTF_EXTERN_C_END

#endif

#endif // HAVE(ACCESSIBILITY_SUPPORT)
