/*
 * Copyright (C) 2019 Apple Inc. All Rights Reserved.
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
#include "RegExpGlobalData.h"

namespace JSC {

void RegExpGlobalData::visitAggregate(SlotVisitor& visitor)
{
    m_cachedResult.visitAggregate(visitor);
}

JSValue RegExpGlobalData::getBackref(ExecState* exec, JSGlobalObject* owner, unsigned i)
{
    VM& vm = exec->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    JSArray* array = m_cachedResult.lastResult(exec, owner);
    RETURN_IF_EXCEPTION(scope, { });

    if (i < array->length()) {
        JSValue result = JSValue(array).get(exec, i);
        RETURN_IF_EXCEPTION(scope, { });
        ASSERT(result.isString() || result.isUndefined());
        if (!result.isUndefined())
            return result;
    }
    return jsEmptyString(&vm);
}

JSValue RegExpGlobalData::getLastParen(ExecState* exec, JSGlobalObject* owner)
{
    VM& vm = exec->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    JSArray* array = m_cachedResult.lastResult(exec, owner);
    RETURN_IF_EXCEPTION(scope, { });

    unsigned length = array->length();
    if (length > 1) {
        JSValue result = JSValue(array).get(exec, length - 1);
        RETURN_IF_EXCEPTION(scope, { });
        ASSERT(result.isString() || result.isUndefined());
        if (!result.isUndefined())
            return result;
    }
    return jsEmptyString(&vm);
}

JSValue RegExpGlobalData::getLeftContext(ExecState* exec, JSGlobalObject* owner)
{
    return m_cachedResult.leftContext(exec, owner);
}

JSValue RegExpGlobalData::getRightContext(ExecState* exec, JSGlobalObject* owner)
{
    return m_cachedResult.rightContext(exec, owner);
}

} // namespace JSC
