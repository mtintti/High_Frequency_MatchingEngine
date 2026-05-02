
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <wincrypt.h>
#include "windcertsload.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#pragma comment(lib, "crypt32.lib")

void windowsCertificateStore(boost::asio::ssl::context& ctx)
{
    // Open the Windows certificate store
    HCERTSTORE hStore = CertOpenSystemStoreA(NULL, "ROOT");
    if (!hStore) {
        std::cerr << "\n failed to open Windows cert store";
        return;
    }

    // Get the OpenSSL X509 store from the context
    X509_STORE* opensslStore = SSL_CTX_get_cert_store(ctx.native_handle());

    PCCERT_CONTEXT pCert = nullptr;
    int count = 0;
    while ((pCert = CertEnumCertificatesInStore(hStore, pCert)) != nullptr)
    {
        // Convert Windows cert format (DER) to OpenSSL X509
        const unsigned char* certData = pCert->pbCertEncoded;
        X509* x509 = d2i_X509(nullptr, &certData, pCert->cbCertEncoded);
        if (x509) {
            X509_STORE_add_cert(opensslStore, x509);
            X509_free(x509);
            count++;
        }
    }

    CertCloseStore(hStore, 0);
    std::cout << "\n loaded " << count << " certs from Windows store";
}