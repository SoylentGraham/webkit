/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "B3Common.h"

#if ENABLE(B3_JIT)

#include "DFGCommon.h"
#include "FTLState.h"
#include "Options.h"
#include <wtf/Optional.h>

namespace JSC { namespace B3 {

bool shouldDumpIR(B3ComplitationMode mode)
{
#if ENABLE(FTL_JIT)
    return FTL::verboseCompilationEnabled() || FTL::shouldDumpDisassembly() || shouldDumpIRAtEachPhase(mode);
#else
    return shouldDumpIRAtEachPhase(mode);
#endif
}

bool shouldDumpIRAtEachPhase(B3ComplitationMode mode)
{
    if (mode == B3Mode)
        return Options::dumpGraphAtEachPhase() || Options::dumpB3GraphAtEachPhase();
    return Options::dumpGraphAtEachPhase() || Options::dumpAirGraphAtEachPhase();
}

bool shouldValidateIR()
{
    return DFG::validationEnabled() || shouldValidateIRAtEachPhase();
}

bool shouldValidateIRAtEachPhase()
{
    return Options::validateGraphAtEachPhase();
}

bool shouldSaveIRBeforePhase()
{
    return Options::verboseValidationFailure();
}

Optional<GPRReg> pinnedExtendedOffsetAddrRegister()
{
#if CPU(ARM64)
    return static_cast<GPRReg>(+MacroAssembler::dataTempRegister);
#elif CPU(X86_64)
    return WTF::nullopt;
#else
#error Unhandled architecture.
#endif
}

} } // namespace JSC::B3

#endif // ENABLE(B3_JIT)

