#!/bin/bash

SRCDIR=./src/
NUL=/dev/null
PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.zurcoin/regtest
$SRCDIR/zurbankd --regtest --server --daemon >$NUL
sleep 3
printf "   * Preparing some mature testnet ZUR\n"
$SRCDIR/zurbank-cli --regtest setgenerate true 102 >$NUL
printf "   * Obtaining a master address to work with\n"
ADDR=$($SRCDIR/zurbank-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet ZUR for fees\n"
$SRCDIR/zurbank-cli --regtest sendtoaddress $ADDR 20 >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Participating in the Exodus crowdsale to obtain some OMNI\n"
JSON="{\"pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy\":10,\""$ADDR"\":4}"
$SRCDIR/zurbank-cli --regtest sendmany OMNIAccount $JSON >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Creating an indivisible test property\n"
$SRCDIR/zurbank-cli --regtest omni_sendissuancefixed $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 10000000 >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Creating a divisible test property\n"
$SRCDIR/zurbank-cli --regtest omni_sendissuancefixed $ADDR 1 2 0 "Z_TestCat" "Z_TestSubCat" "Z_DivisTestProperty" "Z_TestURL" "Z_TestData" 10000 >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Creating another indivisible test property\n"
$SRCDIR/zurbank-cli --regtest omni_sendissuancefixed $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 10000000 >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "\nTesting a trade against self that uses divisible / divisible (10.0 OMNI for 100.0 #4)\n"
printf "   * Executing the trade\n"
TXID=$($SRCDIR/zurbank-cli --regtest omni_sendtrade $ADDR 1 10.0 4 100.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$($SRCDIR/zurbank-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that uses divisible / indivisible (10.0 OMNI for 100 #3)\n"
printf "   * Executing the trade\n"
TXID=$($SRCDIR/zurbank-cli --regtest omni_sendtrade $ADDR 1 10.0 3 100)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$($SRCDIR/zurbank-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that uses indivisible / divisible (10 #3 for 100.0 OMNI)\n"
printf "   * Executing the trade\n"
TXID=$($SRCDIR/zurbank-cli --regtest omni_sendtrade $ADDR 3 10 1 100.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$($SRCDIR/zurbank-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that uses indivisible / indivisible (10 #5 for 100 #3)\n"
printf "   * Executing the trade\n"
TXID=$($SRCDIR/zurbank-cli --regtest omni_sendtrade $ADDR 5 10 3 100)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$($SRCDIR/zurbank-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi

printf "\n"
printf "####################\n"
printf "#  Summary:        #\n"
printf "#    Passed = %d    #\n" $PASS
printf "#    Failed = %d    #\n" $FAIL
printf "####################\n"
printf "\n"

$SRCDIR/zurbank-cli --regtest stop

