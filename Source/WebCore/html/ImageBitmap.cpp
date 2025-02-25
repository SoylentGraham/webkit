/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
#include "ImageBitmap.h"

#include "BitmapImage.h"
#include "Blob.h"
#include "CachedImage.h"
#include "ExceptionOr.h"
#include "FileReaderLoader.h"
#include "FileReaderLoaderClient.h"
#include "GraphicsContext.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLVideoElement.h"
#include "ImageBitmapOptions.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "IntRect.h"
#include "JSImageBitmap.h"
#include "LayoutSize.h"
#include "RenderElement.h"
#include "SharedBuffer.h"
#include "TypedOMCSSImageValue.h"
#include <wtf/Optional.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Variant.h>

namespace WebCore {

#if USE(IOSURFACE_CANVAS_BACKING_STORE) || ENABLE(ACCELERATED_2D_CANVAS)
static RenderingMode bufferRenderingMode = Accelerated;
#else
static RenderingMode bufferRenderingMode = Unaccelerated;
#endif

Ref<ImageBitmap> ImageBitmap::create(IntSize size)
{
    return create(ImageBuffer::create(FloatSize(size.width(), size.height()), bufferRenderingMode));
}

Ref<ImageBitmap> ImageBitmap::create(std::pair<std::unique_ptr<ImageBuffer>, bool>&& buffer)
{
    auto imageBitmap = create(WTFMove(buffer.first));
    imageBitmap->m_originClean = buffer.second;
    return imageBitmap;
}

Ref<ImageBitmap> ImageBitmap::create(std::unique_ptr<ImageBuffer>&& buffer)
{
    return adoptRef(*new ImageBitmap(WTFMove(buffer)));
}

void ImageBitmap::createPromise(ScriptExecutionContext& scriptExecutionContext, ImageBitmap::Source&& source, ImageBitmapOptions&& options, ImageBitmap::Promise&& promise)
{
    WTF::switchOn(source,
        [&] (auto& specificSource) {
            createPromise(scriptExecutionContext, specificSource, WTFMove(options), WTF::nullopt, WTFMove(promise));
        }
    );
}

Vector<std::pair<std::unique_ptr<ImageBuffer>, bool>> ImageBitmap::detachBitmaps(Vector<RefPtr<ImageBitmap>>&& bitmaps)
{
    Vector<std::pair<std::unique_ptr<ImageBuffer>, bool>> buffers;
    for (auto& bitmap : bitmaps)
        buffers.append(std::make_pair(bitmap->transferOwnershipAndClose(), bitmap->originClean()));
    return buffers;
}


void ImageBitmap::createPromise(ScriptExecutionContext& scriptExecutionContext, ImageBitmap::Source&& source, ImageBitmapOptions&& options, int sx, int sy, int sw, int sh, ImageBitmap::Promise&& promise)
{
    // 1. If either the sw or sh arguments are specified but zero, return a promise
    //    rejected with an "RangeError" DOMException and abort these steps.
    if (!sw || !sh) {
        promise.reject(RangeError, "Cannot create ImageBitmap with a width or height of 0");
        return;
    }

    auto left = sw >= 0 ? sx : sx + sw;
    auto top = sh >= 0 ? sy : sy + sh;
    auto width = std::abs(sw);
    auto height = std::abs(sh);

    WTF::switchOn(source,
        [&] (auto& specificSource) {
            createPromise(scriptExecutionContext, specificSource, WTFMove(options), IntRect { left, top, width, height }, WTFMove(promise));
        }
    );
}

static bool taintsOrigin(CachedImage& cachedImage)
{
    auto* image = cachedImage.image();
    if (!image)
        return false;

    if (image->sourceURL().protocolIsData())
        return false;

    if (!image->hasSingleSecurityOrigin())
        return true;

    if (!cachedImage.isCORSSameOrigin())
        return true;

    return false;
}

#if ENABLE(VIDEO)
static bool taintsOrigin(SecurityOrigin* origin, HTMLVideoElement& video)
{
    if (!video.hasSingleSecurityOrigin())
        return true;

    if (video.player()->didPassCORSAccessCheck())
        return false;

    auto url = video.currentSrc();
    if (url.protocolIsData())
        return false;

    return !origin->canRequest(url);
}
#endif

// https://html.spec.whatwg.org/multipage/imagebitmap-and-animations.html#cropped-to-the-source-rectangle-with-formatting
static ExceptionOr<IntRect> croppedSourceRectangleWithFormatting(IntSize inputSize, ImageBitmapOptions& options, Optional<IntRect> rect)
{
    // 2. If either or both of resizeWidth and resizeHeight members of options are less
    //    than or equal to 0, then return a promise rejected with "InvalidStateError"
    //    DOMException and abort these steps.
    if ((options.resizeWidth && options.resizeWidth.value() <= 0) || (options.resizeHeight && options.resizeHeight.value() <= 0))
        return Exception { InvalidStateError, "Invalid resize dimensions" };

    // 3. If sx, sy, sw and sh are specified, let sourceRectangle be a rectangle whose
    //    corners are the four points (sx, sy), (sx+sw, sy),(sx+sw, sy+sh), (sx,sy+sh).
    //    Otherwise let sourceRectangle be a rectangle whose corners are the four points
    //    (0,0), (width of input, 0), (width of input, height of input), (0, height of
    //    input).
    auto sourceRectangle = rect.valueOr(IntRect { 0, 0, inputSize.width(), inputSize.height() });

    // 4. Clip sourceRectangle to the dimensions of input.
    sourceRectangle.intersect(IntRect { 0, 0, inputSize.width(), inputSize.height() });

    return { WTFMove(sourceRectangle) };
}

static IntSize outputSizeForSourceRectangle(IntRect sourceRectangle, ImageBitmapOptions& options)
{
    // 5. Let outputWidth be determined as follows:
    auto outputWidth = [&] () -> int {
        if (options.resizeWidth)
            return options.resizeWidth.value();
        if (options.resizeHeight)
            return ceil(sourceRectangle.width() * static_cast<double>(options.resizeHeight.value()) / sourceRectangle.height());
        return sourceRectangle.width();
    }();

    // 6. Let outputHeight be determined as follows:
    auto outputHeight = [&] () -> int {
        if (options.resizeHeight)
            return options.resizeHeight.value();
        if (options.resizeWidth)
            return ceil(sourceRectangle.height() * static_cast<double>(options.resizeWidth.value()) / sourceRectangle.width());
        return sourceRectangle.height();
    }();

    return { outputWidth, outputHeight };
}

static InterpolationQuality interpolationQualityForResizeQuality(ImageBitmapOptions::ResizeQuality resizeQuality)
{
    switch (resizeQuality) {
    case ImageBitmapOptions::ResizeQuality::Pixelated:
        return InterpolationNone;
    case ImageBitmapOptions::ResizeQuality::Low:
        return InterpolationDefault; // Low is the default.
    case ImageBitmapOptions::ResizeQuality::Medium:
        return InterpolationMedium;
    case ImageBitmapOptions::ResizeQuality::High:
        return InterpolationHigh;
    }
    ASSERT_NOT_REACHED();
    return InterpolationDefault;
}

// FIXME: More steps from https://html.spec.whatwg.org/multipage/imagebitmap-and-animations.html#cropped-to-the-source-rectangle-with-formatting

// 7. Place input on an infinite transparent black grid plane, positioned so that its
//    top left corner is at the origin of the plane, with the x-coordinate increasing
//    to the right, and the y-coordinate increasing down, and with each pixel in the
//    input image data occupying a cell on the plane's grid.

// 8. Let output be the rectangle on the plane denoted by sourceRectangle.

// 9. Scale output to the size specified by outputWidth and outputHeight. The user
//    agent should use the value of the resizeQuality option to guide the choice of
//    scaling algorithm.

// 10. If the value of the imageOrientation member of options is "flipY", output must
//     be flipped vertically, disregarding any image orientation metadata of the source
//     (such as EXIF metadata), if any.

// 11. If image is an img element or a Blob object, let val be the value of the
//     colorSpaceConversion member of options, and then run these substeps:
//
//     1. If val is "default", the color space conversion behavior is implementation-specific,
//        and should be chosen according to the color space that the implementation uses for
//        drawing images onto the canvas.
//
//     2. If val is "none", output must be decoded without performing any color space
//        conversions. This means that the image decoding algorithm must ignore color profile
//        metadata embedded in the source data as well as the display device color profile.

// 12. Let val be the value of premultiplyAlpha member of options, and then run these substeps:
//
//     1. If val is "default", the alpha premultiplication behavior is implementation-specific,
//        and should be chosen according to implementation deems optimal for drawing images
//        onto the canvas.
//
//     2. If val is "premultiply", the output that is not premultiplied by alpha must have its
//        color components multiplied by alpha and that is premultiplied by alpha must be left
//        untouched.
//
//     3. If val is "none", the output that is not premultiplied by alpha must be left untouched
//        and that is premultiplied by alpha must have its color components divided by alpha.

// 13. Return output.

void ImageBitmap::createPromise(ScriptExecutionContext&, RefPtr<HTMLImageElement>& imageElement, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
{
    // 2. If image is not completely available, then return a promise rejected with
    // an "InvalidStateError" DOMException and abort these steps.

    auto* cachedImage = imageElement->cachedImage();
    if (!cachedImage || !imageElement->complete()) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap that is not completely available");
        return;
    }

    // 3. If image's media data has no intrinsic dimensions (e.g. it's a vector graphic
    //    with no specified content size), and both or either of the resizeWidth and
    //    resizeHeight options are not specified, then return a promise rejected with
    //    an "InvalidStateError" DOMException and abort these steps.

    auto imageSize = cachedImage->imageSizeForRenderer(imageElement->renderer(), 1.0f);
    if ((!imageSize.width() || !imageSize.height()) && (!options.resizeWidth || !options.resizeHeight)) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap from a source with no intrinsic size without providing resize dimensions");
        return;
    }

    // 4. If image's media data has no intrinsic dimensions (e.g. it's a vector graphics
    //    with no specified content size), it should be rendered to a bitmap of the size
    //    specified by the resizeWidth and the resizeHeight options.

    if (!imageSize.width() && !imageSize.height()) {
        imageSize.setWidth(options.resizeWidth.value());
        imageSize.setHeight(options.resizeHeight.value());
    }

    // 5. If the sw and sh arguments are not specified and image's media data has both or
    //    either of its intrinsic width and intrinsic height values equal to 0, then return
    //    a promise rejected with an "InvalidStateError" DOMException and abort these steps.
    // 6. If the sh argument is not specified and image's media data has an intrinsic height
    //    of 0, then return a promise rejected with an "InvalidStateError" DOMException and
    //    abort these steps.

    // FIXME: It's unclear how these steps can happen, since step 4 required setting a
    // width and height for the image.

    if (!rect && (!imageSize.width() || !imageSize.height())) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap from a source with no intrinsic size without providing dimensions");
        return;
    }

    // 8. Let the ImageBitmap object's bitmap data be a copy of image's media data, cropped to
    //    the source rectangle with formatting. If this is an animated image, the ImageBitmap
    //    object's bitmap data must only be taken from the default image of the animation (the
    //    one that the format defines is to be used when animation is not supported or is disabled),
    //    or, if there is no such image, the first frame of the animation.

    auto sourceRectangle = croppedSourceRectangleWithFormatting(roundedIntSize(imageSize), options, WTFMove(rect));
    if (sourceRectangle.hasException()) {
        promise.reject(sourceRectangle.releaseException());
        return;
    }

    auto outputSize = outputSizeForSourceRectangle(sourceRectangle.returnValue(), options);
    auto bitmapData = ImageBuffer::create(FloatSize(outputSize.width(), outputSize.height()), bufferRenderingMode);

    auto imageForRender = cachedImage->imageForRenderer(imageElement->renderer());
    if (!imageForRender) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap from image that can't be rendered");
        return;
    }

    FloatRect destRect(FloatPoint(), outputSize);
    ImagePaintingOptions paintingOptions;
    paintingOptions.m_interpolationQuality = interpolationQualityForResizeQuality(options.resizeQuality);

    bitmapData->context().drawImage(*imageForRender, destRect, sourceRectangle.releaseReturnValue(), paintingOptions);

    // 7. Create a new ImageBitmap object.
    auto imageBitmap = create(WTFMove(bitmapData));

    // 9. If the origin of image's image is not the same origin as the origin specified by the
    //    entry settings object, then set the origin-clean flag of the ImageBitmap object's
    //    bitmap to false.

    imageBitmap->m_originClean = !taintsOrigin(*cachedImage);

    // 10. Return a new promise, but continue running these steps in parallel.
    // 11. Resolve the promise with the new ImageBitmap object as the value.

    promise.resolve(WTFMove(imageBitmap));
}

void ImageBitmap::createPromise(ScriptExecutionContext&, RefPtr<HTMLCanvasElement>& canvasElement, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
{
    // 2. If the canvas element's bitmap has either a horizontal dimension or a vertical
    //    dimension equal to zero, then return a promise rejected with an "InvalidStateError"
    //    DOMException and abort these steps.
    auto size = canvasElement->size();
    if (!size.width() || !size.height()) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap from a canvas that has zero width or height");
        return;
    }

    // 4. Let the ImageBitmap object's bitmap data be a copy of the canvas element's bitmap
    //    data, cropped to the source rectangle with formatting.

    auto sourceRectangle = croppedSourceRectangleWithFormatting(size, options, WTFMove(rect));
    if (sourceRectangle.hasException()) {
        promise.reject(sourceRectangle.releaseException());
        return;
    }

    auto outputSize = outputSizeForSourceRectangle(sourceRectangle.returnValue(), options);
    auto bitmapData = ImageBuffer::create(FloatSize(outputSize.width(), outputSize.height()), bufferRenderingMode);

    auto imageForRender = canvasElement->copiedImage();
    if (!imageForRender) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap from canvas that can't be rendered");
        return;
    }

    FloatRect destRect(FloatPoint(), outputSize);
    ImagePaintingOptions paintingOptions;
    paintingOptions.m_interpolationQuality = interpolationQualityForResizeQuality(options.resizeQuality);

    bitmapData->context().drawImage(*imageForRender, destRect, sourceRectangle.releaseReturnValue(), paintingOptions);

    // 3. Create a new ImageBitmap object.
    auto imageBitmap = create(WTFMove(bitmapData));

    // 5. Set the origin-clean flag of the ImageBitmap object's bitmap to the same value as
    //    the origin-clean flag of the canvas element's bitmap.

    imageBitmap->m_originClean = canvasElement->originClean();

    // 6. Return a new promise, but continue running these steps in parallel.
    // 7. Resolve the promise with the new ImageBitmap object as the value.

    promise.resolve(WTFMove(imageBitmap));
}

#if ENABLE(VIDEO)
void ImageBitmap::createPromise(ScriptExecutionContext& scriptExecutionContext, RefPtr<HTMLVideoElement>& video, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
{
    // https://html.spec.whatwg.org/multipage/#dom-createimagebitmap
    // WHATWG HTML 2102913b313078cd8eeac7e81e6a8756cbd3e773
    // Steps 3-7.
    // (Step 3 is handled in croppedSourceRectangleWithFormatting.)

    // 4. Check the usability of the image argument. If this throws an exception
    //    or returns bad, then return p rejected with an "InvalidStateError"
    //    DOMException.
    if (video->readyState() == HTMLMediaElement::HAVE_NOTHING || video->readyState() == HTMLMediaElement::HAVE_METADATA) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap before the HTMLVideoElement has data");
        return;
    }

    // 6.1. If image's networkState attribute is NETWORK_EMPTY, then return p
    //      rejected with an "InvalidStateError" DOMException.
    if (video->networkState() == HTMLMediaElement::NETWORK_EMPTY) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap before the HTMLVideoElement has data");
        return;
    }

    // 6.2. Set imageBitmap's bitmap data to a copy of the frame at the current
    //      playback position, at the media resource's intrinsic width and
    //      intrinsic height (i.e., after any aspect-ratio correction has been
    //      applied), cropped to the source rectangle with formatting.
    auto size = video->player() ? roundedIntSize(video->player()->naturalSize()) : IntSize();
    auto maybeSourceRectangle = croppedSourceRectangleWithFormatting(size, options, WTFMove(rect));
    if (maybeSourceRectangle.hasException()) {
        promise.reject(maybeSourceRectangle.releaseException());
        return;
    }
    auto sourceRectangle = maybeSourceRectangle.releaseReturnValue();

    auto outputSize = outputSizeForSourceRectangle(sourceRectangle, options);
    auto bitmapData = ImageBuffer::create(FloatSize(outputSize.width(), outputSize.height()), bufferRenderingMode);

    {
        GraphicsContext& c = bitmapData->context();
        GraphicsContextStateSaver stateSaver(c);
        c.clip(FloatRect(FloatPoint(), outputSize));
        auto scaleX = float(outputSize.width()) / float(sourceRectangle.width());
        auto scaleY = float(outputSize.height()) / float(sourceRectangle.height());
        c.scale(FloatSize(scaleX, scaleY));
        c.translate(-sourceRectangle.location());
        video->paintCurrentFrameInContext(c, FloatRect(FloatPoint(), size));
    }

    // 5. Let imageBitmap be a new ImageBitmap object.
    auto imageBitmap = create(WTFMove(bitmapData));

    // 6.3. If the origin of image's video is not same origin with entry
    //      settings object's origin, then set the origin-clean flag of
    //      image's bitmap to false.
    imageBitmap->m_originClean = !taintsOrigin(scriptExecutionContext.securityOrigin(), *video);

    // 6.4.1. Resolve p with imageBitmap.
    promise.resolve(WTFMove(imageBitmap));
}
#endif

#if ENABLE(CSS_TYPED_OM)
void ImageBitmap::createPromise(ScriptExecutionContext&, RefPtr<TypedOMCSSImageValue>&, ImageBitmapOptions&&, Optional<IntRect>, ImageBitmap::Promise&& promise)
{
    promise.reject(InvalidStateError, "Not implemented");
}
#endif

void ImageBitmap::createPromise(ScriptExecutionContext&, RefPtr<ImageBitmap>& existingImageBitmap, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
{
    // 2. If image's [[Detached]] internal slot value is true, return a promise
    //    rejected with an "InvalidStateError" DOMException and abort these steps.
    if (existingImageBitmap->isDetached() || !existingImageBitmap->buffer()) {
        promise.reject(InvalidStateError, "Cannot create ImageBitmap from a detached ImageBitmap");
        return;
    }

    // 4. Let the ImageBitmap object's bitmap data be a copy of the image argument's
    //    bitmap data, cropped to the source rectangle with formatting.
    auto sourceRectangle = croppedSourceRectangleWithFormatting(existingImageBitmap->buffer()->logicalSize(), options, WTFMove(rect));
    if (sourceRectangle.hasException()) {
        promise.reject(sourceRectangle.releaseException());
        return;
    }

    auto outputSize = outputSizeForSourceRectangle(sourceRectangle.returnValue(), options);
    auto bitmapData = ImageBuffer::create(FloatSize(outputSize.width(), outputSize.height()), bufferRenderingMode);

    auto imageForRender = existingImageBitmap->buffer()->copyImage();

    FloatRect destRect(FloatPoint(), outputSize);
    ImagePaintingOptions paintingOptions;
    paintingOptions.m_interpolationQuality = interpolationQualityForResizeQuality(options.resizeQuality);

    bitmapData->context().drawImage(*imageForRender, destRect, sourceRectangle.releaseReturnValue(), paintingOptions);

    // 3. Create a new ImageBitmap object.
    auto imageBitmap = create(WTFMove(bitmapData));

    // 5. Set the origin-clean flag of the ImageBitmap object's bitmap to the same
    //    value as the origin-clean flag of the bitmap of the image argument.
    imageBitmap->m_originClean = existingImageBitmap->originClean();

    // 6. Return a new promise, but continue running these steps in parallel.
    // 7. Resolve the promise with the new ImageBitmap object as the value.
    promise.resolve(WTFMove(imageBitmap));
}

class ImageBitmapImageObserver final : public RefCounted<ImageBitmapImageObserver>, public ImageObserver {
public:
    static Ref<ImageBitmapImageObserver> create(String mimeType, long long expectedContentLength, const URL& sourceUrl)
    {
        return adoptRef(*new ImageBitmapImageObserver(mimeType, expectedContentLength, sourceUrl));
    }

    URL sourceUrl() const override { return m_sourceUrl; }
    String mimeType() const override { return m_mimeType; }
    long long expectedContentLength() const override { return m_expectedContentLength; }

    void decodedSizeChanged(const Image&, long long) override { }

    void didDraw(const Image&) override { }

    bool canDestroyDecodedData(const Image&) override { return true; }
    void imageFrameAvailable(const Image&, ImageAnimatingState, const IntRect* = nullptr, DecodingStatus = DecodingStatus::Invalid) override { }
    void changedInRect(const Image&, const IntRect* = nullptr) override { }

private:
    ImageBitmapImageObserver(String mimeType, long long expectedContentLength, const URL& sourceUrl)
        : m_mimeType(mimeType)
        , m_expectedContentLength(expectedContentLength)
        , m_sourceUrl(sourceUrl)
    { }

    String m_mimeType;
    long long m_expectedContentLength;
    URL m_sourceUrl;
};

class PendingImageBitmap final : public ActiveDOMObject, public FileReaderLoaderClient {
public:
    static void fetch(ScriptExecutionContext& scriptExecutionContext, RefPtr<Blob>&& blob, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
    {
        auto pendingImageBitmap = new PendingImageBitmap(scriptExecutionContext, WTFMove(blob), WTFMove(options), WTFMove(rect), WTFMove(promise));
        pendingImageBitmap->start(scriptExecutionContext);
    }

private:
    PendingImageBitmap(ScriptExecutionContext& scriptExecutionContext, RefPtr<Blob>&& blob, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
        : ActiveDOMObject(&scriptExecutionContext)
        , m_blobLoader(FileReaderLoader::ReadAsArrayBuffer, this)
        , m_blob(WTFMove(blob))
        , m_options(WTFMove(options))
        , m_rect(WTFMove(rect))
        , m_promise(WTFMove(promise))
    {
        suspendIfNeeded();
    }

    void start(ScriptExecutionContext& scriptExecutionContext)
    {
        m_blobLoader.start(&scriptExecutionContext, *m_blob);
    }

    // ActiveDOMObject

    const char* activeDOMObjectName() const override
    {
        return "PendingImageBitmap";
    }

    bool canSuspendForDocumentSuspension() const override
    {
        // FIXME: Deal with suspension.
        return false;
    }

    // FileReaderLoaderClient

    void didStartLoading() override
    {
    }

    void didReceiveData() override
    {
    }

    void didFinishLoading() override
    {
        createImageBitmap(m_blobLoader.arrayBufferResult());
        delete this;
    }

    void didFail(int) override
    {
        createImageBitmap(nullptr);
        delete this;
    }

    void createImageBitmap(RefPtr<ArrayBuffer>&& arrayBuffer)
    {
        if (!arrayBuffer) {
            m_promise.reject(InvalidStateError, "An error occured reading the Blob argument to createImageBitmap");
            return;
        }

        ImageBitmap::createFromBuffer(arrayBuffer.releaseNonNull(), m_blob->type(), m_blob->size(), m_blobLoader.url(), WTFMove(m_options), WTFMove(m_rect), WTFMove(m_promise));
    }

    FileReaderLoader m_blobLoader;
    RefPtr<Blob> m_blob;
    ImageBitmapOptions m_options;
    Optional<IntRect> m_rect;
    ImageBitmap::Promise m_promise;
};

void ImageBitmap::createFromBuffer(
    Ref<ArrayBuffer>&& arrayBuffer,
    String mimeType,
    long long expectedContentLength,
    const URL& sourceUrl,
    ImageBitmapOptions&& options,
    Optional<IntRect> rect,
    ImageBitmap::Promise&& promise)
{
    if (!arrayBuffer->byteLength()) {
        promise.reject(InvalidStateError, "Cannot create an ImageBitmap from an empty buffer");
        return;
    }

    auto sharedBuffer = SharedBuffer::create(static_cast<const char*>(arrayBuffer->data()), arrayBuffer->byteLength());
    auto observer = ImageBitmapImageObserver::create(mimeType, expectedContentLength, sourceUrl);
    auto image = Image::create(observer.get());
    if (!image) {
        promise.reject(InvalidStateError, "The type of the argument to createImageBitmap is not supported");
        return;
    }

    auto result = image->setData(sharedBuffer.copyRef(), true);
    if (result != EncodedDataStatus::Complete) {
        promise.reject(InvalidStateError, "Cannot decode the data in the argument to createImageBitmap");
        return;
    }

    auto sourceRectangle = croppedSourceRectangleWithFormatting(roundedIntSize(image->size()), options, rect);
    if (sourceRectangle.hasException()) {
        promise.reject(sourceRectangle.releaseException());
        return;
    }

    auto outputSize = outputSizeForSourceRectangle(sourceRectangle.returnValue(), options);
    auto bitmapData = ImageBuffer::create(FloatSize(outputSize.width(), outputSize.height()), bufferRenderingMode);
    if (!bitmapData) {
        promise.reject(InvalidStateError, "Cannot create an image buffer from the argument to createImageBitmap");
        return;
    }

    FloatRect destRect(FloatPoint(), outputSize);
    ImagePaintingOptions paintingOptions;
    paintingOptions.m_interpolationQuality = interpolationQualityForResizeQuality(options.resizeQuality);

    bitmapData->context().drawImage(*image, destRect, sourceRectangle.releaseReturnValue(), paintingOptions);

    auto imageBitmap = create(WTFMove(bitmapData));

    promise.resolve(WTFMove(imageBitmap));
}

void ImageBitmap::createPromise(ScriptExecutionContext& scriptExecutionContext, RefPtr<Blob>& blob, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
{
    // 2. Return a new promise, but continue running these steps in parallel.
    PendingImageBitmap::fetch(scriptExecutionContext, WTFMove(blob), WTFMove(options), WTFMove(rect), WTFMove(promise));
}

void ImageBitmap::createPromise(ScriptExecutionContext&, RefPtr<ImageData>& imageData, ImageBitmapOptions&& options, Optional<IntRect> rect, ImageBitmap::Promise&& promise)
{
    UNUSED_PARAM(imageData);
    UNUSED_PARAM(options);
    UNUSED_PARAM(rect);

    // 2. If the image object's data attribute value's [[Detached]] internal slot value
    //    is true, return a promise rejected with an "InvalidStateError" DOMException
    //    and abort these steps.

    // 3. Create a new ImageBitmap object.

    // 4. Let the ImageBitmap object's bitmap data be the image data given by the ImageData
    //    object, cropped to the source rectangle with formatting.

    // 5. Return a new promise, but continue running these steps in parallel.
    // 6. Resolve the promise with the new ImageBitmap object as the value.
    promise.reject(TypeError, "createImageBitmap with ImageData is not implemented");
}

ImageBitmap::ImageBitmap(std::unique_ptr<ImageBuffer>&& buffer)
    : m_bitmapData(WTFMove(buffer))
{
    ASSERT(m_bitmapData);
}

ImageBitmap::~ImageBitmap() = default;

unsigned ImageBitmap::width() const
{
    if (m_detached || !m_bitmapData)
        return 0;

    // FIXME: Is this the right width?
    return m_bitmapData->logicalSize().width();
}

unsigned ImageBitmap::height() const
{
    if (m_detached || !m_bitmapData)
        return 0;

    // FIXME: Is this the right height?
    return m_bitmapData->logicalSize().height();
}

void ImageBitmap::close()
{
    m_detached = true;
    m_bitmapData = nullptr;
}

std::unique_ptr<ImageBuffer> ImageBitmap::transferOwnershipAndClose()
{
    m_detached = true;
    return WTFMove(m_bitmapData);
}

}
