#include "zurbank/zurbank.h"
#include "zurbank/rules.h"

#include "base58.h"
#include "chainparams.h"
#include "test/test_zurcoin.h"

#include <boost/test/unit_test.hpp>

#include <limits>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(zurbank_exodus_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(exodus_address_mainnet)
{
    BOOST_CHECK(CBitcoinAddress("UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6") ==
                ExodusAddress());
    BOOST_CHECK(!(CBitcoinAddress("1rDQWR9yZLJY7ciyghAaF7XKD9tGzQuP6") ==
                ExodusAddress()));
}

BOOST_AUTO_TEST_CASE(exodus_crowdsale_address_mainnet)
{
    BOOST_CHECK(CBitcoinAddress("UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6") ==
                ExodusCrowdsaleAddress(0));
    BOOST_CHECK(CBitcoinAddress("UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>::max()));
    BOOST_CHECK(!(CBitcoinAddress("1rDQWR9yZLJY7ciyghAaF7XKD9tGzQuP6") ==
                ExodusCrowdsaleAddress(0)));
    BOOST_CHECK(!(CBitcoinAddress("1rDQWR9yZLJY7ciyghAaF7XKD9tGzQuP6") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>::max())));
}

BOOST_AUTO_TEST_CASE(exodus_address_testnet)
{
    SelectParams(CBaseChainParams::TESTNET);

    BOOST_CHECK(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusAddress());
    BOOST_CHECK(!(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusAddress()));

    SelectParams(CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_CASE(exodus_address_regtest)
{
    SelectParams(CBaseChainParams::REGTEST);

    BOOST_CHECK(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusAddress());
    BOOST_CHECK(!(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusAddress()));

    SelectParams(CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_CASE(exodus_crowdsale_address_testnet)
{
    SelectParams(CBaseChainParams::TESTNET);

    BOOST_CHECK(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(0));
    BOOST_CHECK(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK-1));
    BOOST_CHECK(!(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(0)));
    BOOST_CHECK(!(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK-1)));
    BOOST_CHECK(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK));
    BOOST_CHECK(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>::max()));
    BOOST_CHECK(!(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK)));
    BOOST_CHECK(!(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>::max())));

    SelectParams(CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_CASE(exodus_crowdsale_address_regtest)
{
    SelectParams(CBaseChainParams::REGTEST);

    BOOST_CHECK(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(0));
    BOOST_CHECK(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK-1));
    BOOST_CHECK(!(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(0)));
    BOOST_CHECK(!(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK-1)));
    BOOST_CHECK(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK));
    BOOST_CHECK(CBitcoinAddress("pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>::max()));
    BOOST_CHECK(!(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK)));
    BOOST_CHECK(!(CBitcoinAddress("qALrKLGjQnqAf8S3u6B9oFVs4dwmGg83Z5") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>::max())));

    SelectParams(CBaseChainParams::MAIN);
}


BOOST_AUTO_TEST_SUITE_END()
