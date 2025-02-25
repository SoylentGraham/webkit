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

#import "config.h"

#import "PlatformUtilities.h"
#import "TCPServer.h"
#import "Test.h"
#import "TestWKWebView.h"

#if PLATFORM(MAC)

static std::pair<RetainPtr<NSURLCredential>, RetainPtr<NSString>> credentialWithIdentityAndKeychainPath()
{
    // Certificate and private key were generated by running this command:
    // openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
    // and entering this information:
    /*
     Country Name (2 letter code) []:US
     State or Province Name (full name) []:New Mexico
     Locality Name (eg, city) []:Santa Fe
     Organization Name (eg, company) []:Self
     Organizational Unit Name (eg, section) []:Myself
     Common Name (eg, fully qualified host name) []:Me
     Email Address []:me@example.com
     */
    
    NSString *pemEncodedCertificate = @""
    "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"
    "3vjZD3y9kZk7mre9XHwkI8MdK5s=";
    
    NSString *pemEncodedPrivateKey = @""
    "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"
    "qJfU8bbdbu2pi47Y4FdJK0HLffl5Rw==";
    
    NSData *derEncodedCertificate = [[[NSData alloc] initWithBase64EncodedString:pemEncodedCertificate options:0] autorelease];
    RetainPtr<SecCertificateRef> certificate = adoptCF(SecCertificateCreateWithData(nullptr, (__bridge CFDataRef)derEncodedCertificate));
    
    NSData *derEncodedPrivateKey = [[[NSData alloc] initWithBase64EncodedString:pemEncodedPrivateKey options:0] autorelease];
    NSDictionary* options = @{
        (id)kSecAttrKeyType: (id)kSecAttrKeyTypeRSA,
        (id)kSecAttrKeyClass: (id)kSecAttrKeyClassPrivate,
        (id)kSecAttrKeySizeInBits: @4096,
    };
    const NSUInteger pemEncodedPrivateKeyHeaderLength = 26;
    CFErrorRef error = nullptr;
    RetainPtr<SecKeyRef> privateKey = adoptCF(SecKeyCreateWithData((__bridge CFDataRef)[derEncodedPrivateKey subdataWithRange:NSMakeRange(pemEncodedPrivateKeyHeaderLength, derEncodedPrivateKey.length - pemEncodedPrivateKeyHeaderLength)], (__bridge CFDictionaryRef)options, &error));
    EXPECT_NULL(error);
    EXPECT_NOT_NULL(privateKey.get());

    SecKeychainRef keychainRef = nullptr;
    const char* keychainPassword = "testpassword";
    NSString *keychainPath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"TestKeychain"];
    OSStatus status = SecKeychainCreate([keychainPath UTF8String], strlen(keychainPassword), keychainPassword, NO, nullptr, &keychainRef);
    EXPECT_TRUE(status == errSecSuccess);
    RetainPtr<SecKeychainRef> keychain = adoptCF(keychainRef);
    EXPECT_NOT_NULL(keychain);

    SecExternalItemType type = kSecItemTypePrivateKey;
    status = SecItemImport((__bridge CFDataRef)derEncodedPrivateKey, nullptr, nullptr, &type, 0, nullptr, keychain.get(), nullptr);
    EXPECT_TRUE(status == errSecSuccess);

    SecIdentityRef identityRef = nullptr;
    status = SecIdentityCreateWithCertificate(keychain.get(), certificate.get(), &identityRef);
    EXPECT_NOT_NULL(identityRef);
    EXPECT_TRUE(status == errSecSuccess);
    RetainPtr<SecIdentityRef> identity = adoptCF(identityRef);
    
    return {
        [NSURLCredential credentialWithIdentity:identity.get() certificates:@[(id)certificate.get()] persistence:NSURLCredentialPersistenceNone],
        keychainPath
    };
}

static bool navigationFinished;
static RetainPtr<NSString> keychainPath;

@interface ChallengeDelegate : NSObject <WKNavigationDelegate>
@end

@implementation ChallengeDelegate

- (void)webView:(WKWebView *)webView didFinishNavigation:(null_unspecified WKNavigation *)navigation
{
    navigationFinished = true;
}

- (void)webView:(WKWebView *)webView didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition, NSURLCredential *))completionHandler
{
    NSURLProtectionSpace *protectionSpace = challenge.protectionSpace;
    EXPECT_NULL(challenge.proposedCredential);
    EXPECT_EQ(challenge.previousFailureCount, 0);
    EXPECT_TRUE([protectionSpace.realm isEqualToString:@"testrealm"]);
    EXPECT_FALSE(protectionSpace.receivesCredentialSecurely);
    EXPECT_FALSE(protectionSpace.isProxy);
    EXPECT_TRUE([protectionSpace.host isEqualToString:@"127.0.0.1"]);
    EXPECT_NULL(protectionSpace.proxyType);
    EXPECT_TRUE([protectionSpace.protocol isEqualToString:NSURLProtectionSpaceHTTP]);
    EXPECT_TRUE([protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodHTTPBasic]);
    EXPECT_EQ([(NSHTTPURLResponse *)challenge.failureResponse statusCode], 401);

    auto pair = credentialWithIdentityAndKeychainPath();
    completionHandler(NSURLSessionAuthChallengeUseCredential, pair.first.get());
    keychainPath = WTFMove(pair.second);
}

@end

namespace TestWebKitAPI {

TEST(Challenge, SecIdentity)
{
    TCPServer server([] (auto socket) {
        char readBuffer[1000];
        auto bytesRead = ::read(socket, readBuffer, sizeof(readBuffer));
        EXPECT_GT(bytesRead, 0);
        EXPECT_TRUE(static_cast<size_t>(bytesRead) < sizeof(readBuffer));
        
        const char* challengeHeader =
        "HTTP/1.1 401 Unauthorized\r\n"
        "Date: Sat, 23 Mar 2019 06:29:01 GMT\r\n"
        "Content-Length: 0\r\n"
        "WWW-Authenticate: Basic realm=\"testrealm\"\r\n\r\n";
        auto bytesWritten = ::write(socket, challengeHeader, strlen(challengeHeader));
        EXPECT_EQ(static_cast<size_t>(bytesWritten), strlen(challengeHeader));
        
        bytesRead = ::read(socket, readBuffer, sizeof(readBuffer));
        EXPECT_GT(bytesRead, 0);
        EXPECT_TRUE(static_cast<size_t>(bytesRead) < sizeof(readBuffer));
        
        const char* responseHeader =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n\r\n"
        "Hello, World!";
        bytesWritten = ::write(socket, responseHeader, strlen(responseHeader));
        EXPECT_EQ(static_cast<size_t>(bytesWritten), strlen(responseHeader));
    });

    auto webView = adoptNS([WKWebView new]);
    auto delegate = adoptNS([ChallengeDelegate new]);
    [webView setNavigationDelegate:delegate.get()];
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithFormat:@"http://127.0.0.1:%d/", server.port()]]]];

    Util::run(&navigationFinished);
    
    EXPECT_NOT_NULL(keychainPath.get());
    NSError *error = nil;
    [[NSFileManager defaultManager] removeItemAtPath:keychainPath.get() error:&error];
    EXPECT_NULL(error);
}

} // namespace TestWebKitAPI

#endif
