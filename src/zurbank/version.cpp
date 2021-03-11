#include "zurbank/version.h"

#include "clientversion.h"
#include "tinyformat.h"

#include <string>

#ifdef HAVE_BUILD_INFO
#    include "build.h"
#endif

#ifdef ZURBANK_VERSION_STATUS
#    define ZURBANK_VERSION_SUFFIX STRINGIZE(ZURBANK_VERSION_STATUS)
#else
#    define ZURBANK_VERSION_SUFFIX ""
#endif

//! Returns formatted ZURBank version, e.g. "1.2.0" or "1.3.4.1"
const std::string ZurBankVersion()
{
    if (ZURBANK_VERSION_BUILD) {
        return strprintf("%d.%d.%d.%d",
                ZURBANK_VERSION_MAJOR,
                ZURBANK_VERSION_MINOR,
                ZURBANK_VERSION_PATCH,
                ZURBANK_VERSION_BUILD);
    } else {
        return strprintf("%d.%d.%d",
                ZURBANK_VERSION_MAJOR,
                ZURBANK_VERSION_MINOR,
                ZURBANK_VERSION_PATCH);
    }
}

//! Returns formatted Bitcoin Core version, e.g. "0.10", "0.9.3"
const std::string BitcoinCoreVersion()
{
    if (CLIENT_VERSION_BUILD) {
        return strprintf("%d.%d.%d.%d",
                CLIENT_VERSION_MAJOR,
                CLIENT_VERSION_MINOR,
                CLIENT_VERSION_REVISION,
                CLIENT_VERSION_BUILD);
    } else {
        return strprintf("%d.%d.%d",
                CLIENT_VERSION_MAJOR,
                CLIENT_VERSION_MINOR,
                CLIENT_VERSION_REVISION);
    }
}
