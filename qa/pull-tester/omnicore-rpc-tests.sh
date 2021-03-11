#!/bin/bash

# Get BUILDDIR and REAL_BITCOIND
CURDIR=$(cd $(dirname "$0"); pwd)
. "${CURDIR}/tests_config.py"

OMNICORED="$BUILDDIR/src/zurbankd$EXEEXT"
OMNICORECLI="$BUILDDIR/src/zurbank-cli$EXEEXT"
TESTDIR="$BUILDDIR/qa/tmp/zurbank-rpc-tests"
DATADIR="$TESTDIR/.zurcoin"

# Start clean
rm -rf "$TESTDIR"

git clone https://github.com/zurcoin/OmniJ.git $TESTDIR
mkdir -p "$DATADIR/regtest"
touch "$DATADIR/regtest/zurbank.log"
cd $TESTDIR
echo "Omni Core RPC test dir: "$TESTDIR
echo "Last OmniJ commit: "$(git log -n 1 --format="%H Author: %cn <%ce>")
if [ "$@" = "true" ]; then
    echo "Debug logging level: maximum"
    $OMNICORED -datadir="$DATADIR" -regtest -txindex -server -daemon -rpcuser=zurcoinrpc -rpcpassword=pass -debug=1 -omnidebug=all -omnialertallowsender=any -omniactivationallowsender=any -paytxfee=0.0001 -minrelaytxfee=0.00001 -limitancestorcount=750 -limitdescendantcount=750 -rpcserialversion=0 -discover=0 -listen=0 &
else
    echo "Debug logging level: minimum"
    $OMNICORED -datadir="$DATADIR" -regtest -txindex -server -daemon -rpcuser=zurcoinrpc -rpcpassword=pass -debug=0 -omnidebug=none -omnialertallowsender=any -omniactivationallowsender=any -paytxfee=0.0001 -minrelaytxfee=0.00001 -limitancestorcount=750 -limitdescendantcount=750 -rpcserialversion=0 -discover=0 -listen=0 &
fi
$OMNICORECLI -datadir="$DATADIR" -regtest -rpcuser=zurcoinrpc -rpcpassword=pass -rpcwait getinfo
$OMNICORECLI -datadir="$DATADIR" -regtest -rpcuser=zurcoinrpc -rpcpassword=pass -rpcwait omni_getinfo
./gradlew --console plain :omnij-rpc:regTest
STATUS=$?
$OMNICORECLI -datadir="$DATADIR" -regtest -rpcuser=zurcoinrpc -rpcpassword=pass -rpcwait stop

# If $STATUS is not 0, the test failed.
if [ $STATUS -ne 0 ]; then tail -100 $DATADIR/regtest/zurbank.log; fi


exit $STATUS
