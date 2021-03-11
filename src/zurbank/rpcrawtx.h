#ifndef ZURBANK_RPCRAWTX_H
#define ZURBANK_RPCRAWTX_H

#include <univalue.h>

UniValue zus_decodetransaction(const UniValue& params, bool fHelp);
UniValue zus_createrawtx_opreturn(const UniValue& params, bool fHelp);
UniValue zus_createrawtx_multisig(const UniValue& params, bool fHelp);
UniValue zus_createrawtx_input(const UniValue& params, bool fHelp);
UniValue zus_createrawtx_reference(const UniValue& params, bool fHelp);
UniValue zus_createrawtx_change(const UniValue& params, bool fHelp);


#endif // ZURBANK_RPCRAWTX_H