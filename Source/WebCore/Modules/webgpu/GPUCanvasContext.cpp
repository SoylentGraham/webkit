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

#include "config.h"
#include "GPUCanvasContext.h"

#if ENABLE(WEBGPU)

namespace WebCore {

std::unique_ptr<GPUCanvasContext> GPUCanvasContext::create(CanvasBase& canvas)
{
    auto context = std::unique_ptr<GPUCanvasContext>(new GPUCanvasContext(canvas));
    context->suspendIfNeeded();
    return context;
}

GPUCanvasContext::GPUCanvasContext(CanvasBase& canvas)
    : GPUBasedCanvasRenderingContext(canvas)
{
}

void GPUCanvasContext::replaceSwapChain(Ref<WebGPUSwapChain>&& newSwapChain)
{
    ASSERT(newSwapChain->swapChain());

    if (m_swapChain)
        m_swapChain->destroy();

    m_swapChain = WTFMove(newSwapChain);
}

CALayer* GPUCanvasContext::platformLayer() const
{
    if (m_swapChain && m_swapChain->swapChain())
        return m_swapChain->swapChain()->platformLayer();
    return nullptr;
}

void GPUCanvasContext::reshape(int width, int height)
{
    if (m_swapChain && m_swapChain->swapChain())
        m_swapChain->swapChain()->reshape(width, height);
}

void GPUCanvasContext::markLayerComposited()
{
    if (m_swapChain && m_swapChain->swapChain())
        m_swapChain->swapChain()->present();
}

} // namespace WebCore

#endif // ENABLE(WEBGPU)
