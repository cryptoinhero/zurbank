#!/bin/bash

SRCDIR=./src/
NUL=/dev/null
PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.zurcoin/regtest
$SRCDIR/zurbankd --regtest --server --daemon --omniactivationallowsender=any >$NUL
sleep 10
printf "   * Preparing some mature testnet ZUR\n"
$SRCDIR/zurbank-cli --regtest setgenerate true 102 >$NUL
printf "   * Obtaining a master address to work with\n"
ADDR=$($SRCDIR/zurbank-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet ZUR for fees\n"
$SRCDIR/zurbank-cli --regtest sendtoaddress $ADDR 20 >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Participating in the Exodus crowdsale to obtain some ZUS\n"
JSON="{\"pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy\":10,\""$ADDR"\":4}"
$SRCDIR/zurbank-cli --regtest sendmany OMNIAccount $JSON >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "   * Creating test properties\n"
$SRCDIR/zurbank-cli --regtest zus_sendissuancefixed $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 10000000 >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
$SRCDIR/zurbank-cli --regtest zus_sendissuancefixed $ADDR 1 2 0 "Z_TestCat" "Z_TestSubCat" "Z_DivisTestProperty" "Z_TestURL" "Z_TestData" 10000 >$NUL
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
printf "\nActivating the fee system & all pair trading, and testing they went live:\n\n"
printf "   * Sending the all pair activation & checking it was valid... "
BLOCKS=$($SRCDIR/zurbank-cli --regtest getblockcount)
TXID=$($SRCDIR/zurbank-cli --regtest zus_sendactivation $ADDR 8 $(($BLOCKS + 8)) 999)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending the fee system activation & checking it was valid... "
BLOCKS=$($SRCDIR/zurbank-cli --regtest getblockcount)
TXIDA=$($SRCDIR/zurbank-cli --regtest zus_sendactivation $ADDR 9 $(($BLOCKS + 8)) 999)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDA | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Mining 10 blocks to forward past the activation block\n"
$SRCDIR/zurbank-cli --regtest setgenerate true 10 >$NUL
printf "   * Sending a trade of #3 for #4 & checking it was valid... "
TXIDB=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 3 2000 4 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDB | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending a matching trade of #4 for #3 & checking it was valid... "
TXIDC=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 4 1.0 3 2000)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDC | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Checking the fee cache now has 1 fee cached for property 3..."
CACHEDFEE=$($SRCDIR/zurbank-cli --regtest zus_getfeecache 3 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "\nDeactivating the fee system & testing it's now disabled...\n\n"
printf "   * Sending the deactivation & checking it was valid... "
BLOCKS=$($SRCDIR/zurbank-cli --regtest getblockcount)
TXIDD=$($SRCDIR/zurbank-cli --regtest zus_senddeactivation $ADDR 9)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDD | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending a new trade of #3 for #4 & checking it was valid... "
TXIDE=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 3 2000 4 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDE | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending another matching trade of #4 for #3 & checking it was valid... "
TXIDF=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 4 1.0 3 2000)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDF | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Checking the fee cache did not increase and still has 1 fee cached for property 3..."
CACHEDFEEA=$($SRCDIR/zurbank-cli --regtest zus_getfeecache 3 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEEA == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEEA
    FAIL=$((FAIL+1))
fi
printf "\nDeactivating all pair trading & testing it's now disabled...\n\n"
printf "   * Sending a new trade of #3 for #4 & checking it was valid... "
TXIDG=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 3 2000 4 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDG | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending the deactivation & checking it was valid... "
BLOCKS=$($SRCDIR/zurbank-cli --regtest getblockcount)
TXIDH=$($SRCDIR/zurbank-cli --regtest zus_senddeactivation $ADDR 8)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDH | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Making sure the orderbook for #3/#4 is now empty... "
ORDERS=$($SRCDIR/zurbank-cli --regtest zus_getorderbook 3 4 | grep -vF "[" | grep -vF "]" | wc -l)
if [ $ORDERS == "0" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $ORDERS
    FAIL=$((FAIL+1))
fi
printf "   * Checking the first trade was refunded... "
BALANCE=$($SRCDIR/zurbank-cli --regtest zus_getbalance $ADDR 3 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "   * Sending a trade of #3 for #4 & checking it was invalid... "
TXIDI=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 3 2000 4 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDI | grep valid | cut -c15-)
if [ $RESULT == "false," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nDeactivating the MetaDEx completely & testing it's now disabled...\n\n"
printf "   * Sending a trade of #3 for #1 & checking it was valid... "
TXIDJ=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 3 2000 1 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDJ | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending a trade of #4 for #1 & checking it was valid... "
TXIDK=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 4 2000 1 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDK | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending the deactivation & checking it was valid... "
BLOCKS=$($SRCDIR/zurbank-cli --regtest getblockcount)
TXIDL=$($SRCDIR/zurbank-cli --regtest zus_senddeactivation $ADDR 2)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDL | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Making sure the orderbook for #3/#1 is now empty... "
ORDERSB=$($SRCDIR/zurbank-cli --regtest zus_getorderbook 3 1 | grep -vF "[" | grep -vF "]" | wc -l)
if [ $ORDERSB == "0" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "   * Making sure the orderbook for #4/#1 is now empty... "
ORDERSC=$($SRCDIR/zurbank-cli --regtest zus_getorderbook 4 1 | grep -vF "[" | grep -vF "]" | wc -l)
if [ $ORDERSC == "0" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "   * Checking the trade for #3/#1 was refunded... "
BALANCEA=$($SRCDIR/zurbank-cli --regtest zus_getbalance $ADDR 3 | grep balance | cut -d '"' -f4)
if [ $BALANCEA == "9999999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCEA
    FAIL=$((FAIL+1))
fi
printf "   * Checking the trade for #4/#1 was refunded... "
BALANCEB=$($SRCDIR/zurbank-cli --regtest zus_getbalance $ADDR 4 | grep balance | cut -d '"' -f4)
if [ $BALANCEB == "10000.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCEB
    FAIL=$((FAIL+1))
fi
printf "   * Sending a trade of #3 for #1 & checking it was invalid... "
TXIDM=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 3 2000 1 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDM | grep valid | cut -c15-)
if [ $RESULT == "false," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending a trade of #1 for #4 & checking it was invalid... "
TXIDG=$($SRCDIR/zurbank-cli --regtest zus_sendtrade $ADDR 1 1.0 4 1.0)
$SRCDIR/zurbank-cli --regtest setgenerate true 1 >$NUL
RESULT=$($SRCDIR/zurbank-cli --regtest zus_gettransaction $TXIDG | grep valid | cut -c15-)
if [ $RESULT == "false," ]
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
printf "#    Passed = %d   #\n" $PASS
printf "#    Failed = %d    #\n" $FAIL
printf "####################\n"
printf "\n"

$SRCDIR/zurbank-cli --regtest stop

