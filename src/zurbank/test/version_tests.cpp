#include "zurbank/version.h"

#include "config/zurcoin-config.h"
#include "test/test_zurcoin.h"

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_FIXTURE_TEST_SUITE(zurbank_version_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(version_comparison)
{
    BOOST_CHECK(ZURBANK_VERSION > 1000000); // ZURBank v0.0.10.0
}

/**
 * The following tests are very unhandy, because any version bump
 * breaks the tests.
 *
 * TODO: might be removed completely.
 */

BOOST_AUTO_TEST_CASE(version_string)
{
    BOOST_CHECK_EQUAL(ZurBankVersion(), "0.5.0");
}

BOOST_AUTO_TEST_CASE(version_number)
{
    BOOST_CHECK_EQUAL(ZURBANK_VERSION, 50000000);
}

BOOST_AUTO_TEST_CASE(config_package_version)
{
    // the package version is used in the file names:
    BOOST_CHECK_EQUAL(PACKAGE_VERSION, "0.5.0");
}


BOOST_AUTO_TEST_SUITE_END()
