/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#pragma once

#include <memory>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class Document;
class MediaQueryList;
class MediaQueryListListener;
class MediaQueryEvaluator;
class MediaQuerySet;
class RenderStyle;

// MediaQueryMatcher class is responsible for keeping a vector of pairs
// MediaQueryList x MediaQueryListListener. It is responsible for evaluating the queries
// whenever it is needed and to call the listeners if the corresponding query has changed.
// The listeners must be called in the order in which they were added.

class MediaQueryMatcher final : public RefCounted<MediaQueryMatcher> {
public:
    static Ref<MediaQueryMatcher> create(Document& document) { return adoptRef(*new MediaQueryMatcher(document)); }
    ~MediaQueryMatcher();

    void documentDestroyed();

    void addListener(Ref<MediaQueryListListener>&&, MediaQueryList&);
    void removeListener(MediaQueryListListener&, MediaQueryList&);

    RefPtr<MediaQueryList> matchMedia(const String&);

    unsigned evaluationRound() const { return m_evaluationRound; }

    void styleResolverChanged();

    bool evaluate(const MediaQuerySet&);

private:
    struct Listener {
        Ref<MediaQueryListListener> listener;
        Ref<MediaQueryList> query;
    };

    explicit MediaQueryMatcher(Document&);
    std::unique_ptr<RenderStyle> documentElementUserAgentStyle() const;
    String mediaType() const;

    WeakPtr<Document> m_document;
    Vector<Listener> m_listeners;

    // This value is incremented at style selector changes.
    // It is used to avoid evaluating queries more then once and to make sure
    // that a media query result change is notified exactly once.
    unsigned m_evaluationRound { 1 };
};

} // namespace WebCore
