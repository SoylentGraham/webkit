/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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

WI.Breakpoint = class Breakpoint extends WI.Object
{
    constructor(sourceCodeLocationOrInfo, disabled, condition)
    {
        super();

        if (sourceCodeLocationOrInfo instanceof WI.SourceCodeLocation) {
            var sourceCode = sourceCodeLocationOrInfo.sourceCode;
            var contentIdentifier = sourceCode ? sourceCode.contentIdentifier : null;
            var scriptIdentifier = sourceCode instanceof WI.Script ? sourceCode.id : null;
            var target = sourceCode instanceof WI.Script ? sourceCode.target : null;
            var location = sourceCodeLocationOrInfo;
        } else if (sourceCodeLocationOrInfo && typeof sourceCodeLocationOrInfo === "object") {
            // The 'url' fallback is for transitioning from older frontends and should be removed.
            var contentIdentifier = sourceCodeLocationOrInfo.contentIdentifier || sourceCodeLocationOrInfo.url;
            var lineNumber = sourceCodeLocationOrInfo.lineNumber || 0;
            var columnNumber = sourceCodeLocationOrInfo.columnNumber || 0;
            var location = new WI.SourceCodeLocation(null, lineNumber, columnNumber);
            var ignoreCount = sourceCodeLocationOrInfo.ignoreCount || 0;
            var autoContinue = sourceCodeLocationOrInfo.autoContinue || false;
            var actions = sourceCodeLocationOrInfo.actions || [];
            for (var i = 0; i < actions.length; ++i)
                actions[i] = new WI.BreakpointAction(this, actions[i]);
            disabled = sourceCodeLocationOrInfo.disabled;
            condition = sourceCodeLocationOrInfo.condition;
        } else
            console.error("Unexpected type passed to WI.Breakpoint", sourceCodeLocationOrInfo);

        this._id = null;
        this._contentIdentifier = contentIdentifier || null;
        this._scriptIdentifier = scriptIdentifier || null;
        this._target = target || null;
        this._disabled = disabled || false;
        this._condition = condition || "";
        this._ignoreCount = ignoreCount || 0;
        this._autoContinue = autoContinue || false;
        this._actions = actions || [];
        this._resolved = false;

        this._sourceCodeLocation = location;
        this._sourceCodeLocation.addEventListener(WI.SourceCodeLocation.Event.LocationChanged, this._sourceCodeLocationLocationChanged, this);
        this._sourceCodeLocation.addEventListener(WI.SourceCodeLocation.Event.DisplayLocationChanged, this._sourceCodeLocationDisplayLocationChanged, this);
    }

    // Public

    get identifier()
    {
        return this._id;
    }

    set identifier(id)
    {
        this._id = id || null;
    }

    get contentIdentifier()
    {
        return this._contentIdentifier;
    }

    get scriptIdentifier()
    {
        return this._scriptIdentifier;
    }

    get target()
    {
        return this._target;
    }

    get sourceCodeLocation()
    {
        return this._sourceCodeLocation;
    }

    get resolved()
    {
        return this._resolved;
    }

    set resolved(resolved)
    {
        if (this._resolved === resolved)
            return;

        function isSpecialBreakpoint()
        {
            return this._sourceCodeLocation.isEqual(new WI.SourceCodeLocation(null, Infinity, Infinity));
        }

        console.assert(!resolved || this._sourceCodeLocation.sourceCode || isSpecialBreakpoint.call(this), "Breakpoints must have a SourceCode to be resolved.", this);

        this._resolved = resolved || false;

        this.dispatchEventToListeners(WI.Breakpoint.Event.ResolvedStateDidChange);
    }

    get disabled()
    {
        return this._disabled;
    }

    set disabled(disabled)
    {
        if (this._disabled === disabled)
            return;

        this._disabled = disabled || false;

        this.dispatchEventToListeners(WI.Breakpoint.Event.DisabledStateDidChange);
    }

    get condition()
    {
        return this._condition;
    }

    set condition(condition)
    {
        if (this._condition === condition)
            return;

        this._condition = condition;

        this.dispatchEventToListeners(WI.Breakpoint.Event.ConditionDidChange);
    }

    get ignoreCount()
    {
        return this._ignoreCount;
    }

    set ignoreCount(ignoreCount)
    {
        console.assert(ignoreCount >= 0, "Ignore count cannot be negative.");
        if (ignoreCount < 0)
            return;

        if (this._ignoreCount === ignoreCount)
            return;

        this._ignoreCount = ignoreCount;

        this.dispatchEventToListeners(WI.Breakpoint.Event.IgnoreCountDidChange);
    }

    get autoContinue()
    {
        return this._autoContinue;
    }

    set autoContinue(cont)
    {
        if (this._autoContinue === cont)
            return;

        this._autoContinue = cont;

        this.dispatchEventToListeners(WI.Breakpoint.Event.AutoContinueDidChange);
    }

    get actions()
    {
        return this._actions;
    }

    get probeActions()
    {
        return this._actions.filter(function(action) {
            return action.type === WI.BreakpointAction.Type.Probe;
        });
    }

    cycleToNextMode()
    {
        if (this.disabled) {
            // When cycling, clear auto-continue when going from disabled to enabled.
            this.autoContinue = false;
            this.disabled = false;
            return;
        }

        if (this.autoContinue) {
            this.disabled = true;
            return;
        }

        if (this.actions.length) {
            this.autoContinue = true;
            return;
        }

        this.disabled = true;
    }

    createAction(type, precedingAction, data)
    {
        var newAction = new WI.BreakpointAction(this, type, data || null);

        if (!precedingAction)
            this._actions.push(newAction);
        else {
            var index = this._actions.indexOf(precedingAction);
            console.assert(index !== -1);
            if (index === -1)
                this._actions.push(newAction);
            else
                this._actions.splice(index + 1, 0, newAction);
        }

        this.dispatchEventToListeners(WI.Breakpoint.Event.ActionsDidChange);

        return newAction;
    }

    recreateAction(type, actionToReplace)
    {
        var newAction = new WI.BreakpointAction(this, type, null);

        var index = this._actions.indexOf(actionToReplace);
        console.assert(index !== -1);
        if (index === -1)
            return null;

        this._actions[index] = newAction;

        this.dispatchEventToListeners(WI.Breakpoint.Event.ActionsDidChange);

        return newAction;
    }

    removeAction(action)
    {
        var index = this._actions.indexOf(action);
        console.assert(index !== -1);
        if (index === -1)
            return;

        this._actions.splice(index, 1);

        if (!this._actions.length)
            this.autoContinue = false;

        this.dispatchEventToListeners(WI.Breakpoint.Event.ActionsDidChange);
    }

    clearActions(type)
    {
        if (!type)
            this._actions = [];
        else
            this._actions = this._actions.filter(function(action) { return action.type !== type; });

        this.dispatchEventToListeners(WI.Breakpoint.Event.ActionsDidChange);
    }

    saveIdentityToCookie(cookie)
    {
        cookie["breakpoint-content-identifier"] = this.contentIdentifier;
        cookie["breakpoint-line-number"] = this.sourceCodeLocation.lineNumber;
        cookie["breakpoint-column-number"] = this.sourceCodeLocation.columnNumber;
    }

    serializeOptions()
    {
        return {
            condition: this._condition,
            ignoreCount: this._ignoreCount,
            actions: this._actions.map((action) => action.toJSON()),
            autoContinue: this._autoContinue,
        };
    }

    toJSON(key)
    {
        // The id, scriptIdentifier, target, and resolved state are tied to the current session, so don't include them for serialization.
        let json = {
            contentIdentifier: this._contentIdentifier,
            lineNumber: this._sourceCodeLocation.lineNumber,
            columnNumber: this._sourceCodeLocation.columnNumber,
            disabled: this._disabled,
            ...this.serializeOptions(),
        };
        if (key === WI.ObjectStore.toJSONSymbol)
            json[WI.objectStores.breakpoints.keyPath] = this._contentIdentifier + ":" + this._sourceCodeLocation.lineNumber + ":" + this._sourceCodeLocation.columnNumber;
        return json;
    }

    // Protected (Called by BreakpointAction)

    breakpointActionDidChange(action)
    {
        var index = this._actions.indexOf(action);
        console.assert(index !== -1);
        if (index === -1)
            return;

        this.dispatchEventToListeners(WI.Breakpoint.Event.ActionsDidChange);
    }

    // Private

    _sourceCodeLocationLocationChanged(event)
    {
        this.dispatchEventToListeners(WI.Breakpoint.Event.LocationDidChange, event.data);
    }

    _sourceCodeLocationDisplayLocationChanged(event)
    {
        this.dispatchEventToListeners(WI.Breakpoint.Event.DisplayLocationDidChange, event.data);
    }
};

WI.Breakpoint.TypeIdentifier = "breakpoint";

WI.Breakpoint.Event = {
    DisabledStateDidChange: "breakpoint-disabled-state-did-change",
    ResolvedStateDidChange: "breakpoint-resolved-state-did-change",
    ConditionDidChange: "breakpoint-condition-did-change",
    IgnoreCountDidChange: "breakpoint-ignore-count-did-change",
    ActionsDidChange: "breakpoint-actions-did-change",
    AutoContinueDidChange: "breakpoint-auto-continue-did-change",
    LocationDidChange: "breakpoint-location-did-change",
    DisplayLocationDidChange: "breakpoint-display-location-did-change",
};
