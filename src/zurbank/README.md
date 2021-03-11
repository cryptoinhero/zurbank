ZURBank (beta) integration/staging tree
=========================================

[![Build Status](https://travis-ci.org/zurcoin/zurbank.svg?branch=zurbank-0.0.10)](https://travis-ci.org/zurcoin/zurbank)

What is the Zus Layer
----------------------
The Zus Layer is a communications protocol that uses the Bitcoin block chain to enable features such as smart contracts, user currencies and decentralized peer-to-peer exchanges. A common analogy that is used to describe the relation of the Zus Layer to Bitcoin is that of HTTP to TCP/IP: HTTP, like the Zus Layer, is the application layer to the more fundamental transport and internet layer of TCP/IP, like Bitcoin.

http://www.omnilayer.org

What is ZURBank
-----------------

ZURBank is a fast, portable Zus Layer implementation that is based off the Bitcoin Core codebase (currently 0.13.2). This implementation requires no external dependencies extraneous to Bitcoin Core, and is native to the Zurcoin network just like other Bitcoin nodes. It currently supports a wallet mode and is seamlessly available on three platforms: Windows, Linux and Mac OS. Zus Layer extensions are exposed via the UI and the JSON-RPC interface. Development has been consolidated on the ZURBank product, and it is the reference client for the Zus Layer.

Disclaimer, warning
-------------------
This software is EXPERIMENTAL software. USE ON MAINNET AT YOUR OWN RISK.

By default this software will use your existing Bitcoin wallet, including spending zurcoins contained therein (for example for transaction fees or trading).
The protocol and transaction processing rules for the Zus Layer are still under active development and are subject to change in future.
ZURBank should be considered an alpha-level product, and you use it at your own risk. Neither the Zus Foundation nor the ZURBank developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this installation of ZURBank should be viewed as EXPERIMENTAL. Your wallet data, zurcoins and Zus Layer tokens may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

This software is provided open-source at no cost. You are responsible for knowing the law in your country and determining if your use of this software contravenes any local laws.

PLEASE DO NOT use wallet(s) with significant amounts of zurcoins or Zus Layer tokens while testing!

Related projects
----------------

* https://github.com/zurcoin/OmniJ

* https://github.com/zurcoin/omniwallet

* https://github.com/zurcoin/spec

Support
-------

* Please open a [GitHub issue](https://github.com/zurcoin/zurbank/issues) to file a bug submission.
