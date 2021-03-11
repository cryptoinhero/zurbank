#ifndef ZURBANK_RPCTX
#define ZURBANK_RPCTX

#include <univalue.h>

UniValue zus_sendrawtx(const UniValue& params, bool fHelp);
UniValue zus_send(const UniValue& params, bool fHelp);
UniValue zus_sendall(const UniValue& params, bool fHelp);
UniValue zus_senddexsell(const UniValue& params, bool fHelp);
UniValue zus_senddexaccept(const UniValue& params, bool fHelp);
UniValue zus_sendissuancecrowdsale(const UniValue& params, bool fHelp);
UniValue zus_sendissuancefixed(const UniValue& params, bool fHelp);
UniValue zus_sendissuancemanaged(const UniValue& params, bool fHelp);
UniValue zus_sendsto(const UniValue& params, bool fHelp);
UniValue zus_sendgrant(const UniValue& params, bool fHelp);
UniValue zus_sendrevoke(const UniValue& params, bool fHelp);
UniValue zus_sendclosecrowdsale(const UniValue& params, bool fHelp);
UniValue trade_MP(const UniValue& params, bool fHelp);
UniValue zus_sendtrade(const UniValue& params, bool fHelp);
UniValue zus_sendcanceltradesbyprice(const UniValue& params, bool fHelp);
UniValue zus_sendcanceltradesbypair(const UniValue& params, bool fHelp);
UniValue zus_sendcancelalltrades(const UniValue& params, bool fHelp);
UniValue zus_sendchangeissuer(const UniValue& params, bool fHelp);
UniValue zus_sendactivation(const UniValue& params, bool fHelp);
UniValue zus_sendalert(const UniValue& params, bool fHelp);

#endif // ZURBANK_RPCTX
