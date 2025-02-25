/*
 * Copyright (C) 2015-2019 Apple Inc. All rights reserved.
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

#if ENABLE(APPLE_PAY)

#include "MessageReceiver.h"
#include "MessageSender.h"
#include "PaymentAuthorizationPresenter.h"
#include <WebCore/PaymentHeaders.h>
#include <wtf/Forward.h>
#include <wtf/RetainPtr.h>
#include <wtf/WeakPtr.h>
#include <wtf/WorkQueue.h>

#if USE(APPLE_INTERNAL_SDK)
#include <WebKitAdditions/WebPaymentCoordinatorProxyAdditions.h>
#endif

namespace IPC {
class Connection;
}

namespace PAL {
class SessionID;
}

namespace WebCore {
enum class PaymentAuthorizationStatus;
class Payment;
class PaymentContact;
class PaymentMerchantSession;
class PaymentMethod;
}

OBJC_CLASS NSObject;
OBJC_CLASS NSWindow;
OBJC_CLASS PKPaymentAuthorizationViewController;
OBJC_CLASS PKPaymentRequest;
OBJC_CLASS UIViewController;

namespace WebKit {

class WebPageProxy;

class WebPaymentCoordinatorProxy
    : private IPC::MessageReceiver
    , private IPC::MessageSender
    , public CanMakeWeakPtr<WebPaymentCoordinatorProxy>
    , public PaymentAuthorizationPresenter::Client {
public:
    struct Client {
        virtual ~Client() = default;

        virtual IPC::Connection* paymentCoordinatorConnection(const WebPaymentCoordinatorProxy&) = 0;
        virtual const String& paymentCoordinatorSourceApplicationBundleIdentifier(const WebPaymentCoordinatorProxy&, PAL::SessionID) = 0;
        virtual const String& paymentCoordinatorSourceApplicationSecondaryIdentifier(const WebPaymentCoordinatorProxy&, PAL::SessionID) = 0;
        virtual void paymentCoordinatorAddMessageReceiver(WebPaymentCoordinatorProxy&, const IPC::StringReference&, IPC::MessageReceiver&) = 0;
        virtual void paymentCoordinatorRemoveMessageReceiver(WebPaymentCoordinatorProxy&, const IPC::StringReference&) = 0;
#if PLATFORM(IOS_FAMILY)
        virtual UIViewController *paymentCoordinatorPresentingViewController(const WebPaymentCoordinatorProxy&) = 0;
        virtual const String& paymentCoordinatorCTDataConnectionServiceType(const WebPaymentCoordinatorProxy&, PAL::SessionID) = 0;
        virtual std::unique_ptr<PaymentAuthorizationPresenter> paymentCoordinatorAuthorizationPresenter(WebPaymentCoordinatorProxy&, PKPaymentRequest *) = 0;
#endif
#if PLATFORM(MAC)
        virtual NSWindow *paymentCoordinatorPresentingWindow(const WebPaymentCoordinatorProxy&) = 0;
#endif
    };

    friend class NetworkConnectionToWebProcess;
    explicit WebPaymentCoordinatorProxy(Client&);
    ~WebPaymentCoordinatorProxy();

private:
    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;

    // IPC::MessageSender
    IPC::Connection* messageSenderConnection() const final;
    uint64_t messageSenderDestinationID() const final;
    
    // PaymentAuthorizationPresenter::Client
    void presenterDidAuthorizePayment(PaymentAuthorizationPresenter&, const WebCore::Payment&) final;
    void presenterDidFinish(PaymentAuthorizationPresenter&, bool didReachFinalState) final;
    void presenterDidSelectPaymentMethod(PaymentAuthorizationPresenter&, const WebCore::PaymentMethod&) final;
    void presenterDidSelectShippingContact(PaymentAuthorizationPresenter&, const WebCore::PaymentContact&) final;
    void presenterDidSelectShippingMethod(PaymentAuthorizationPresenter&, const WebCore::ApplePaySessionPaymentRequest::ShippingMethod&) final;
    void presenterWillValidateMerchant(PaymentAuthorizationPresenter&, const URL&) final;

    // Message handlers
    void canMakePayments(CompletionHandler<void(bool)>&&);
    void canMakePaymentsWithActiveCard(const String& merchantIdentifier, const String& domainName, PAL::SessionID, CompletionHandler<void(bool)>&&);
    void openPaymentSetup(const String& merchantIdentifier, const String& domainName, CompletionHandler<void(bool)>&&);
    void showPaymentUI(uint64_t destinationID, PAL::SessionID, const String& originatingURLString, const Vector<String>& linkIconURLStrings, const WebCore::ApplePaySessionPaymentRequest&, CompletionHandler<void(bool)>&&);
    void completeMerchantValidation(const WebCore::PaymentMerchantSession&);
    void completeShippingMethodSelection(const Optional<WebCore::ShippingMethodUpdate>&);
    void completeShippingContactSelection(const Optional<WebCore::ShippingContactUpdate>&);
    void completePaymentMethodSelection(const Optional<WebCore::PaymentMethodUpdate>&);
    void completePaymentSession(const Optional<WebCore::PaymentAuthorizationResult>&);
    void abortPaymentSession();
    void cancelPaymentSession();

    bool canBegin() const;
    bool canCancel() const;
    bool canCompletePayment() const;
    bool canAbort() const;

    void didCancelPaymentSession();
    void didReachFinalState();
    void hidePaymentUI();

    void platformCanMakePayments(CompletionHandler<void(bool)>&&);
    void platformCanMakePaymentsWithActiveCard(const String& merchantIdentifier, const String& domainName, PAL::SessionID, WTF::Function<void(bool)>&& completionHandler);
    void platformOpenPaymentSetup(const String& merchantIdentifier, const String& domainName, WTF::Function<void(bool)>&& completionHandler);
    void platformShowPaymentUI(const URL& originatingURL, const Vector<URL>& linkIconURLs, PAL::SessionID, const WebCore::ApplePaySessionPaymentRequest&, CompletionHandler<void(bool)>&&);
    void platformCompleteMerchantValidation(const WebCore::PaymentMerchantSession&);
    void platformCompleteShippingMethodSelection(const Optional<WebCore::ShippingMethodUpdate>&);
    void platformCompleteShippingContactSelection(const Optional<WebCore::ShippingContactUpdate>&);
    void platformCompletePaymentMethodSelection(const Optional<WebCore::PaymentMethodUpdate>&);
    void platformCompletePaymentSession(const Optional<WebCore::PaymentAuthorizationResult>&);
#if PLATFORM(COCOA)
    RetainPtr<PKPaymentRequest> platformPaymentRequest(const URL& originatingURL, const Vector<URL>& linkIconURLs, PAL::SessionID, const WebCore::ApplePaySessionPaymentRequest&);
#endif

    Client& m_client;
    Optional<uint64_t> m_destinationID;

    enum class State {
        // Idle - Nothing's happening.
        Idle,

        // Activating - Waiting to show the payment UI.
        Activating,

        // Active - Showing payment UI.
        Active,

        // Authorized - Dispatching the authorized event and waiting for the paymentSessionCompleted message.
        Authorized,

        // ShippingMethodSelected - Dispatching the shippingmethodselected event and waiting for a reply.
        ShippingMethodSelected,

        // ShippingContactSelected - Dispatching the shippingcontactselected event and waiting for a reply.
        ShippingContactSelected,

        // PaymentMethodSelected - Dispatching the paymentmethodselected event and waiting for a reply.
        PaymentMethodSelected,
    } m_state { State::Idle };

    enum class MerchantValidationState {
        // Idle - Nothing's happening.
        Idle,

        // Validating - Dispatching the validatemerchant event and waiting for a reply.
        Validating,

        // ValidationComplete - A merchant session has been sent along to PassKit.
        ValidationComplete
    } m_merchantValidationState { MerchantValidationState::Idle };

    std::unique_ptr<PaymentAuthorizationPresenter> m_authorizationPresenter;
    Ref<WorkQueue> m_canMakePaymentsQueue;

#if PLATFORM(MAC)
    uint64_t m_showPaymentUIRequestSeed { 0 };
    RetainPtr<NSWindow> m_sheetWindow;
    RetainPtr<NSObject> m_sheetWindowWillCloseObserver;
#endif
    
#if defined(WEBPAYMENTCOORDINATORPROXY_ADDITIONS)
WEBPAYMENTCOORDINATORPROXY_ADDITIONS
#undef WEBPAYMENTCOORDINATORPROXY_ADDITIONS
#else
    void finishConstruction(WebPaymentCoordinatorProxy&) { }
#endif
};

}

#endif
