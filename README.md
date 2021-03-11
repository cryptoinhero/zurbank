ZURBank (beta) integration/staging tree
=========================================

[![Build Status](https://travis-ci.org/zurcoin/zurbank.svg?branch=zurbank-0.0.10)](https://travis-ci.org/zurcoin/zurbank)

What is the Zus Layer
----------------------
The Zus Layer is a communications protocol that uses the Bitcoin block chain to enable features such as smart contracts, user currencies and decentralized peer-to-peer exchanges. A common analogy that is used to describe the relation of the Zus Layer to Bitcoin is that of HTTP to TCP/IP: HTTP, like the Zus Layer, is the application layer to the more fundamental transport and internet layer of TCP/IP, like Bitcoin.

http://www.omnilayer.org

What is ZURBank
-----------------

ZURBank is a fast, portable Zus Layer implementation that is based off the Bitcoin Core codebase (currently 0.13.2). This implementation requires no external dependencies extraneous to Bitcoin Core, and is native to the Bitcoin network just like other Bitcoin nodes. It currently supports a wallet mode and is seamlessly available on three platforms: Windows, Linux and Mac OS. Zus Layer extensions are exposed via the JSON-RPC interface. Development has been consolidated on the ZURBank product, and it is the reference client for the Zus Layer.

Disclaimer, warning
-------------------
This software is EXPERIMENTAL software. USE ON MAINNET AT YOUR OWN RISK.

By default this software will use your existing Bitcoin wallet, including spending zurcoins contained therein (for example for transaction fees or trading).
The protocol and transaction processing rules for the Zus Layer are still under active development and are subject to change in future.
ZURBank should be considered an alpha-level product, and you use it at your own risk. Neither the Zus Foundation nor the ZURBank developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this installation of ZURBank should be viewed as EXPERIMENTAL. Your wallet data, zurcoins and Zus Layer tokens may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

This software is provided open-source at no cost. You are responsible for knowing the law in your country and determining if your use of this software contravenes any local laws.

PLEASE DO NOT use wallet(s) with significant amounts of zurcoins or Zus Layer tokens while testing!

Testnet
-------

Testnet mode allows ZURBank to be run on the Bitcoin testnet blockchain for safe testing.

1. To run ZURBank in testnet mode, run ZURBank with the following option in place: `-testnet`.

2. To receive ZUS (and TZUS) on testnet please send TZUR to `pxELuroPRgD7Di8hQikT4fqdK7xoYKdrZy`. For each 1 TZUR you will receive 100 ZUS and 100 TZUS.

Dependencies
------------
Boost >= 1.53

Installation
------------

You will need appropriate libraries to run ZURBank on Unix,
please see [doc/build-unix.md](doc/build-unix.md) for the full listing.

You will need to install git & pkg-config:

```
sudo apt-get install git
sudo apt-get install pkg-config
```

Clone the ZURBank repository:

```
git clone https://github.com/zurcoin/zurbank.git
cd zurbank/
```

Then, run:

```
./autogen.sh
./configure
make
```
Once complete:

```
cd src/
```
And start ZURBank using `./zurbankd` (or `./qt/zurbank-qt` if built with UI). The inital parse step for a first time run
will take up to 60 minutes or more, during this time your client will scan the blockchain for Zus Layer transactions. You can view the
output of the parsing at any time by viewing the log located in your datadir, by default: `~/.zurcoin/zurbank.log`.

ZURBank requires the transaction index to be enabled. Add an entry to your zurcoin.conf file for `txindex=1` to enable it or ZURBank will refuse to start.

If a message is returned asking you to reindex, pass the `-reindex` flag as startup option. The reindexing process can take serveral hours.

To issue RPC commands to ZURBank you may add the `-server=1` CLI flag or add an entry to the zurcoin.conf file (located in `~/.zurcoin/` by default).

In zurcoin.conf:
```
server=1
```

After this step completes, check that the installation went smoothly by issuing the following command `./zurbank-cli zus_getinfo` which should return the `zurbankversion` as well as some
additional information related to the client.

The documentation for the RPC interface and command-line is located in [src/zurbank/doc/rpc-api.md] (src/zurbank/doc/rpc-api.md).

Current feature set:
--------------------

* Broadcasting of simple send (tx 0) [doc] (src/zurbank/doc/rpc-api.md#zus_send), and send to owners (tx 3) [doc] (src/zurbank/doc/rpc-api.md#zus_sendsto)

* Obtaining a Zus Layer balance [doc] (src/zurbank/doc/rpc-api.md#zus_getbalance)

* Obtaining all balances (including smart property) for an address [doc] (src/zurbank/doc/rpc-api.md#zus_getallbalancesforaddress)

* Obtaining all balances associated with a specific smart property [doc] (src/zurbank/doc/rpc-api.md#zus_getallbalancesforid)

* Retrieving information about any Zus Layer transaction [doc] (src/zurbank/doc/rpc-api.md#zus_gettransaction)

* Listing historical transactions of addresses in the wallet [doc] (src/zurbank/doc/rpc-api.md#zus_listtransactions)

* Retreiving detailed information about a smart property [doc] (src/zurbank/doc/rpc-api.md#zus_getproperty)

* Retreiving active and expired crowdsale information [doc] (src/zurbank/doc/rpc-api.md#zus_getcrowdsale)

* Sending a specific ZUR amount to a receiver with referenceamount in `zus_send`

* Creating and broadcasting transactions based on raw Zus Layer transactions with `zus_sendrawtx`

* Functional UI for balances, sending and historical transactions

* Creating any supported transaction type via RPC interface

* Meta-DEx integration

* Support for class B (multisig) and class C (op-return) encoded transactions

* Support of unconfirmed transactions

* Creation of raw transactions with non-wallet inputs

Related projects:
-----------------

* https://github.com/zurcoin/OmniJ

* https://github.com/zurcoin/omniwallet

* https://github.com/zurcoin/spec

Support:
--------

* Please open a [GitHub issue] (https://github.com/zurcoin/zurbank/issues) to file a bug submission.
