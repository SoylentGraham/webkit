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

#include "config.h"
#include "DownloadMonitor.h"

#include "Download.h"
#include "Logging.h"

#undef RELEASE_LOG_IF_ALLOWED
#define RELEASE_LOG_IF_ALLOWED(fmt, ...) RELEASE_LOG_IF(m_download.isAlwaysOnLoggingAllowed(), Network, "%p - DownloadMonitor::" fmt, this, ##__VA_ARGS__)

namespace WebKit {

constexpr uint64_t operator"" _kbps(unsigned long long kilobytesPerSecond)
{
    return kilobytesPerSecond * 1024;
}

struct ThroughputInterval {
    Seconds time;
    uint64_t bytesPerSecond;
};

static const ThroughputInterval throughputIntervals[] = {
    { 1_min, 1_kbps },
    { 5_min, 2_kbps },
    { 10_min, 4_kbps },
    { 15_min, 8_kbps },
    { 20_min, 16_kbps },
    { 25_min, 32_kbps },
    { 30_min, 64_kbps },
    { 45_min, 96_kbps },
    { 60_min, 128_kbps }
};

static Seconds timeUntilNextInterval(size_t currentInterval)
{
    RELEASE_ASSERT(currentInterval + 1 < WTF_ARRAY_LENGTH(throughputIntervals));
    return throughputIntervals[currentInterval + 1].time - throughputIntervals[currentInterval].time;
}

DownloadMonitor::DownloadMonitor(Download& download)
    : m_download(download)
{
}

double DownloadMonitor::measuredThroughputRate() const
{
    uint64_t bytes { 0 };
    for (const auto& timestamp : m_timestamps)
        bytes += timestamp.bytesReceived;
    if (!bytes)
        return 0;
    ASSERT(!m_timestamps.isEmpty());
    Seconds timeDifference = m_timestamps.last().time.secondsSinceEpoch() - m_timestamps.first().time.secondsSinceEpoch();
    double seconds = timeDifference.seconds();
    if (!seconds)
        return std::numeric_limits<double>::max();
    return bytes / seconds;
}

void DownloadMonitor::downloadReceivedBytes(uint64_t bytesReceived)
{
    if (m_timestamps.size() > timestampCapacity - 1) {
        ASSERT(m_timestamps.size() == timestampCapacity);
        m_timestamps.removeFirst();
    }
    m_timestamps.append({ MonotonicTime::now(), bytesReceived });
}

void DownloadMonitor::applicationWillEnterForeground()
{
    RELEASE_LOG_IF_ALLOWED("applicationWillEnterForeground (id = %" PRIu64 ")", m_download.downloadID().downloadID());
    m_timer.stop();
    m_interval = 0;
}

void DownloadMonitor::applicationDidEnterBackground()
{
    RELEASE_LOG_IF_ALLOWED("applicationDidEnterBackground (id = %" PRIu64 ")", m_download.downloadID().downloadID());
    ASSERT(!m_timer.isActive());
    ASSERT(!m_interval);
    m_timer.startOneShot(throughputIntervals[0].time / speedMultiplier());
}

uint32_t DownloadMonitor::speedMultiplier() const
{
    return m_download.manager().client().downloadMonitorSpeedMultiplier();
}

void DownloadMonitor::timerFired()
{
    RELEASE_ASSERT(m_interval < WTF_ARRAY_LENGTH(throughputIntervals));
    if (measuredThroughputRate() < throughputIntervals[m_interval].bytesPerSecond) {
        RELEASE_LOG_IF_ALLOWED("timerFired: cancelling download (id = %" PRIu64 ")", m_download.downloadID().downloadID());
        m_download.cancel();
    } else if (m_interval + 1 < WTF_ARRAY_LENGTH(throughputIntervals)) {
        RELEASE_LOG_IF_ALLOWED("timerFired: sufficient throughput rate (id = %" PRIu64 ")", m_download.downloadID().downloadID());
        m_timer.startOneShot(timeUntilNextInterval(m_interval++) / speedMultiplier());
    } else
        RELEASE_LOG_IF_ALLOWED("timerFired: Download reached threshold to not be terminated (id = %" PRIu64 ")", m_download.downloadID().downloadID());
}

} // namespace WebKit
