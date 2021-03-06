#ifndef ZURBANK_RPCPAYLOAD_H
#define ZURBANK_RPCPAYLOAD_H

#include <univalue.h>

UniValue zus_createpayload_simplesend(const UniValue& params, bool fHelp);
UniValue zus_createpayload_sendall(const UniValue& params, bool fHelp);
UniValue zus_createpayload_dexsell(const UniValue& params, bool fHelp);
UniValue zus_createpayload_dexaccept(const UniValue& params, bool fHelp);
UniValue zus_createpayload_sto(const UniValue& params, bool fHelp);
UniValue zus_createpayload_issuancefixed(const UniValue& params, bool fHelp);
UniValue zus_createpayload_issuancecrowdsale(const UniValue& params, bool fHelp);
UniValue zus_createpayload_issuancemanaged(const UniValue& params, bool fHelp);
UniValue zus_createpayload_closecrowdsale(const UniValue& params, bool fHelp);
UniValue zus_createpayload_grant(const UniValue& params, bool fHelp);
UniValue zus_createpayload_revoke(const UniValue& params, bool fHelp);
UniValue zus_createpayload_changeissuer(const UniValue& params, bool fHelp);
UniValue zus_createpayload_trade(const UniValue& params, bool fHelp);
UniValue zus_createpayload_canceltradesbyprice(const UniValue& params, bool fHelp);
UniValue zus_createpayload_canceltradesbypair(const UniValue& params, bool fHelp);
UniValue zus_createpayload_cancelalltrades(const UniValue& params, bool fHelp);

#endif // ZURBANK_RPCPAYLOAD_H
