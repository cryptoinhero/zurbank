/**
 * @file rpc.cpp
 *
 * This file contains RPC calls for data retrieval.
 */

#include "zurbank/rpc.h"

#include "zurbank/activation.h"
#include "zurbank/consensushash.h"
#include "zurbank/convert.h"
#include "zurbank/dbfees.h"
#include "zurbank/dbspinfo.h"
#include "zurbank/dbstolist.h"
#include "zurbank/dbtradelist.h"
#include "zurbank/dbtxlist.h"
#include "zurbank/dex.h"
#include "zurbank/errors.h"
#include "zurbank/log.h"
#include "zurbank/mdex.h"
#include "zurbank/notifications.h"
#include "zurbank/zurbank.h"
#include "zurbank/parsing.h"
#include "zurbank/rpcrequirements.h"
#include "zurbank/rpctx.h"
#include "zurbank/rpctxobject.h"
#include "zurbank/rpcvalues.h"
#include "zurbank/rules.h"
#include "zurbank/sp.h"
#include "zurbank/sto.h"
#include "zurbank/tally.h"
#include "zurbank/tx.h"
#include "zurbank/utilszurcoin.h"
#include "zurbank/version.h"
#include "zurbank/walletfetchtxs.h"
#include "zurbank/walletutils.h"

#include "amount.h"
#include "base58.h"
#include "chainparams.h"
#include "init.h"
#include "main.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "rpc/server.h"
#include "tinyformat.h"
#include "txmempool.h"
#include "uint256.h"
#include "utilstrencodings.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <univalue.h>

#include <stdint.h>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

using std::runtime_error;
using namespace mastercore;

/**
 * Throws a JSONRPCError, depending on error code.
 */
void PopulateFailure(int error)
{
    switch (error) {
        case MP_TX_NOT_FOUND:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
        case MP_TX_UNCONFIRMED:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unconfirmed transactions are not supported");
        case MP_BLOCK_NOT_IN_CHAIN:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not part of the active chain");
        case MP_CROWDSALE_WITHOUT_PROPERTY:
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Potential database corruption: \
                                                  \"Crowdsale Purchase\" without valid property identifier");
        case MP_INVALID_TX_IN_DB_FOUND:
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Potential database corruption: Invalid transaction found");
        case MP_TX_IS_NOT_OMNI_PROTOCOL:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No Zus Layer Protocol transaction");
    }
    throw JSONRPCError(RPC_INTERNAL_ERROR, "Generic transaction population failure");
}

void PropertyToJSON(const CMPSPInfo::Entry& sProperty, UniValue& property_obj)
{
    property_obj.push_back(Pair("name", sProperty.name));
    property_obj.push_back(Pair("category", sProperty.category));
    property_obj.push_back(Pair("subcategory", sProperty.subcategory));
    property_obj.push_back(Pair("data", sProperty.data));
    property_obj.push_back(Pair("url", sProperty.url));
    property_obj.push_back(Pair("divisible", sProperty.isDivisible()));
}

void MetaDexObjectToJSON(const CMPMetaDEx& obj, UniValue& metadex_obj)
{
    bool propertyIdForSaleIsDivisible = isPropertyDivisible(obj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(obj.getDesProperty());
    // add data to JSON object
    metadex_obj.push_back(Pair("address", obj.getAddr()));
    metadex_obj.push_back(Pair("txid", obj.getHash().GetHex()));
    if (obj.getAction() == 4) metadex_obj.push_back(Pair("ecosystem", isTestEcosystemProperty(obj.getProperty()) ? "test" : "main"));
    metadex_obj.push_back(Pair("propertyidforsale", (uint64_t) obj.getProperty()));
    metadex_obj.push_back(Pair("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible));
    metadex_obj.push_back(Pair("amountforsale", FormatMP(obj.getProperty(), obj.getAmountForSale())));
    metadex_obj.push_back(Pair("amountremaining", FormatMP(obj.getProperty(), obj.getAmountRemaining())));
    metadex_obj.push_back(Pair("propertyiddesired", (uint64_t) obj.getDesProperty()));
    metadex_obj.push_back(Pair("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible));
    metadex_obj.push_back(Pair("amountdesired", FormatMP(obj.getDesProperty(), obj.getAmountDesired())));
    metadex_obj.push_back(Pair("amounttofill", FormatMP(obj.getDesProperty(), obj.getAmountToFill())));
    metadex_obj.push_back(Pair("action", (int) obj.getAction()));
    metadex_obj.push_back(Pair("block", obj.getBlock()));
    metadex_obj.push_back(Pair("blocktime", obj.getBlockTime()));
}

void MetaDexObjectsToJSON(std::vector<CMPMetaDEx>& vMetaDexObjs, UniValue& response)
{
    MetaDEx_compare compareByHeight;

    // sorts metadex objects based on block height and position in block
    std::sort (vMetaDexObjs.begin(), vMetaDexObjs.end(), compareByHeight);

    for (std::vector<CMPMetaDEx>::const_iterator it = vMetaDexObjs.begin(); it != vMetaDexObjs.end(); ++it) {
        UniValue metadex_obj(UniValue::VOBJ);
        MetaDexObjectToJSON(*it, metadex_obj);

        response.push_back(metadex_obj);
    }
}

bool BalanceToJSON(const std::string& address, uint32_t property, UniValue& balance_obj, bool divisible)
{
    // confirmed balance minus unconfirmed, spent amounts
    int64_t nAvailable = GetAvailableTokenBalance(address, property);
    int64_t nReserved = GetReservedTokenBalance(address, property);
    int64_t nFrozen = GetFrozenTokenBalance(address, property);

    if (divisible) {
        balance_obj.push_back(Pair("balance", FormatDivisibleMP(nAvailable)));
        balance_obj.push_back(Pair("reserved", FormatDivisibleMP(nReserved)));
        balance_obj.push_back(Pair("frozen", FormatDivisibleMP(nFrozen)));
    } else {
        balance_obj.push_back(Pair("balance", FormatIndivisibleMP(nAvailable)));
        balance_obj.push_back(Pair("reserved", FormatIndivisibleMP(nReserved)));
        balance_obj.push_back(Pair("frozen", FormatIndivisibleMP(nFrozen)));
    }

    return (nAvailable || nReserved || nFrozen);
}

// Obtains details of a fee distribution
UniValue zus_getfeedistribution(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getfeedistribution distributionid\n"
            "\nGet the details for a fee distribution.\n"
            "\nArguments:\n"
            "1. distributionid           (number, required) the distribution to obtain details for\n"
            "\nResult:\n"
            "{\n"
            "  \"distributionid\" : n,          (number) the distribution id\n"
            "  \"propertyid\" : n,              (number) the property id of the distributed tokens\n"
            "  \"block\" : n,                   (number) the block the distribution occurred\n"
            "  \"amount\" : \"n.nnnnnnnn\",     (string) the amount that was distributed\n"
            "  \"recipients\": [                (array of JSON objects) a list of recipients\n"
            "    {\n"
            "      \"address\" : \"address\",          (string) the address of the recipient\n"
            "      \"amount\" : \"n.nnnnnnnn\"         (string) the amount of fees received by the recipient\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getfeedistribution", "1")
            + HelpExampleRpc("zus_getfeedistribution", "1")
        );

    int id = params[0].get_int();

    int block = 0;
    uint32_t propertyId = 0;
    int64_t total = 0;

    UniValue response(UniValue::VOBJ);

    bool found = pDbFeeHistory->GetDistributionData(id, &propertyId, &block, &total);
    if (!found) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Fee distribution ID does not exist");
    }
    response.push_back(Pair("distributionid", id));
    response.push_back(Pair("propertyid", (uint64_t)propertyId));
    response.push_back(Pair("block", block));
    response.push_back(Pair("amount", FormatMP(propertyId, total)));
    UniValue recipients(UniValue::VARR);
    std::set<std::pair<std::string,int64_t> > sRecipients = pDbFeeHistory->GetFeeDistribution(id);
    bool divisible = isPropertyDivisible(propertyId);
    if (!sRecipients.empty()) {
        for (std::set<std::pair<std::string,int64_t> >::iterator it = sRecipients.begin(); it != sRecipients.end(); it++) {
            std::string address = (*it).first;
            int64_t amount = (*it).second;
            UniValue recipient(UniValue::VOBJ);
            recipient.push_back(Pair("address", address));
            if (divisible) {
                recipient.push_back(Pair("amount", FormatDivisibleMP(amount)));
            } else {
                recipient.push_back(Pair("amount", FormatIndivisibleMP(amount)));
            }
            recipients.push_back(recipient);
        }
    }
    response.push_back(Pair("recipients", recipients));
    return response;
}

// Obtains all fee distributions for a property
// TODO : Split off code to populate a fee distribution object into a seperate function
UniValue zus_getfeedistributions(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getfeedistributions propertyid\n"
            "\nGet the details of all fee distributions for a property.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the property id to retrieve distributions for\n"
            "\nResult:\n"
            "[                       (array of JSON objects)\n"
            "  {\n"
            "    \"distributionid\" : n,          (number) the distribution id\n"
            "    \"propertyid\" : n,              (number) the property id of the distributed tokens\n"
            "    \"block\" : n,                   (number) the block the distribution occurred\n"
            "    \"amount\" : \"n.nnnnnnnn\",     (string) the amount that was distributed\n"
            "    \"recipients\": [                (array of JSON objects) a list of recipients\n"
            "      {\n"
            "        \"address\" : \"address\",          (string) the address of the recipient\n"
            "        \"amount\" : \"n.nnnnnnnn\"         (string) the amount of fees received by the recipient\n"
            "      },\n"
            "      ...\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getfeedistributions", "1")
            + HelpExampleRpc("zus_getfeedistributions", "1")
        );

    uint32_t prop = ParsePropertyId(params[0]);
    RequireExistingProperty(prop);

    UniValue response(UniValue::VARR);

    std::set<int> sDistributions = pDbFeeHistory->GetDistributionsForProperty(prop);
    if (!sDistributions.empty()) {
        for (std::set<int>::iterator it = sDistributions.begin(); it != sDistributions.end(); it++) {
            int id = *it;
            int block = 0;
            uint32_t propertyId = 0;
            int64_t total = 0;
            UniValue responseObj(UniValue::VOBJ);
            bool found = pDbFeeHistory->GetDistributionData(id, &propertyId, &block, &total);
            if (!found) {
                PrintToLog("Fee History Error - Distribution data not found for distribution ID %d but it was included in GetDistributionsForProperty(prop %d)\n", id, prop);
                continue;
            }
            responseObj.push_back(Pair("distributionid", id));
            responseObj.push_back(Pair("propertyid", (uint64_t)propertyId));
            responseObj.push_back(Pair("block", block));
            responseObj.push_back(Pair("amount", FormatMP(propertyId, total)));
            UniValue recipients(UniValue::VARR);
            std::set<std::pair<std::string,int64_t> > sRecipients = pDbFeeHistory->GetFeeDistribution(id);
            bool divisible = isPropertyDivisible(propertyId);
            if (!sRecipients.empty()) {
                for (std::set<std::pair<std::string,int64_t> >::iterator it = sRecipients.begin(); it != sRecipients.end(); it++) {
                    std::string address = (*it).first;
                    int64_t amount = (*it).second;
                    UniValue recipient(UniValue::VOBJ);
                    recipient.push_back(Pair("address", address));
                    if (divisible) {
                        recipient.push_back(Pair("amount", FormatDivisibleMP(amount)));
                    } else {
                        recipient.push_back(Pair("amount", FormatIndivisibleMP(amount)));
                    }
                    recipients.push_back(recipient);
                }
            }
            responseObj.push_back(Pair("recipients", recipients));
            response.push_back(responseObj);
        }
    }
    return response;
}

// Obtains the trigger value for fee distribution for a/all properties
UniValue zus_getfeetrigger(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "zus_getfeetrigger ( propertyid )\n"
            "\nReturns the amount of fees required in the cache to trigger distribution.\n"
            "\nArguments:\n"
            "1. propertyid           (number, optional) filter the results on this property id\n"
            "\nResult:\n"
            "[                       (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : nnnnnnn,          (number) the property id\n"
            "    \"feetrigger\" : \"n.nnnnnnnn\",   (string) the amount of fees required to trigger distribution\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getfeetrigger", "3")
            + HelpExampleRpc("zus_getfeetrigger", "3")
        );

    uint32_t propertyId = 0;
    if (0 < params.size()) {
        propertyId = ParsePropertyId(params[0]);
    }

    if (propertyId > 0) {
        RequireExistingProperty(propertyId);
    }

    UniValue response(UniValue::VARR);

    for (uint8_t ecosystem = 1; ecosystem <= 2; ecosystem++) {
        uint32_t startPropertyId = (ecosystem == 1) ? 1 : TEST_ECO_PROPERTY_1;
        for (uint32_t itPropertyId = startPropertyId; itPropertyId < pDbSpInfo->peekNextSPID(ecosystem); itPropertyId++) {
            if (propertyId == 0 || propertyId == itPropertyId) {
                int64_t feeTrigger = pDbFeeCache->GetDistributionThreshold(itPropertyId);
                std::string strFeeTrigger = FormatMP(itPropertyId, feeTrigger);
                UniValue cacheObj(UniValue::VOBJ);
                cacheObj.push_back(Pair("propertyid", (uint64_t)itPropertyId));
                cacheObj.push_back(Pair("feetrigger", strFeeTrigger));
                response.push_back(cacheObj);
            }
        }
    }

    return response;
}

// Provides the fee share the wallet (or specific address) will receive from fee distributions
UniValue zus_getfeeshare(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "zus_getfeeshare ( address ecosystem )\n"
            "\nReturns the percentage share of fees distribution applied to the wallet (default) or address (if supplied).\n"
            "\nArguments:\n"
            "1. address              (string, optional) retrieve the fee share for the supplied address\n"
            "2. ecosystem            (number, optional) the ecosystem to check the fee share (1 for main ecosystem, 2 for test ecosystem)\n"
            "\nResult:\n"
            "[                       (array of JSON objects)\n"
            "  {\n"
            "    \"address\" : nnnnnnn,          (number) the property id\n"
            "    \"feeshare\" : \"n.nnnnnnnn\",   (string) the percentage of fees this address will receive based on current state\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getfeeshare", "\"UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6\" 1")
            + HelpExampleRpc("zus_getfeeshare", "\"UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6\", 1")
        );

    std::string address;
    uint8_t ecosystem = 1;
    if (0 < params.size()) {
        if ("*" != params[0].get_str()) { //ParseAddressOrEmpty doesn't take wildcards
            address = ParseAddressOrEmpty(params[0]);
        } else {
            address = "*";
        }
    }
    if (1 < params.size()) {
        ecosystem = ParseEcosystem(params[1]);
    }

    UniValue response(UniValue::VARR);
    bool addObj = false;

    OwnerAddrType receiversSet;
    if (ecosystem == 1) {
        receiversSet = STO_GetReceivers("FEEDISTRIBUTION", OMNI_PROPERTY_MSC, COIN);
    } else {
        receiversSet = STO_GetReceivers("FEEDISTRIBUTION", OMNI_PROPERTY_TMSC, COIN);
    }

    for (OwnerAddrType::reverse_iterator it = receiversSet.rbegin(); it != receiversSet.rend(); ++it) {
        addObj = false;
        if (address.empty()) {
            if (IsMyAddress(it->second)) {
                addObj = true;
            }
        } else if (address == it->second || address == "*") {
            addObj = true;
        }
        if (addObj) {
            UniValue feeShareObj(UniValue::VOBJ);
            // NOTE: using float here as this is a display value only which isn't an exact percentage and
            //       changes block to block (due to dev Zus) so high precision not required(?)
            double feeShare = (double(it->first) / double(COIN)) * (double)100;
            std::string strFeeShare = strprintf("%.4f", feeShare);
            strFeeShare += "%";
            feeShareObj.push_back(Pair("address", it->second));
            feeShareObj.push_back(Pair("feeshare", strFeeShare));
            response.push_back(feeShareObj);
        }
    }

    return response;
}

// Provides the current values of the fee cache
UniValue zus_getfeecache(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "zus_getfeecache ( propertyid )\n"
            "\nReturns the amount of fees cached for distribution.\n"
            "\nArguments:\n"
            "1. propertyid           (number, optional) filter the results on this property id\n"
            "\nResult:\n"
            "[                       (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : nnnnnnn,          (number) the property id\n"
            "    \"cachedfees\" : \"n.nnnnnnnn\",   (string) the amount of fees cached for this property\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getfeecache", "31")
            + HelpExampleRpc("zus_getfeecache", "31")
        );

    uint32_t propertyId = 0;
    if (0 < params.size()) {
        propertyId = ParsePropertyId(params[0]);
    }

    if (propertyId > 0) {
        RequireExistingProperty(propertyId);
    }

    UniValue response(UniValue::VARR);

    for (uint8_t ecosystem = 1; ecosystem <= 2; ecosystem++) {
        uint32_t startPropertyId = (ecosystem == 1) ? 1 : TEST_ECO_PROPERTY_1;
        for (uint32_t itPropertyId = startPropertyId; itPropertyId < pDbSpInfo->peekNextSPID(ecosystem); itPropertyId++) {
            if (propertyId == 0 || propertyId == itPropertyId) {
                int64_t cachedFee = pDbFeeCache->GetCachedAmount(itPropertyId);
                if (cachedFee == 0) {
                    // filter empty results unless the call specifically requested this property
                    if (propertyId != itPropertyId) continue;
                }
                std::string strFee = FormatMP(itPropertyId, cachedFee);
                UniValue cacheObj(UniValue::VOBJ);
                cacheObj.push_back(Pair("propertyid", (uint64_t)itPropertyId));
                cacheObj.push_back(Pair("cachedfees", strFee));
                response.push_back(cacheObj);
            }
        }
    }

    return response;
}

// generate a list of seed blocks based on the data in LevelDB
UniValue zus_getseedblocks(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "zus_getseedblocks startblock endblock\n"
            "\nReturns a list of blocks containing Zus transactions for use in seed block filtering.\n"
            "\nWARNING: The Exodus crowdsale is not stored in LevelDB, thus this is currently only safe to use to generate seed blocks after block 255365."
            "\nArguments:\n"
            "1. startblock           (number, required) the first block to look for Zus transactions (inclusive)\n"
            "2. endblock             (number, required) the last block to look for Zus transactions (inclusive)\n"
            "\nResult:\n"
            "[                     (array of numbers) a list of seed blocks\n"
            "   nnnnnn,              (number) the block height of the seed block\n"
            "   ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getseedblocks", "290000 300000")
            + HelpExampleRpc("zus_getseedblocks", "290000, 300000")
        );

    int startHeight = params[0].get_int();
    int endHeight = params[1].get_int();

    RequireHeightInChain(startHeight);
    RequireHeightInChain(endHeight);

    UniValue response(UniValue::VARR);

    {
        LOCK(cs_tally);
        std::set<int> setSeedBlocks = pDbTransactionList->GetSeedBlocks(startHeight, endHeight);
        for (std::set<int>::const_iterator it = setSeedBlocks.begin(); it != setSeedBlocks.end(); ++it) {
            response.push_back(*it);
        }
    }

    return response;
}

// obtain the payload for a transaction
UniValue zus_getpayload(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getpayload \"txid\"\n"
            "\nGet the payload for an Zus transaction.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the transaction to retrieve payload\n"
            "\nResult:\n"
            "{\n"
            "  \"payload\" : \"payloadmessage\",       (string) the decoded Zus payload message\n"
            "  \"payloadsize\" : n                     (number) the size of the payload\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getpayload", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("zus_getpayload", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 txid = ParseHashV(params[0], "txid");

    CTransaction tx;
    uint256 blockHash;
    if (!GetTransaction(txid, tx, Params().GetConsensus(), blockHash, true)) {
        PopulateFailure(MP_TX_NOT_FOUND);
    }

    int blockTime = 0;
    int blockHeight = GetHeight();
    if (!blockHash.IsNull()) {
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL != pBlockIndex) {
            blockTime = pBlockIndex->nTime;
            blockHeight = pBlockIndex->nHeight;
        }
    }

    CMPTransaction mp_obj;
    int parseRC = ParseTransaction(tx, blockHeight, 0, mp_obj, blockTime);
    if (parseRC < 0) PopulateFailure(MP_TX_IS_NOT_OMNI_PROTOCOL);

    UniValue payloadObj(UniValue::VOBJ);
    payloadObj.push_back(Pair("payload", mp_obj.getPayload()));
    payloadObj.push_back(Pair("payloadsize", mp_obj.getPayloadSize()));
    return payloadObj;
}

// determine whether to automatically commit transactions
UniValue zus_setautocommit(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_setautocommit flag\n"
            "\nSets the global flag that determines whether transactions are automatically committed and broadcast.\n"
            "\nArguments:\n"
            "1. flag                 (boolean, required) the flag\n"
            "\nResult:\n"
            "true|false              (boolean) the updated flag status\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_setautocommit", "false")
            + HelpExampleRpc("zus_setautocommit", "false")
        );

    LOCK(cs_tally);

    autoCommit = params[0].get_bool();
    return autoCommit;
}

// display the tally map & the offer/accept list(s)
UniValue mscrpc(const UniValue& params, bool fHelp)
{
    int extra = 0;
    int extra2 = 0, extra3 = 0;

    if (fHelp || params.size() > 3)
        throw runtime_error(
            "mscrpc\n"
            "\nReturns the number of blocks in the longest block chain.\n"
            "\nResult:\n"
            "n    (number) the current block count\n"
            "\nExamples:\n"
            + HelpExampleCli("mscrpc", "")
            + HelpExampleRpc("mscrpc", "")
        );

    if (0 < params.size()) extra = atoi(params[0].get_str());
    if (1 < params.size()) extra2 = atoi(params[1].get_str());
    if (2 < params.size()) extra3 = atoi(params[2].get_str());

    PrintToConsole("%s(extra=%d,extra2=%d,extra3=%d)\n", __FUNCTION__, extra, extra2, extra3);

    bool bDivisible = isPropertyDivisible(extra2);

    // various extra tests
    switch (extra) {
        case 0:
        {
            LOCK(cs_tally);
            int64_t total = 0;
            // display all balances
            for (std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
                PrintToConsole("%34s => ", my_it->first);
                total += (my_it->second).print(extra2, bDivisible);
            }
            PrintToConsole("total for property %d  = %X is %s\n", extra2, extra2, FormatDivisibleMP(total));
            break;
        }
        case 1:
        {
            LOCK(cs_tally);
            // display the whole CMPTxList (leveldb)
            pDbTransactionList->printAll();
            pDbTransactionList->printStats();
            break;
        }
        case 2:
        {
            LOCK(cs_tally);
            // display smart properties
            pDbSpInfo->printAll();
            break;
        }
        case 3:
        {
            LOCK(cs_tally);
            uint32_t id = 0;
            // for each address display all currencies it holds
            for (std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
                PrintToConsole("%34s => ", my_it->first);
                (my_it->second).print(extra2);
                (my_it->second).init();
                while (0 != (id = (my_it->second).next())) {
                    PrintToConsole("Id: %u=0x%X ", id, id);
                }
                PrintToConsole("\n");
            }
            break;
        }
        case 4:
        {
            LOCK(cs_tally);
            for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
                (it->second).print(it->first);
            }
            break;
        }
        case 5:
        {
            LOCK(cs_tally);
            PrintToConsole("isMPinBlockRange(%d,%d)=%s\n", extra2, extra3, pDbTransactionList->isMPinBlockRange(extra2, extra3, false) ? "YES" : "NO");
            break;
        }
        case 6:
        {
            LOCK(cs_tally);
            MetaDEx_debug_print(true, true);
            break;
        }
        case 7:
        {
            LOCK(cs_tally);
            // display the whole CMPTradeList (leveldb)
            pDbTradeList->printAll();
            pDbTradeList->printStats();
            break;
        }
        case 8:
        {
            LOCK(cs_tally);
            // display the STO receive list
            pDbStoList->printAll();
            pDbStoList->printStats();
            break;
        }
        case 9:
        {
            PrintToConsole("Locking cs_tally for %d milliseconds..\n", extra2);
            LOCK(cs_tally);
            MilliSleep(extra2);
            PrintToConsole("Unlocking cs_tally now\n");
            break;
        }
        case 10:
        {
            PrintToConsole("Locking cs_main for %d milliseconds..\n", extra2);
            LOCK(cs_main);
            MilliSleep(extra2);
            PrintToConsole("Unlocking cs_main now\n");
            break;
        }
#ifdef ENABLE_WALLET
        case 11:
        {
            PrintToConsole("Locking pwalletMain->cs_wallet for %d milliseconds..\n", extra2);
            LOCK(pwalletMain->cs_wallet);
            MilliSleep(extra2);
            PrintToConsole("Unlocking pwalletMain->cs_wallet now\n");
            break;
        }
#endif
        case 14:
        {
            LOCK(cs_tally);
            pDbFeeCache->printAll();
            pDbFeeCache->printStats();

            int64_t a = 1000;
            int64_t b = 1999;
            int64_t c = 2001;
            int64_t d = 20001;
            int64_t e = 19999;

            int64_t z = 2000;

            int64_t aa = a/z;
            int64_t bb = b/z;
            int64_t cc = c/z;
            int64_t dd = d/z;
            int64_t ee = e/z;

            PrintToConsole("%d / %d = %d\n",a,z,aa);
            PrintToConsole("%d / %d = %d\n",b,z,bb);
            PrintToConsole("%d / %d = %d\n",c,z,cc);
            PrintToConsole("%d / %d = %d\n",d,z,dd);
            PrintToConsole("%d / %d = %d\n",e,z,ee);

            break;
        }
        default:
            break;
    }

    return GetHeight();
}

// display an MP balance via RPC
UniValue zus_getbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "zus_getbalance \"address\" propertyid\n"
            "\nReturns the token balance for a given address and property.\n"
            "\nArguments:\n"
            "1. address              (string, required) the address\n"
            "2. propertyid           (number, required) the property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
            "  \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
            "  \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getbalance", "\"UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6\" 1")
            + HelpExampleRpc("zus_getbalance", "\"UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6\", 1")
        );

    std::string address = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);

    RequireExistingProperty(propertyId);

    UniValue balanceObj(UniValue::VOBJ);
    BalanceToJSON(address, propertyId, balanceObj, isPropertyDivisible(propertyId));

    return balanceObj;
}

UniValue zus_getallbalancesforid(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getallbalancesforid propertyid\n"
            "\nReturns a list of token balances for a given currency or property identifier.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the property identifier\n"
            "\nResult:\n"
            "[                           (array of JSON objects)\n"
            "  {\n"
            "    \"address\" : \"address\",      (string) the address\n"
            "    \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
            "    \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
            "    \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getallbalancesforid", "1")
            + HelpExampleRpc("zus_getallbalancesforid", "1")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    RequireExistingProperty(propertyId);

    UniValue response(UniValue::VARR);
    bool isDivisible = isPropertyDivisible(propertyId); // we want to check this BEFORE the loop

    LOCK(cs_tally);

    for (std::unordered_map<std::string, CMPTally>::iterator it = mp_tally_map.begin(); it != mp_tally_map.end(); ++it) {
        uint32_t id = 0;
        bool includeAddress = false;
        std::string address = it->first;
        (it->second).init();
        while (0 != (id = (it->second).next())) {
            if (id == propertyId) {
                includeAddress = true;
                break;
            }
        }
        if (!includeAddress) {
            continue; // ignore this address, has never transacted in this propertyId
        }
        UniValue balanceObj(UniValue::VOBJ);
        balanceObj.push_back(Pair("address", address));
        bool nonEmptyBalance = BalanceToJSON(address, propertyId, balanceObj, isDivisible);

        if (nonEmptyBalance) {
            response.push_back(balanceObj);
        }
    }

    return response;
}

UniValue zus_getallbalancesforaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getallbalancesforaddress \"address\"\n"
            "\nReturns a list of all token balances for a given address.\n"
            "\nArguments:\n"
            "1. address              (string, required) the address\n"
            "\nResult:\n"
            "[                           (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : n,           (number) the property identifier\n"
            "    \"name\" : \"name\",            (string) the name of the property\n"
            "    \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
            "    \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
            "    \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getallbalancesforaddress", "\"UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6\"")
            + HelpExampleRpc("zus_getallbalancesforaddress", "\"UrpY6GsjF5WK33TzeiS8mQCPxzMdvbizp6\"")
        );

    std::string address = ParseAddress(params[0]);

    UniValue response(UniValue::VARR);

    LOCK(cs_tally);

    CMPTally* addressTally = getTally(address);

    if (NULL == addressTally) { // addressTally object does not exist
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Address not found");
    }

    addressTally->init();

    uint32_t propertyId = 0;
    while (0 != (propertyId = addressTally->next())) {
        CMPSPInfo::Entry property;
        if (!pDbSpInfo->getSP(propertyId, property)) {
            continue;
        }

        UniValue balanceObj(UniValue::VOBJ);
        balanceObj.push_back(Pair("propertyid", (uint64_t) propertyId));
        balanceObj.push_back(Pair("name", property.name));

        bool nonEmptyBalance = BalanceToJSON(address, propertyId, balanceObj, property.isDivisible());

        if (nonEmptyBalance) {
            response.push_back(balanceObj);
        }
    }

    return response;
}

/** Returns all addresses that may be mine. */
static std::set<std::string> getWalletAddresses(bool fIncludeWatchOnly)
{
    std::set<std::string> result;

#ifdef ENABLE_WALLET
    LOCK(pwalletMain->cs_wallet);

    BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, pwalletMain->mapAddressBook) {
        const CBitcoinAddress& address = item.first;
        isminetype iIsMine = IsMine(*pwalletMain, address.Get());

        if (iIsMine == ISMINE_SPENDABLE || (fIncludeWatchOnly && iIsMine != ISMINE_NO)) {
            result.insert(address.ToString());
        }
    }
#endif

    return result;
}

UniValue zus_getwalletbalances(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "zus_getwalletbalances ( includewatchonly )\n"
            "\nReturns a list of the total token balances of the whole wallet.\n"
            "\nArguments:\n"
            "1. includewatchonly     (boolean, optional) include balances of watchonly addresses (default: false)\n"
            "\nResult:\n"
            "[                           (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : n,         (number) the property identifier\n"
            "    \"name\" : \"name\",            (string) the name of the token\n"
            "    \"balance\" : \"n.nnnnnnnn\",   (string) the total available balance for the token\n"
            "    \"reserved\" : \"n.nnnnnnnn\"   (string) the total amount reserved by sell offers and accepts\n"
            "    \"frozen\" : \"n.nnnnnnnn\"     (string) the total amount frozen by the issuer (applies to managed properties only)\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getwalletbalances", "")
            + HelpExampleRpc("zus_getwalletbalances", "")
        );

    bool fIncludeWatchOnly = false;
    if (params.size() > 0) {
        fIncludeWatchOnly = params[0].get_bool();
    }

    UniValue response(UniValue::VARR);

#ifdef ENABLE_WALLET
    if (!pwalletMain) {
        return response;
    }

    std::set<std::string> addresses = getWalletAddresses(fIncludeWatchOnly);
    std::map<uint32_t, std::tuple<int64_t, int64_t, int64_t>> balances;

    LOCK(cs_tally);
    BOOST_FOREACH(const std::string& address, addresses) {
        CMPTally* addressTally = getTally(address);
        if (NULL == addressTally) {
            continue; // address doesn't have tokens
        }

        uint32_t propertyId = 0;
        addressTally->init();

        while (0 != (propertyId = addressTally->next())) {
            int64_t nAvailable = GetAvailableTokenBalance(address, propertyId);
            int64_t nReserved = GetReservedTokenBalance(address, propertyId);
            int64_t nFrozen = GetFrozenTokenBalance(address, propertyId);

            if (!nAvailable && !nReserved && !nFrozen) {
                continue;
            }

            std::tuple<int64_t, int64_t, int64_t> currentBalance{0, 0, 0};
            if (balances.find(propertyId) != balances.end()) {
                currentBalance = balances[propertyId];
            }

            currentBalance = std::make_tuple(
                    std::get<0>(currentBalance) + nAvailable,
                    std::get<1>(currentBalance) + nReserved,
                    std::get<2>(currentBalance) + nFrozen
            );

            balances[propertyId] = currentBalance;
        }
    }

    for (auto const& item : balances) {
        uint32_t propertyId = item.first;
        std::tuple<int64_t, int64_t, int64_t> balance = item.second;

        CMPSPInfo::Entry property;
        if (!pDbSpInfo->getSP(propertyId, property)) {
            continue; // token wasn't found in the DB
        }

        int64_t nAvailable = std::get<0>(balance);
        int64_t nReserved = std::get<1>(balance);
        int64_t nFrozen = std::get<2>(balance);

        UniValue objBalance(UniValue::VOBJ);
        objBalance.push_back(Pair("propertyid", (uint64_t) propertyId));
        objBalance.push_back(Pair("name", property.name));

        if (property.isDivisible()) {
            objBalance.push_back(Pair("balance", FormatDivisibleMP(nAvailable)));
            objBalance.push_back(Pair("reserved", FormatDivisibleMP(nReserved)));
            objBalance.push_back(Pair("frozen", FormatDivisibleMP(nFrozen)));
        } else {
            objBalance.push_back(Pair("balance", FormatIndivisibleMP(nAvailable)));
            objBalance.push_back(Pair("reserved", FormatIndivisibleMP(nReserved)));
            objBalance.push_back(Pair("frozen", FormatIndivisibleMP(nFrozen)));
        }

        response.push_back(objBalance);
    }
#endif

    return response;
}

UniValue zus_getwalletaddressbalances(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "zus_getwalletaddressbalances ( includewatchonly )\n"
            "\nReturns a list of all token balances for every wallet address.\n"
            "\nArguments:\n"
            "1. includewatchonly     (boolean, optional) include balances of watchonly addresses (default: false)\n"
            "\nResult:\n"
            "[                           (array of JSON objects)\n"
            "  {\n"
            "    \"address\" : \"address\",      (string) the address linked to the following balances\n"
            "    \"balances\" :\n"
            "    [\n"
            "      {\n"
            "        \"propertyid\" : n,         (number) the property identifier\n"
            "        \"name\" : \"name\",            (string) the name of the token\n"
            "        \"balance\" : \"n.nnnnnnnn\",   (string) the available balance for the token\n"
            "        \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
            "        \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
            "      },\n"
            "      ...\n"
            "    ]\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getwalletaddressbalances", "")
            + HelpExampleRpc("zus_getwalletaddressbalances", "")
        );

    bool fIncludeWatchOnly = false;
    if (params.size() > 0) {
        fIncludeWatchOnly = params[0].get_bool();
    }

    UniValue response(UniValue::VARR);

#ifdef ENABLE_WALLET
    if (!pwalletMain) {
        return response;
    }

    std::set<std::string> addresses = getWalletAddresses(fIncludeWatchOnly);

    LOCK(cs_tally);
    BOOST_FOREACH(const std::string& address, addresses) {
        CMPTally* addressTally = getTally(address);
        if (NULL == addressTally) {
            continue; // address doesn't have tokens
        }

        UniValue arrBalances(UniValue::VARR);
        uint32_t propertyId = 0;
        addressTally->init();

        while (0 != (propertyId = addressTally->next())) {
            CMPSPInfo::Entry property;
            if (!pDbSpInfo->getSP(propertyId, property)) {
                continue; // token wasn't found in the DB
            }

            UniValue objBalance(UniValue::VOBJ);
            objBalance.push_back(Pair("propertyid", (uint64_t) propertyId));
            objBalance.push_back(Pair("name", property.name));

            bool nonEmptyBalance = BalanceToJSON(address, propertyId, objBalance, property.isDivisible());

            if (nonEmptyBalance) {
                arrBalances.push_back(objBalance);
            }
        }

        if (arrBalances.size() > 0) {
            UniValue objEntry(UniValue::VOBJ);
            objEntry.push_back(Pair("address", address));
            objEntry.push_back(Pair("balances", arrBalances));
            response.push_back(objEntry);
        }
    }
#endif

    return response;
}

UniValue zus_getproperty(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getproperty propertyid\n"
            "\nReturns details for about the tokens or smart property to lookup.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens or property\n"
            "\nResult:\n"
            "{\n"
            "  \"propertyid\" : n,                (number) the identifier\n"
            "  \"name\" : \"name\",                 (string) the name of the tokens\n"
            "  \"category\" : \"category\",         (string) the category used for the tokens\n"
            "  \"subcategory\" : \"subcategory\",   (string) the subcategory used for the tokens\n"
            "  \"data\" : \"information\",          (string) additional information or a description\n"
            "  \"url\" : \"uri\",                   (string) an URI, for example pointing to a website\n"
            "  \"divisible\" : true|false,        (boolean) whether the tokens are divisible\n"
            "  \"issuer\" : \"address\",            (string) the Zurcoin address of the issuer on record\n"
            "  \"creationtxid\" : \"hash\",         (string) the hex-encoded creation transaction hash\n"
            "  \"fixedissuance\" : true|false,    (boolean) whether the token supply is fixed\n"
            "  \"managedissuance\" : true|false,    (boolean) whether the token supply is managed\n"
            "  \"totaltokens\" : \"n.nnnnnnnn\"     (string) the total number of tokens in existence\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getproperty", "3")
            + HelpExampleRpc("zus_getproperty", "3")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    RequireExistingProperty(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!pDbSpInfo->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }
    int64_t nTotalTokens = getTotalTokens(propertyId);
    std::string strCreationHash = sp.txid.GetHex();
    std::string strTotalTokens = FormatMP(propertyId, nTotalTokens);

    UniValue response(UniValue::VOBJ);
    response.push_back(Pair("propertyid", (uint64_t) propertyId));
    PropertyToJSON(sp, response); // name, category, subcategory, data, url, divisible
    response.push_back(Pair("issuer", sp.issuer));
    response.push_back(Pair("creationtxid", strCreationHash));
    response.push_back(Pair("fixedissuance", sp.fixed));
    response.push_back(Pair("managedissuance", sp.manual));
    if (sp.manual) {
        int currentBlock = GetHeight();
        LOCK(cs_tally);
        response.push_back(Pair("freezingenabled", isFreezingEnabled(propertyId, currentBlock)));
    }
    response.push_back(Pair("totaltokens", strTotalTokens));

    return response;
}

UniValue zus_listproperties(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "zus_listproperties\n"
            "\nLists all tokens or smart properties.\n"
            "\nResult:\n"
            "[                                (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : n,                (number) the identifier of the tokens\n"
            "    \"name\" : \"name\",                 (string) the name of the tokens\n"
            "    \"category\" : \"category\",         (string) the category used for the tokens\n"
            "    \"subcategory\" : \"subcategory\",   (string) the subcategory used for the tokens\n"
            "    \"data\" : \"information\",          (string) additional information or a description\n"
            "    \"url\" : \"uri\",                   (string) an URI, for example pointing to a website\n"
            "    \"divisible\" : true|false         (boolean) whether the tokens are divisible\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_listproperties", "")
            + HelpExampleRpc("zus_listproperties", "")
        );

    UniValue response(UniValue::VARR);

    LOCK(cs_tally);

    uint32_t nextSPID = pDbSpInfo->peekNextSPID(1);
    for (uint32_t propertyId = 1; propertyId < nextSPID; propertyId++) {
        CMPSPInfo::Entry sp;
        if (pDbSpInfo->getSP(propertyId, sp)) {
            UniValue propertyObj(UniValue::VOBJ);
            propertyObj.push_back(Pair("propertyid", (uint64_t) propertyId));
            PropertyToJSON(sp, propertyObj); // name, category, subcategory, data, url, divisible

            response.push_back(propertyObj);
        }
    }

    uint32_t nextTestSPID = pDbSpInfo->peekNextSPID(2);
    for (uint32_t propertyId = TEST_ECO_PROPERTY_1; propertyId < nextTestSPID; propertyId++) {
        CMPSPInfo::Entry sp;
        if (pDbSpInfo->getSP(propertyId, sp)) {
            UniValue propertyObj(UniValue::VOBJ);
            propertyObj.push_back(Pair("propertyid", (uint64_t) propertyId));
            PropertyToJSON(sp, propertyObj); // name, category, subcategory, data, url, divisible

            response.push_back(propertyObj);
        }
    }

    return response;
}

UniValue zus_getcrowdsale(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "zus_getcrowdsale propertyid ( verbose )\n"
            "\nReturns information about a crowdsale.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the crowdsale\n"
            "2. verbose              (boolean, optional) list crowdsale participants (default: false)\n"
            "\nResult:\n"
            "{\n"
            "  \"propertyid\" : n,                     (number) the identifier of the crowdsale\n"
            "  \"name\" : \"name\",                      (string) the name of the tokens issued via the crowdsale\n"
            "  \"active\" : true|false,                (boolean) whether the crowdsale is still active\n"
            "  \"issuer\" : \"address\",                 (string) the Zurcoin address of the issuer on record\n"
            "  \"propertyiddesired\" : n,              (number) the identifier of the tokens eligible to participate in the crowdsale\n"
            "  \"tokensperunit\" : \"n.nnnnnnnn\",       (string) the amount of tokens granted per unit invested in the crowdsale\n"
            "  \"earlybonus\" : n,                     (number) an early bird bonus for participants in percent per week\n"
            "  \"percenttoissuer\" : n,                (number) a percentage of tokens that will be granted to the issuer\n"
            "  \"starttime\" : nnnnnnnnnn,             (number) the start time of the of the crowdsale as Unix timestamp\n"
            "  \"deadline\" : nnnnnnnnnn,              (number) the deadline of the crowdsale as Unix timestamp\n"
            "  \"amountraised\" : \"n.nnnnnnnn\",        (string) the amount of tokens invested by participants\n"
            "  \"tokensissued\" : \"n.nnnnnnnn\",        (string) the total number of tokens issued via the crowdsale\n"
            "  \"issuerbonustokens\" : \"n.nnnnnnnn\",   (string) the amount of tokens granted to the issuer as bonus\n"
            "  \"addedissuertokens\" : \"n.nnnnnnnn\",   (string) the amount of issuer bonus tokens not yet emitted\n"
            "  \"closedearly\" : true|false,           (boolean) whether the crowdsale ended early (if not active)\n"
            "  \"maxtokens\" : true|false,             (boolean) whether the crowdsale ended early due to reaching the limit of max. issuable tokens (if not active)\n"
            "  \"endedtime\" : nnnnnnnnnn,             (number) the time when the crowdsale ended (if closed early)\n"
            "  \"closetx\" : \"hash\",                   (string) the hex-encoded hash of the transaction that closed the crowdsale (if closed manually)\n"
            "  \"participanttransactions\": [          (array of JSON objects) a list of crowdsale participations (if verbose=true)\n"
            "    {\n"
            "      \"txid\" : \"hash\",                      (string) the hex-encoded hash of participation transaction\n"
            "      \"amountsent\" : \"n.nnnnnnnn\",          (string) the amount of tokens invested by the participant\n"
            "      \"participanttokens\" : \"n.nnnnnnnn\",   (string) the tokens granted to the participant\n"
            "      \"issuertokens\" : \"n.nnnnnnnn\"         (string) the tokens granted to the issuer as bonus\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getcrowdsale", "3 true")
            + HelpExampleRpc("zus_getcrowdsale", "3, true")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);
    bool showVerbose = (params.size() > 1) ? params[1].get_bool() : false;

    RequireExistingProperty(propertyId);
    RequireCrowdsale(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!pDbSpInfo->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }

    const uint256& creationHash = sp.txid;

    CTransaction tx;
    uint256 hashBlock;
    if (!GetTransaction(creationHash, tx, Params().GetConsensus(), hashBlock, true)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
    }

    UniValue response(UniValue::VOBJ);
    bool active = isCrowdsaleActive(propertyId);
    std::map<uint256, std::vector<int64_t> > database;

    if (active) {
        bool crowdFound = false;

        LOCK(cs_tally);

        for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
            const CMPCrowd& crowd = it->second;
            if (propertyId == crowd.getPropertyId()) {
                crowdFound = true;
                database = crowd.getDatabase();
            }
        }
        if (!crowdFound) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Crowdsale is flagged active but cannot be retrieved");
        }
    } else {
        database = sp.historicalData;
    }

    int64_t tokensIssued = getTotalTokens(propertyId);
    const std::string& txidClosed = sp.txid_close.GetHex();

    int64_t startTime = -1;
    if (!hashBlock.IsNull() && GetBlockIndex(hashBlock)) {
        startTime = GetBlockIndex(hashBlock)->nTime;
    }

    // note the database is already deserialized here and there is minimal performance penalty to iterate recipients to calculate amountRaised
    int64_t amountRaised = 0;
    int64_t amountIssuerTokens = 0;
    uint16_t propertyIdType = isPropertyDivisible(propertyId) ? MSC_PROPERTY_TYPE_DIVISIBLE : MSC_PROPERTY_TYPE_INDIVISIBLE;
    uint16_t desiredIdType = isPropertyDivisible(sp.property_desired) ? MSC_PROPERTY_TYPE_DIVISIBLE : MSC_PROPERTY_TYPE_INDIVISIBLE;
    std::map<std::string, UniValue> sortMap;
    for (std::map<uint256, std::vector<int64_t> >::const_iterator it = database.begin(); it != database.end(); it++) {
        UniValue participanttx(UniValue::VOBJ);
        std::string txid = it->first.GetHex();
        amountRaised += it->second.at(0);
        amountIssuerTokens += it->second.at(3);
        participanttx.push_back(Pair("txid", txid));
        participanttx.push_back(Pair("amountsent", FormatByType(it->second.at(0), desiredIdType)));
        participanttx.push_back(Pair("participanttokens", FormatByType(it->second.at(2), propertyIdType)));
        participanttx.push_back(Pair("issuertokens", FormatByType(it->second.at(3), propertyIdType)));
        std::string sortKey = strprintf("%d-%s", it->second.at(1), txid);
        sortMap.insert(std::make_pair(sortKey, participanttx));
    }

    response.push_back(Pair("propertyid", (uint64_t) propertyId));
    response.push_back(Pair("name", sp.name));
    response.push_back(Pair("active", active));
    response.push_back(Pair("issuer", sp.issuer));
    response.push_back(Pair("propertyiddesired", (uint64_t) sp.property_desired));
    response.push_back(Pair("tokensperunit", FormatMP(propertyId, sp.num_tokens)));
    response.push_back(Pair("earlybonus", sp.early_bird));
    response.push_back(Pair("percenttoissuer", sp.percentage));
    response.push_back(Pair("starttime", startTime));
    response.push_back(Pair("deadline", sp.deadline));
    response.push_back(Pair("amountraised", FormatMP(sp.property_desired, amountRaised)));
    response.push_back(Pair("tokensissued", FormatMP(propertyId, tokensIssued)));
    response.push_back(Pair("issuerbonustokens", FormatMP(propertyId, amountIssuerTokens)));
    response.push_back(Pair("addedissuertokens", FormatMP(propertyId, sp.missedTokens)));

    // TODO: return fields every time?
    if (!active) response.push_back(Pair("closedearly", sp.close_early));
    if (!active) response.push_back(Pair("maxtokens", sp.max_tokens));
    if (sp.close_early) response.push_back(Pair("endedtime", sp.timeclosed));
    if (sp.close_early && !sp.max_tokens) response.push_back(Pair("closetx", txidClosed));

    if (showVerbose) {
        UniValue participanttxs(UniValue::VARR);
        for (std::map<std::string, UniValue>::iterator it = sortMap.begin(); it != sortMap.end(); ++it) {
            participanttxs.push_back(it->second);
        }
        response.push_back(Pair("participanttransactions", participanttxs));
    }

    return response;
}

UniValue zus_getactivecrowdsales(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "zus_getactivecrowdsales\n"
            "\nLists currently active crowdsales.\n"
            "\nResult:\n"
            "[                                 (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : n,                 (number) the identifier of the crowdsale\n"
            "    \"name\" : \"name\",                  (string) the name of the tokens issued via the crowdsale\n"
            "    \"issuer\" : \"address\",             (string) the Zurcoin address of the issuer on record\n"
            "    \"propertyiddesired\" : n,          (number) the identifier of the tokens eligible to participate in the crowdsale\n"
            "    \"tokensperunit\" : \"n.nnnnnnnn\",   (string) the amount of tokens granted per unit invested in the crowdsale\n"
            "    \"earlybonus\" : n,                 (number) an early bird bonus for participants in percent per week\n"
            "    \"percenttoissuer\" : n,            (number) a percentage of tokens that will be granted to the issuer\n"
            "    \"starttime\" : nnnnnnnnnn,         (number) the start time of the of the crowdsale as Unix timestamp\n"
            "    \"deadline\" : nnnnnnnnnn           (number) the deadline of the crowdsale as Unix timestamp\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getactivecrowdsales", "")
            + HelpExampleRpc("zus_getactivecrowdsales", "")
        );

    UniValue response(UniValue::VARR);

    LOCK2(cs_main, cs_tally);

    for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
        const CMPCrowd& crowd = it->second;
        uint32_t propertyId = crowd.getPropertyId();

        CMPSPInfo::Entry sp;
        if (!pDbSpInfo->getSP(propertyId, sp)) {
            continue;
        }

        const uint256& creationHash = sp.txid;

        CTransaction tx;
        uint256 hashBlock;
        if (!GetTransaction(creationHash, tx, Params().GetConsensus(), hashBlock, true)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
        }

        int64_t startTime = -1;
        if (!hashBlock.IsNull() && GetBlockIndex(hashBlock)) {
            startTime = GetBlockIndex(hashBlock)->nTime;
        }

        UniValue responseObj(UniValue::VOBJ);
        responseObj.push_back(Pair("propertyid", (uint64_t) propertyId));
        responseObj.push_back(Pair("name", sp.name));
        responseObj.push_back(Pair("issuer", sp.issuer));
        responseObj.push_back(Pair("propertyiddesired", (uint64_t) sp.property_desired));
        responseObj.push_back(Pair("tokensperunit", FormatMP(propertyId, sp.num_tokens)));
        responseObj.push_back(Pair("earlybonus", sp.early_bird));
        responseObj.push_back(Pair("percenttoissuer", sp.percentage));
        responseObj.push_back(Pair("starttime", startTime));
        responseObj.push_back(Pair("deadline", sp.deadline));
        response.push_back(responseObj);
    }

    return response;
}

UniValue zus_getgrants(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getgrants propertyid\n"
            "\nReturns information about granted and revoked units of managed tokens.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the managed tokens to lookup\n"
            "\nResult:\n"
            "{\n"
            "  \"propertyid\" : n,               (number) the identifier of the managed tokens\n"
            "  \"name\" : \"name\",                (string) the name of the tokens\n"
            "  \"issuer\" : \"address\",           (string) the Zurcoin address of the issuer on record\n"
            "  \"creationtxid\" : \"hash\",        (string) the hex-encoded creation transaction hash\n"
            "  \"totaltokens\" : \"n.nnnnnnnn\",   (string) the total number of tokens in existence\n"
            "  \"issuances\": [                  (array of JSON objects) a list of the granted and revoked tokens\n"
            "    {\n"
            "      \"txid\" : \"hash\",                (string) the hash of the transaction that granted tokens\n"
            "      \"grant\" : \"n.nnnnnnnn\"          (string) the number of tokens granted by this transaction\n"
            "    },\n"
            "    {\n"
            "      \"txid\" : \"hash\",                (string) the hash of the transaction that revoked tokens\n"
            "      \"grant\" : \"n.nnnnnnnn\"          (string) the number of tokens revoked by this transaction\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getgrants", "31")
            + HelpExampleRpc("zus_getgrants", "31")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (false == pDbSpInfo->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }
    UniValue response(UniValue::VOBJ);
    const uint256& creationHash = sp.txid;
    int64_t totalTokens = getTotalTokens(propertyId);

    // TODO: sort by height?

    UniValue issuancetxs(UniValue::VARR);
    std::map<uint256, std::vector<int64_t> >::const_iterator it;
    for (it = sp.historicalData.begin(); it != sp.historicalData.end(); it++) {
        const std::string& txid = it->first.GetHex();
        int64_t grantedTokens = it->second.at(0);
        int64_t revokedTokens = it->second.at(1);

        if (grantedTokens > 0) {
            UniValue granttx(UniValue::VOBJ);
            granttx.push_back(Pair("txid", txid));
            granttx.push_back(Pair("grant", FormatMP(propertyId, grantedTokens)));
            issuancetxs.push_back(granttx);
        }

        if (revokedTokens > 0) {
            UniValue revoketx(UniValue::VOBJ);
            revoketx.push_back(Pair("txid", txid));
            revoketx.push_back(Pair("revoke", FormatMP(propertyId, revokedTokens)));
            issuancetxs.push_back(revoketx);
        }
    }

    response.push_back(Pair("propertyid", (uint64_t) propertyId));
    response.push_back(Pair("name", sp.name));
    response.push_back(Pair("issuer", sp.issuer));
    response.push_back(Pair("creationtxid", creationHash.GetHex()));
    response.push_back(Pair("totaltokens", FormatMP(propertyId, totalTokens)));
    response.push_back(Pair("issuances", issuancetxs));

    return response;
}

UniValue zus_getorderbook(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "zus_getorderbook propertyid ( propertyid )\n"
            "\nList active offers on the distributed token exchange.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) filter orders by property identifier for sale\n"
            "2. propertyid           (number, optional) filter orders by property identifier desired\n"
            "\nResult:\n"
            "[                                              (array of JSON objects)\n"
            "  {\n"
            "    \"address\" : \"address\",                         (string) the Zurcoin address of the trader\n"
            "    \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
            "    \"ecosystem\" : \"main\"|\"test\",                   (string) the ecosytem in which the order was made (if \"cancel-ecosystem\")\n"
            "    \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
            "    \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
            "    \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
            "    \"amountremaining\" : \"n.nnnnnnnn\",              (string) the amount of tokens still up for sale\n"
            "    \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
            "    \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
            "    \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
            "    \"amounttofill\" : \"n.nnnnnnnn\",                 (string) the amount of tokens still needed to fill the offer completely\n"
            "    \"action\" : n,                                  (number) the action of the transaction: (1) \"trade\", (2) \"cancel-price\", (3) \"cancel-pair\", (4) \"cancel-ecosystem\"\n"
            "    \"block\" : nnnnnn,                              (number) the index of the block that contains the transaction\n"
            "    \"blocktime\" : nnnnnnnnnn                       (number) the timestamp of the block that contains the transaction\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getorderbook", "2")
            + HelpExampleRpc("zus_getorderbook", "2")
        );

    bool filterDesired = (params.size() > 1);
    uint32_t propertyIdForSale = ParsePropertyId(params[0]);
    uint32_t propertyIdDesired = 0;

    RequireExistingProperty(propertyIdForSale);

    if (filterDesired) {
        propertyIdDesired = ParsePropertyId(params[1]);

        RequireExistingProperty(propertyIdDesired);
        RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
        RequireDifferentIds(propertyIdForSale, propertyIdDesired);
    }

    std::vector<CMPMetaDEx> vecMetaDexObjects;
    {
        LOCK(cs_tally);
        for (md_PropertiesMap::const_iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
            const md_PricesMap& prices = my_it->second;
            for (md_PricesMap::const_iterator it = prices.begin(); it != prices.end(); ++it) {
                const md_Set& indexes = it->second;
                for (md_Set::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
                    const CMPMetaDEx& obj = *it;
                    if (obj.getProperty() != propertyIdForSale) continue;
                    if (!filterDesired || obj.getDesProperty() == propertyIdDesired) vecMetaDexObjects.push_back(obj);
                }
            }
        }
    }

    UniValue response(UniValue::VARR);
    MetaDexObjectsToJSON(vecMetaDexObjects, response);
    return response;
}

UniValue zus_gettradehistoryforaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "zus_gettradehistoryforaddress \"address\" ( count propertyid )\n"
            "\nRetrieves the history of orders on the distributed exchange for the supplied address.\n"
            "\nArguments:\n"
            "1. address              (string, required) address to retrieve history for\n"
            "2. count                (number, optional) number of orders to retrieve (default: 10)\n"
            "3. propertyid           (number, optional) filter by property identifier transacted (default: no filter)\n"
            "\nResult:\n"
            "[                                              (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
            "    \"sendingaddress\" : \"address\",                  (string) the Zurcoin address of the trader\n"
            "    \"ismine\" : true|false,                         (boolean) whether the order involes an address in the wallet\n"
            "    \"confirmations\" : nnnnnnnnnn,                  (number) the number of transaction confirmations\n"
            "    \"fee\" : \"n.nnnnnnnn\",                          (string) the transaction fee in zurcoins\n"
            "    \"blocktime\" : nnnnnnnnnn,                      (number) the timestamp of the block that contains the transaction\n"
            "    \"valid\" : true|false,                          (boolean) whether the transaction is valid\n"
            "    \"version\" : n,                                 (number) the transaction version\n"
            "    \"type_int\" : n,                                (number) the transaction type as number\n"
            "    \"type\" : \"type\",                               (string) the transaction type as string\n"
            "    \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
            "    \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
            "    \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
            "    \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
            "    \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
            "    \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
            "    \"unitprice\" : \"n.nnnnnnnnnnn...\"               (string) the unit price (shown in the property desired)\n"
            "    \"status\" : \"status\"                            (string) the status of the order (\"open\", \"cancelled\", \"filled\", ...)\n"
            "    \"canceltxid\" : \"hash\",                         (string) the hash of the transaction that cancelled the order (if cancelled)\n"
            "    \"matches\": [                                   (array of JSON objects) a list of matched orders and executed trades\n"
            "      {\n"
            "        \"txid\" : \"hash\",                               (string) the hash of the transaction that was matched against\n"
            "        \"block\" : nnnnnn,                              (number) the index of the block that contains this transaction\n"
            "        \"address\" : \"address\",                         (string) the Zurcoin address of the other trader\n"
            "        \"amountsold\" : \"n.nnnnnnnn\",                   (string) the number of tokens sold in this trade\n"
            "        \"amountreceived\" : \"n.nnnnnnnn\"                (string) the number of tokens traded in exchange\n"
            "      },\n"
            "      ...\n"
            "    ]\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nNote:\n"
            "The documentation only covers the output for a trade, but there are also cancel transactions with different properties.\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_gettradehistoryforaddress", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
            + HelpExampleRpc("zus_gettradehistoryforaddress", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
        );

    std::string address = ParseAddress(params[0]);
    uint64_t count = (params.size() > 1) ? params[1].get_int64() : 10;
    uint32_t propertyId = 0;

    if (params.size() > 2) {
        propertyId = ParsePropertyId(params[2]);
        RequireExistingProperty(propertyId);
    }

    // Obtain a sorted vector of txids for the address trade history
    std::vector<uint256> vecTransactions;
    {
        LOCK(cs_tally);
        pDbTradeList->getTradesForAddress(address, vecTransactions, propertyId);
    }

    // Populate the address trade history into JSON objects until we have processed count transactions
    UniValue response(UniValue::VARR);
    uint32_t processed = 0;
    for(std::vector<uint256>::reverse_iterator it = vecTransactions.rbegin(); it != vecTransactions.rend(); ++it) {
        UniValue txobj(UniValue::VOBJ);
        int populateResult = populateRPCTransactionObject(*it, txobj, "", true);
        if (0 == populateResult) {
            response.push_back(txobj);
            processed++;
            if (processed >= count) break;
        }
    }

    return response;
}

UniValue zus_gettradehistoryforpair(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "zus_gettradehistoryforpair propertyid propertyid ( count )\n"
            "\nRetrieves the history of trades on the distributed token exchange for the specified market.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the first side of the traded pair\n"
            "2. propertyid           (number, required) the second side of the traded pair\n"
            "3. count                (number, optional) number of trades to retrieve (default: 10)\n"
            "\nResult:\n"
            "[                                      (array of JSON objects)\n"
            "  {\n"
            "    \"block\" : nnnnnn,                      (number) the index of the block that contains the trade match\n"
            "    \"unitprice\" : \"n.nnnnnnnnnnn...\" ,     (string) the unit price used to execute this trade (received/sold)\n"
            "    \"inverseprice\" : \"n.nnnnnnnnnnn...\",   (string) the inverse unit price (sold/received)\n"
            "    \"sellertxid\" : \"hash\",                 (string) the hash of the transaction of the seller\n"
            "    \"address\" : \"address\",                 (string) the Zurcoin address of the seller\n"
            "    \"amountsold\" : \"n.nnnnnnnn\",           (string) the number of tokens sold in this trade\n"
            "    \"amountreceived\" : \"n.nnnnnnnn\",       (string) the number of tokens traded in exchange\n"
            "    \"matchingtxid\" : \"hash\",               (string) the hash of the transaction that was matched against\n"
            "    \"matchingaddress\" : \"address\"          (string) the Zurcoin address of the other party of this trade\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_gettradehistoryforpair", "1 12 500")
            + HelpExampleRpc("zus_gettradehistoryforpair", "1, 12, 500")
        );

    // obtain property identifiers for pair & check valid parameters
    uint32_t propertyIdSideA = ParsePropertyId(params[0]);
    uint32_t propertyIdSideB = ParsePropertyId(params[1]);
    uint64_t count = (params.size() > 2) ? params[2].get_int64() : 10;

    RequireExistingProperty(propertyIdSideA);
    RequireExistingProperty(propertyIdSideB);
    RequireSameEcosystem(propertyIdSideA, propertyIdSideB);
    RequireDifferentIds(propertyIdSideA, propertyIdSideB);

    // request pair trade history from trade db
    UniValue response(UniValue::VARR);
    LOCK(cs_tally);
    pDbTradeList->getTradesForPair(propertyIdSideA, propertyIdSideB, response, count);
    return response;
}

UniValue zus_getactivedexsells(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "zus_getactivedexsells ( address )\n"
            "\nReturns currently active offers on the distributed exchange.\n"
            "\nArguments:\n"
            "1. address              (string, optional) address filter (default: include any)\n"
            "\nResult:\n"
            "[                                   (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                    (string) the hash of the transaction of this offer\n"
            "    \"propertyid\" : n,                   (number) the identifier of the tokens for sale\n"
            "    \"seller\" : \"address\",               (string) the Zurcoin address of the seller\n"
            "    \"amountavailable\" : \"n.nnnnnnnn\",   (string) the number of tokens still listed for sale and currently available\n"
            "    \"zurcoindesired\" : \"n.nnnnnnnn\",    (string) the number of zurcoins desired in exchange\n"
            "    \"unitprice\" : \"n.nnnnnnnn\" ,        (string) the unit price (ZUR/token)\n"
            "    \"timelimit\" : nn,                   (number) the time limit in blocks a buyer has to pay following a successful accept\n"
            "    \"minimumfee\" : \"n.nnnnnnnn\",        (string) the minimum mining fee a buyer has to pay to accept this offer\n"
            "    \"amountaccepted\" : \"n.nnnnnnnn\",    (string) the number of tokens currently reserved for pending \"accept\" orders\n"
            "    \"accepts\": [                        (array of JSON objects) a list of pending \"accept\" orders\n"
            "      {\n"
            "        \"buyer\" : \"address\",                (string) the Zurcoin address of the buyer\n"
            "        \"block\" : nnnnnn,                   (number) the index of the block that contains the \"accept\" order\n"
            "        \"blocksleft\" : nn,                  (number) the number of blocks left to pay\n"
            "        \"amount\" : \"n.nnnnnnnn\"             (string) the amount of tokens accepted and reserved\n"
            "        \"amounttopay\" : \"n.nnnnnnnn\"        (string) the amount in zurcoins needed finalize the trade\n"
            "      },\n"
            "      ...\n"
            "    ]\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getactivedexsells", "")
            + HelpExampleRpc("zus_getactivedexsells", "")
        );

    std::string addressFilter;

    if (params.size() > 0) {
        addressFilter = ParseAddressOrEmpty(params[0]);
    }

    UniValue response(UniValue::VARR);

    int curBlock = GetHeight();

    LOCK(cs_tally);

    for (OfferMap::iterator it = my_offers.begin(); it != my_offers.end(); ++it) {
        const CMPOffer& selloffer = it->second;
        const std::string& sellCombo = it->first;
        std::string seller = sellCombo.substr(0, sellCombo.size() - 2);

        // filtering
        if (!addressFilter.empty() && seller != addressFilter) continue;

        std::string txid = selloffer.getHash().GetHex();
        uint32_t propertyId = selloffer.getProperty();
        int64_t minFee = selloffer.getMinFee();
        uint8_t timeLimit = selloffer.getBlockTimeLimit();
        int64_t sellOfferAmount = selloffer.getOfferAmountOriginal(); //badly named - "Original" implies off the wire, but is amended amount
        int64_t sellBitcoinDesired = selloffer.getZURDesiredOriginal(); //badly named - "Original" implies off the wire, but is amended amount
        int64_t amountAvailable = GetTokenBalance(seller, propertyId, SELLOFFER_RESERVE);
        int64_t amountAccepted = GetTokenBalance(seller, propertyId, ACCEPT_RESERVE);

        // TODO: no math, and especially no rounding here (!)
        // TODO: no math, and especially no rounding here (!)
        // TODO: no math, and especially no rounding here (!)

        // calculate unit price and updated amount of zurcoin desired
        double unitPriceFloat = 0.0;
        if ((sellOfferAmount > 0) && (sellBitcoinDesired > 0)) {
            unitPriceFloat = (double) sellBitcoinDesired / (double) sellOfferAmount; // divide by zero protection
        }
        int64_t unitPrice = rounduint64(unitPriceFloat * COIN);
        int64_t zurcoinDesired = calculateDesiredZUR(sellOfferAmount, sellBitcoinDesired, amountAvailable);

        UniValue responseObj(UniValue::VOBJ);
        responseObj.push_back(Pair("txid", txid));
        responseObj.push_back(Pair("propertyid", (uint64_t) propertyId));
        responseObj.push_back(Pair("seller", seller));
        responseObj.push_back(Pair("amountavailable", FormatDivisibleMP(amountAvailable)));
        responseObj.push_back(Pair("zurcoindesired", FormatDivisibleMP(zurcoinDesired)));
        responseObj.push_back(Pair("unitprice", FormatDivisibleMP(unitPrice)));
        responseObj.push_back(Pair("timelimit", timeLimit));
        responseObj.push_back(Pair("minimumfee", FormatDivisibleMP(minFee)));

        // display info about accepts related to sell
        responseObj.push_back(Pair("amountaccepted", FormatDivisibleMP(amountAccepted)));
        UniValue acceptsMatched(UniValue::VARR);
        for (AcceptMap::const_iterator ait = my_accepts.begin(); ait != my_accepts.end(); ++ait) {
            UniValue matchedAccept(UniValue::VOBJ);
            const CMPAccept& accept = ait->second;
            const std::string& acceptCombo = ait->first;

            // does this accept match the sell?
            if (accept.getHash() == selloffer.getHash()) {
                // split acceptCombo out to get the buyer address
                std::string buyer = acceptCombo.substr((acceptCombo.find("+") + 1), (acceptCombo.size()-(acceptCombo.find("+") + 1)));
                int blockOfAccept = accept.getAcceptBlock();
                int blocksLeftToPay = (blockOfAccept + selloffer.getBlockTimeLimit()) - curBlock;
                int64_t amountAccepted = accept.getAcceptAmountRemaining();
                // TODO: don't recalculate!
                int64_t amountToPayInZUR = calculateDesiredZUR(accept.getOfferAmountOriginal(), accept.getZURDesiredOriginal(), amountAccepted);
                matchedAccept.push_back(Pair("buyer", buyer));
                matchedAccept.push_back(Pair("block", blockOfAccept));
                matchedAccept.push_back(Pair("blocksleft", blocksLeftToPay));
                matchedAccept.push_back(Pair("amount", FormatDivisibleMP(amountAccepted)));
                matchedAccept.push_back(Pair("amounttopay", FormatDivisibleMP(amountToPayInZUR)));
                acceptsMatched.push_back(matchedAccept);
            }
        }
        responseObj.push_back(Pair("accepts", acceptsMatched));

        // add sell object into response array
        response.push_back(responseObj);
    }

    return response;
}

UniValue zus_listblocktransactions(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_listblocktransactions index\n"
            "\nLists all Zus transactions in a block.\n"
            "\nArguments:\n"
            "1. index                (number, required) the block height or block index\n"
            "\nResult:\n"
            "[                       (array of string)\n"
            "  \"hash\",                 (string) the hash of the transaction\n"
            "  ...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_listblocktransactions", "279007")
            + HelpExampleRpc("zus_listblocktransactions", "279007")
        );

    int blockHeight = params[0].get_int();

    RequireHeightInChain(blockHeight);

    // next let's obtain the block for this height
    CBlock block;
    {
        LOCK(cs_main);
        CBlockIndex* pBlockIndex = chainActive[blockHeight];

        if (!ReadBlockFromDisk(block, pBlockIndex, Params().GetConsensus())) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to read block from disk");
        }
    }

    UniValue response(UniValue::VARR);

    // now we want to loop through each of the transactions in the block and run against CMPTxList::exists
    // those that return positive add to our response array

    LOCK(cs_tally);

    BOOST_FOREACH(const CTransaction&tx, block.vtx) {
        if (pDbTransactionList->exists(tx.GetHash())) {
            // later we can add a verbose flag to decode here, but for now callers can send returned txids into gettransaction_MP
            // add the txid into the response as it's an MP transaction
            response.push_back(tx.GetHash().GetHex());
        }
    }

    return response;
}

UniValue zus_listblockstransactions(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "zus_listblocktransactions firstblock lastblock\n"
            "\nLists all Zus transactions in a given range of blocks.\n"
            "\nNote: the list of transactions is unordered and can contain invalid transactions!\n"
            "\nArguments:\n"
            "1. firstblock           (number, required) the index of the first block to consider\n"
            "2. lastblock            (number, required) the index of the last block to consider\n"
            "\nResult:\n"
            "[                       (array of string)\n"
            "  \"hash\",                 (string) the hash of the transaction\n"
            "  ...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_listblocktransactions", "279007 300000")
            + HelpExampleRpc("zus_listblocktransactions", "279007, 300000")
        );

    int blockFirst = params[0].get_int();
    int blockLast = params[1].get_int();

    std::set<uint256> txs;
    UniValue response(UniValue::VARR);

    LOCK(cs_tally);
    {
        pDbTransactionList->GetOmniTxsInBlockRange(blockFirst, blockLast, txs);
    }

    BOOST_FOREACH(const uint256& tx, txs) {
        response.push_back(tx.GetHex());
    }

    return response;
}

UniValue zus_gettransaction(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_gettransaction \"txid\"\n"
            "\nGet detailed information about an Zus transaction.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the transaction to lookup\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
            "  \"sendingaddress\" : \"address\",     (string) the Zurcoin address of the sender\n"
            "  \"referenceaddress\" : \"address\",   (string) a Zurcoin address used as reference (if any)\n"
            "  \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
            "  \"confirmations\" : nnnnnnnnnn,     (number) the number of transaction confirmations\n"
            "  \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in zurcoins\n"
            "  \"blocktime\" : nnnnnnnnnn,         (number) the timestamp of the block that contains the transaction\n"
            "  \"valid\" : true|false,             (boolean) whether the transaction is valid\n"
            "  \"invalidreason\" : \"reason\",     (string) if a transaction is invalid, the reason \n"
            "  \"version\" : n,                    (number) the transaction version\n"
            "  \"type_int\" : n,                   (number) the transaction type as number\n"
            "  \"type\" : \"type\",                  (string) the transaction type as string\n"
            "  [...]                             (mixed) other transaction type specific properties\n"
            "}\n"
            "\nbExamples:\n"
            + HelpExampleCli("zus_gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("zus_gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 hash = ParseHashV(params[0], "txid");

    UniValue txobj(UniValue::VOBJ);
    int populateResult = populateRPCTransactionObject(hash, txobj);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

UniValue zus_listtransactions(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 5)
        throw runtime_error(
            "zus_listtransactions ( \"address\" count skip startblock endblock )\n"
            "\nList wallet transactions, optionally filtered by an address and block boundaries.\n"
            "\nArguments:\n"
            "1. address              (string, optional) address filter (default: \"*\")\n"
            "2. count                (number, optional) show at most n transactions (default: 10)\n"
            "3. skip                 (number, optional) skip the first n transactions (default: 0)\n"
            "4. startblock           (number, optional) first block to begin the search (default: 0)\n"
            "5. endblock             (number, optional) last block to include in the search (default: 999999999)\n"
            "\nResult:\n"
            "[                                 (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
            "    \"sendingaddress\" : \"address\",     (string) the Zurcoin address of the sender\n"
            "    \"referenceaddress\" : \"address\",   (string) a Zurcoin address used as reference (if any)\n"
            "    \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
            "    \"confirmations\" : nnnnnnnnnn,     (number) the number of transaction confirmations\n"
            "    \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in zurcoins\n"
            "    \"blocktime\" : nnnnnnnnnn,         (number) the timestamp of the block that contains the transaction\n"
            "    \"valid\" : true|false,             (boolean) whether the transaction is valid\n"
            "    \"version\" : n,                    (number) the transaction version\n"
            "    \"type_int\" : n,                   (number) the transaction type as number\n"
            "    \"type\" : \"type\",                  (string) the transaction type as string\n"
            "    [...]                             (mixed) other transaction type specific properties\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_listtransactions", "")
            + HelpExampleRpc("zus_listtransactions", "")
        );

    // obtains parameters - default all wallet addresses & last 10 transactions
    std::string addressParam;
    if (params.size() > 0) {
        if (("*" != params[0].get_str()) && ("" != params[0].get_str())) addressParam = params[0].get_str();
    }
    int64_t nCount = 10;
    if (params.size() > 1) nCount = params[1].get_int64();
    if (nCount < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    int64_t nFrom = 0;
    if (params.size() > 2) nFrom = params[2].get_int64();
    if (nFrom < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");
    int64_t nStartBlock = 0;
    if (params.size() > 3) nStartBlock = params[3].get_int64();
    if (nStartBlock < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative start block");
    int64_t nEndBlock = 999999999;
    if (params.size() > 4) nEndBlock = params[4].get_int64();
    if (nEndBlock < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative end block");

    // obtain a sorted list of Zus layer wallet transactions (including STO receipts and pending)
    std::map<std::string,uint256> walletTransactions = FetchWalletOmniTransactions(nFrom+nCount, nStartBlock, nEndBlock);

    // reverse iterate over (now ordered) transactions and populate RPC objects for each one
    UniValue response(UniValue::VARR);
    for (std::map<std::string,uint256>::reverse_iterator it = walletTransactions.rbegin(); it != walletTransactions.rend(); it++) {
        if (nFrom <= 0 && nCount > 0) {
            uint256 txHash = it->second;
            UniValue txobj(UniValue::VOBJ);
            int populateResult = populateRPCTransactionObject(txHash, txobj, addressParam);
            if (0 == populateResult) {
                response.push_back(txobj);
                nCount--;
            }
        }
        nFrom--;
    }

    return response;
}

UniValue zus_listpendingtransactions(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "zus_listpendingtransactions ( \"address\" )\n"
            "\nReturns a list of unconfirmed Zus transactions, pending in the memory pool.\n"
            "\nAn optional filter can be provided to only include transactions which involve the given address.\n"
            "\nNote: the validity of pending transactions is uncertain, and the state of the memory pool may "
            "change at any moment. It is recommended to check transactions after confirmation, and pending "
            "transactions should be considered as invalid.\n"
            "\nArguments:\n"
            "1. address              (string, optional) address filter (default: \"\" for no filter)\n"
            "\nResult:\n"
            "[                                 (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
            "    \"sendingaddress\" : \"address\",     (string) the Zurcoin address of the sender\n"
            "    \"referenceaddress\" : \"address\",   (string) a Zurcoin address used as reference (if any)\n"
            "    \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
            "    \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in zurcoins\n"
            "    \"version\" : n,                    (number) the transaction version\n"
            "    \"type_int\" : n,                   (number) the transaction type as number\n"
            "    \"type\" : \"type\",                  (string) the transaction type as string\n"
            "    [...]                             (mixed) other transaction type specific properties\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_listpendingtransactions", "")
            + HelpExampleRpc("zus_listpendingtransactions", "")
        );

    std::string filterAddress;
    if (params.size() > 0) {
        filterAddress = ParseAddressOrEmpty(params[0]);
    }

    std::vector<uint256> vTxid;
    mempool.queryHashes(vTxid);

    UniValue result(UniValue::VARR);
    BOOST_FOREACH(const uint256& hash, vTxid) {
        if (!IsInMarkerCache(hash)) {
            continue;
        }

        UniValue txObj(UniValue::VOBJ);
        if (populateRPCTransactionObject(hash, txObj, filterAddress) == 0) {
            result.push_back(txObj);
        }
    }

    return result;
}

UniValue zus_getinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "zus_getinfo\n"
            "Returns various state information of the client and protocol.\n"
            "\nResult:\n"
            "{\n"
            "  \"zurbankversion_int\" : xxxxxxx,       (number) client version as integer\n"
            "  \"zurbankversion\" : \"x.x.x.x-xxx\",     (string) client version\n"
            "  \"mastercoreversion\" : \"x.x.x.x-xxx\",   (string) client version (DEPRECIATED)\n"
            "  \"zurcoincoreversion\" : \"x.x.x\",        (string) Zurcoin Core version\n"
            "  \"block\" : nnnnnn,                      (number) index of the last processed block\n"
            "  \"blocktime\" : nnnnnnnnnn,              (number) timestamp of the last processed block\n"
            "  \"blocktransactions\" : nnnn,            (number) Zus transactions found in the last processed block\n"
            "  \"totaltransactions\" : nnnnnnnn,        (number) Zus transactions processed in total\n"
            "  \"alerts\" : [                           (array of JSON objects) active protocol alert (if any)\n"
            "    {\n"
            "      \"alerttypeint\" : n,                    (number) alert type as integer\n"
            "      \"alerttype\" : \"xxx\",                   (string) alert type\n"
            "      \"alertexpiry\" : \"nnnnnnnnnn\",          (string) expiration criteria\n"
            "      \"alertmessage\" : \"xxx\"                 (string) information about the alert\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getinfo", "")
            + HelpExampleRpc("zus_getinfo", "")
        );

    UniValue infoResponse(UniValue::VOBJ);

    // provide the mastercore and zurcoin version
    infoResponse.push_back(Pair("zurbankversion_int", ZURBANK_VERSION));
    infoResponse.push_back(Pair("zurbankversion", ZurBankVersion()));
    infoResponse.push_back(Pair("mastercoreversion", ZurBankVersion()));
    infoResponse.push_back(Pair("zurcoincoreversion", BitcoinCoreVersion()));

    // provide the current block details
    int block = GetHeight();
    int64_t blockTime = GetLatestBlockTime();

    LOCK(cs_tally);

    int blockMPTransactions = pDbTransactionList->getMPTransactionCountBlock(block);
    int totalMPTransactions = pDbTransactionList->getMPTransactionCountTotal();
    int totalMPTrades = pDbTradeList->getMPTradeCountTotal();
    infoResponse.push_back(Pair("block", block));
    infoResponse.push_back(Pair("blocktime", blockTime));
    infoResponse.push_back(Pair("blocktransactions", blockMPTransactions));

    // provide the number of trades completed
    infoResponse.push_back(Pair("totaltrades", totalMPTrades));
    // provide the number of transactions parsed
    infoResponse.push_back(Pair("totaltransactions", totalMPTransactions));

    // handle alerts
    UniValue alerts(UniValue::VARR);
    std::vector<AlertData> omniAlerts = GetZurBankAlerts();
    for (std::vector<AlertData>::iterator it = omniAlerts.begin(); it != omniAlerts.end(); it++) {
        AlertData alert = *it;
        UniValue alertResponse(UniValue::VOBJ);
        std::string alertTypeStr;
        switch (alert.alert_type) {
            case 1: alertTypeStr = "alertexpiringbyblock";
            break;
            case 2: alertTypeStr = "alertexpiringbyblocktime";
            break;
            case 3: alertTypeStr = "alertexpiringbyclientversion";
            break;
            default: alertTypeStr = "error";
        }
        alertResponse.push_back(Pair("alerttypeint", alert.alert_type));
        alertResponse.push_back(Pair("alerttype", alertTypeStr));
        alertResponse.push_back(Pair("alertexpiry", FormatIndivisibleMP(alert.alert_expiry)));
        alertResponse.push_back(Pair("alertmessage", alert.alert_message));
        alerts.push_back(alertResponse);
    }
    infoResponse.push_back(Pair("alerts", alerts));

    return infoResponse;
}

UniValue zus_getactivations(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "zus_getactivations\n"
            "Returns pending and completed feature activations.\n"
            "\nResult:\n"
            "{\n"
            "  \"pendingactivations\": [       (array of JSON objects) a list of pending feature activations\n"
            "    {\n"
            "      \"featureid\" : n,              (number) the id of the feature\n"
            "      \"featurename\" : \"xxxxxxxx\",   (string) the name of the feature\n"
            "      \"activationblock\" : n,        (number) the block the feature will be activated\n"
            "      \"minimumversion\" : n          (number) the minimum client version needed to support this feature\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "  \"completedactivations\": [     (array of JSON objects) a list of completed feature activations\n"
            "    {\n"
            "      \"featureid\" : n,              (number) the id of the feature\n"
            "      \"featurename\" : \"xxxxxxxx\",   (string) the name of the feature\n"
            "      \"activationblock\" : n,        (number) the block the feature will be activated\n"
            "      \"minimumversion\" : n          (number) the minimum client version needed to support this feature\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getactivations", "")
            + HelpExampleRpc("zus_getactivations", "")
        );

    UniValue response(UniValue::VOBJ);

    UniValue arrayPendingActivations(UniValue::VARR);
    std::vector<FeatureActivation> vecPendingActivations = GetPendingActivations();
    for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ++it) {
        UniValue actObj(UniValue::VOBJ);
        FeatureActivation pendingAct = *it;
        actObj.push_back(Pair("featureid", pendingAct.featureId));
        actObj.push_back(Pair("featurename", pendingAct.featureName));
        actObj.push_back(Pair("activationblock", pendingAct.activationBlock));
        actObj.push_back(Pair("minimumversion", (uint64_t)pendingAct.minClientVersion));
        arrayPendingActivations.push_back(actObj);
    }

    UniValue arrayCompletedActivations(UniValue::VARR);
    std::vector<FeatureActivation> vecCompletedActivations = GetCompletedActivations();
    for (std::vector<FeatureActivation>::iterator it = vecCompletedActivations.begin(); it != vecCompletedActivations.end(); ++it) {
        UniValue actObj(UniValue::VOBJ);
        FeatureActivation completedAct = *it;
        actObj.push_back(Pair("featureid", completedAct.featureId));
        actObj.push_back(Pair("featurename", completedAct.featureName));
        actObj.push_back(Pair("activationblock", completedAct.activationBlock));
        actObj.push_back(Pair("minimumversion", (uint64_t)completedAct.minClientVersion));
        arrayCompletedActivations.push_back(actObj);
    }

    response.push_back(Pair("pendingactivations", arrayPendingActivations));
    response.push_back(Pair("completedactivations", arrayCompletedActivations));

    return response;
}

UniValue zus_getsto(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "zus_getsto \"txid\" \"recipientfilter\"\n"
            "\nGet information and recipients of a send-to-owners transaction.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the transaction to lookup\n"
            "2. recipientfilter      (string, optional) a filter for recipients (wallet by default, \"*\" for all)\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"hash\",                (string) the hex-encoded hash of the transaction\n"
            "  \"sendingaddress\" : \"address\",   (string) the Zurcoin address of the sender\n"
            "  \"ismine\" : true|false,          (boolean) whether the transaction involes an address in the wallet\n"
            "  \"confirmations\" : nnnnnnnnnn,   (number) the number of transaction confirmations\n"
            "  \"fee\" : \"n.nnnnnnnn\",           (string) the transaction fee in zurcoins\n"
            "  \"blocktime\" : nnnnnnnnnn,       (number) the timestamp of the block that contains the transaction\n"
            "  \"valid\" : true|false,           (boolean) whether the transaction is valid\n"
            "  \"version\" : n,                  (number) the transaction version\n"
            "  \"type_int\" : n,                 (number) the transaction type as number\n"
            "  \"type\" : \"type\",                (string) the transaction type as string\n"
            "  \"propertyid\" : n,               (number) the identifier of sent tokens\n"
            "  \"divisible\" : true|false,       (boolean) whether the sent tokens are divisible\n"
            "  \"amount\" : \"n.nnnnnnnn\",        (string) the number of tokens sent to owners\n"
            "  \"totalstofee\" : \"n.nnnnnnnn\",   (string) the fee paid by the sender, nominated in ZUS or TZUS\n"
            "  \"recipients\": [                 (array of JSON objects) a list of recipients\n"
            "    {\n"
            "      \"address\" : \"address\",          (string) the Zurcoin address of the recipient\n"
            "      \"amount\" : \"n.nnnnnnnn\"         (string) the number of tokens sent to this recipient\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_getsto", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\" \"*\"")
            + HelpExampleRpc("zus_getsto", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"*\"")
        );

    uint256 hash = ParseHashV(params[0], "txid");
    std::string filterAddress;
    if (params.size() > 1) filterAddress = ParseAddressOrWildcard(params[1]);

    UniValue txobj(UniValue::VOBJ);
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true, filterAddress);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

UniValue zus_gettrade(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_gettrade \"txid\"\n"
            "\nGet detailed information and trade matches for orders on the distributed token exchange.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the order to lookup\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
            "  \"sendingaddress\" : \"address\",                  (string) the Zurcoin address of the trader\n"
            "  \"ismine\" : true|false,                         (boolean) whether the order involes an address in the wallet\n"
            "  \"confirmations\" : nnnnnnnnnn,                  (number) the number of transaction confirmations\n"
            "  \"fee\" : \"n.nnnnnnnn\",                          (string) the transaction fee in zurcoins\n"
            "  \"blocktime\" : nnnnnnnnnn,                      (number) the timestamp of the block that contains the transaction\n"
            "  \"valid\" : true|false,                          (boolean) whether the transaction is valid\n"
            "  \"version\" : n,                                 (number) the transaction version\n"
            "  \"type_int\" : n,                                (number) the transaction type as number\n"
            "  \"type\" : \"type\",                               (string) the transaction type as string\n"
            "  \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
            "  \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
            "  \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
            "  \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
            "  \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
            "  \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
            "  \"unitprice\" : \"n.nnnnnnnnnnn...\"               (string) the unit price (shown in the property desired)\n"
            "  \"status\" : \"status\"                            (string) the status of the order (\"open\", \"cancelled\", \"filled\", ...)\n"
            "  \"canceltxid\" : \"hash\",                         (string) the hash of the transaction that cancelled the order (if cancelled)\n"
            "  \"matches\": [                                   (array of JSON objects) a list of matched orders and executed trades\n"
            "    {\n"
            "      \"txid\" : \"hash\",                               (string) the hash of the transaction that was matched against\n"
            "      \"block\" : nnnnnn,                              (number) the index of the block that contains this transaction\n"
            "      \"address\" : \"address\",                         (string) the Zurcoin address of the other trader\n"
            "      \"amountsold\" : \"n.nnnnnnnn\",                   (string) the number of tokens sold in this trade\n"
            "      \"amountreceived\" : \"n.nnnnnnnn\"                (string) the number of tokens traded in exchange\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nNote:\n"
            "The documentation only covers the output for a trade, but there are also cancel transactions with different properties.\n"
            "\nExamples:\n"
            + HelpExampleCli("zus_gettrade", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("zus_gettrade", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 hash = ParseHashV(params[0], "txid");

    UniValue txobj(UniValue::VOBJ);
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

UniValue zus_getcurrentconsensushash(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "zus_getcurrentconsensushash\n"
            "\nReturns the consensus hash for all balances for the current block.\n"
            "\nResult:\n"
            "{\n"
            "  \"block\" : nnnnnn,          (number) the index of the block this consensus hash applies to\n"
            "  \"blockhash\" : \"hash\",      (string) the hash of the corresponding block\n"
            "  \"consensushash\" : \"hash\"   (string) the consensus hash for the block\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_getcurrentconsensushash", "")
            + HelpExampleRpc("zus_getcurrentconsensushash", "")
        );

    LOCK(cs_main); // TODO - will this ensure we don't take in a new block in the couple of ms it takes to calculate the consensus hash?

    int block = GetHeight();

    CBlockIndex* pblockindex = chainActive[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 consensusHash = GetConsensusHash();

    UniValue response(UniValue::VOBJ);
    response.push_back(Pair("block", block));
    response.push_back(Pair("blockhash", blockHash.GetHex()));
    response.push_back(Pair("consensushash", consensusHash.GetHex()));

    return response;
}

UniValue zus_getmetadexhash(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "zus_getmetadexhash propertyId\n"
            "\nReturns a hash of the current state of the MetaDEx (default) or orderbook.\n"
            "\nArguments:\n"
            "1. propertyid                  (number, optional) hash orderbook (only trades selling propertyid)\n"
            "\nResult:\n"
            "{\n"
            "  \"block\" : nnnnnn,          (number) the index of the block this hash applies to\n"
            "  \"blockhash\" : \"hash\",    (string) the hash of the corresponding block\n"
            "  \"propertyid\" : nnnnnn,     (number) the market this hash applies to (or 0 for all markets)\n"
            "  \"metadexhash\" : \"hash\"   (string) the hash for the state of the MetaDEx/orderbook\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_getmetadexhash", "3")
            + HelpExampleRpc("zus_getmetadexhash", "3")
        );

    LOCK(cs_main);

    uint32_t propertyId = 0;
    if (params.size() > 0) {
        propertyId = ParsePropertyId(params[0]);
        RequireExistingProperty(propertyId);
    }

    int block = GetHeight();
    CBlockIndex* pblockindex = chainActive[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 metadexHash = GetMetaDExHash(propertyId);

    UniValue response(UniValue::VOBJ);
    response.push_back(Pair("block", block));
    response.push_back(Pair("blockhash", blockHash.GetHex()));
    response.push_back(Pair("propertyid", (uint64_t)propertyId));
    response.push_back(Pair("metadexhash", metadexHash.GetHex()));

    return response;
}

UniValue zus_getbalanceshash(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_getbalanceshash propertyid\n"
            "\nReturns a hash of the balances for the property.\n"
            "\nArguments:\n"
            "1. propertyid                  (number, required) the property to hash balances for\n"
            "\nResult:\n"
            "{\n"
            "  \"block\" : nnnnnn,          (number) the index of the block this hash applies to\n"
            "  \"blockhash\" : \"hash\",    (string) the hash of the corresponding block\n"
            "  \"propertyid\" : nnnnnn,     (number) the property id of the hashed balances\n"
            "  \"balanceshash\" : \"hash\"  (string) the hash for the balances\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_getbalanceshash", "31")
            + HelpExampleRpc("zus_getbalanceshash", "31")
        );

    LOCK(cs_main);

    uint32_t propertyId = ParsePropertyId(params[0]);
    RequireExistingProperty(propertyId);

    int block = GetHeight();
    CBlockIndex* pblockindex = chainActive[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 balancesHash = GetBalancesHash(propertyId);

    UniValue response(UniValue::VOBJ);
    response.push_back(Pair("block", block));
    response.push_back(Pair("blockhash", blockHash.GetHex()));
    response.push_back(Pair("propertyid", (uint64_t)propertyId));
    response.push_back(Pair("balanceshash", balancesHash.GetHex()));

    return response;
}

static const CRPCCommand commands[] =
{ //  category                             name                            actor (function)               okSafeMode
  //  ------------------------------------ ------------------------------- ------------------------------ ----------
    { "omni layer (data retrieval)", "zus_getinfo",                   &zus_getinfo,                    true  },
    { "omni layer (data retrieval)", "zus_getactivations",            &zus_getactivations,             true  },
    { "omni layer (data retrieval)", "zus_getallbalancesforid",       &zus_getallbalancesforid,        false },
    { "omni layer (data retrieval)", "zus_getbalance",                &zus_getbalance,                 false },
    { "omni layer (data retrieval)", "zus_gettransaction",            &zus_gettransaction,             false },
    { "omni layer (data retrieval)", "zus_getproperty",               &zus_getproperty,                false },
    { "omni layer (data retrieval)", "zus_listproperties",            &zus_listproperties,             false },
    { "omni layer (data retrieval)", "zus_getcrowdsale",              &zus_getcrowdsale,               false },
    { "omni layer (data retrieval)", "zus_getgrants",                 &zus_getgrants,                  false },
    { "omni layer (data retrieval)", "zus_getactivedexsells",         &zus_getactivedexsells,          false },
    { "omni layer (data retrieval)", "zus_getactivecrowdsales",       &zus_getactivecrowdsales,        false },
    { "omni layer (data retrieval)", "zus_getorderbook",              &zus_getorderbook,               false },
    { "omni layer (data retrieval)", "zus_gettrade",                  &zus_gettrade,                   false },
    { "omni layer (data retrieval)", "zus_getsto",                    &zus_getsto,                     false },
    { "omni layer (data retrieval)", "zus_listblocktransactions",     &zus_listblocktransactions,      false },
    { "omni layer (data retrieval)", "zus_listblockstransactions",    &zus_listblockstransactions,     false },
    { "omni layer (data retrieval)", "zus_listpendingtransactions",   &zus_listpendingtransactions,    false },
    { "omni layer (data retrieval)", "zus_getallbalancesforaddress",  &zus_getallbalancesforaddress,   false },
    { "omni layer (data retrieval)", "zus_gettradehistoryforaddress", &zus_gettradehistoryforaddress,  false },
    { "omni layer (data retrieval)", "zus_gettradehistoryforpair",    &zus_gettradehistoryforpair,     false },
    { "omni layer (data retrieval)", "zus_getcurrentconsensushash",   &zus_getcurrentconsensushash,    false },
    { "omni layer (data retrieval)", "zus_getpayload",                &zus_getpayload,                 false },
    { "omni layer (data retrieval)", "zus_getseedblocks",             &zus_getseedblocks,              false },
    { "omni layer (data retrieval)", "zus_getmetadexhash",            &zus_getmetadexhash,             false },
    { "omni layer (data retrieval)", "zus_getfeecache",               &zus_getfeecache,                false },
    { "omni layer (data retrieval)", "zus_getfeetrigger",             &zus_getfeetrigger,              false },
    { "omni layer (data retrieval)", "zus_getfeedistribution",        &zus_getfeedistribution,         false },
    { "omni layer (data retrieval)", "zus_getfeedistributions",       &zus_getfeedistributions,        false },
    { "omni layer (data retrieval)", "zus_getbalanceshash",           &zus_getbalanceshash,            false },
#ifdef ENABLE_WALLET
    { "omni layer (data retrieval)", "zus_listtransactions",          &zus_listtransactions,           false },
    { "omni layer (data retrieval)", "zus_getfeeshare",               &zus_getfeeshare,                false },
    { "omni layer (configuration)",  "zus_setautocommit",             &zus_setautocommit,              true  },
    { "omni layer (data retrieval)", "zus_getwalletbalances",         &zus_getwalletbalances,          false },
    { "omni layer (data retrieval)", "zus_getwalletaddressbalances",  &zus_getwalletaddressbalances,   false },
#endif
    { "hidden",                      "mscrpc",                         &mscrpc,                          true  },

    /* depreciated: */
    { "hidden",                      "getinfo_MP",                     &zus_getinfo,                    true  },
    { "hidden",                      "getbalance_MP",                  &zus_getbalance,                 false },
    { "hidden",                      "getallbalancesforaddress_MP",    &zus_getallbalancesforaddress,   false },
    { "hidden",                      "getallbalancesforid_MP",         &zus_getallbalancesforid,        false },
    { "hidden",                      "getproperty_MP",                 &zus_getproperty,                false },
    { "hidden",                      "listproperties_MP",              &zus_listproperties,             false },
    { "hidden",                      "getcrowdsale_MP",                &zus_getcrowdsale,               false },
    { "hidden",                      "getgrants_MP",                   &zus_getgrants,                  false },
    { "hidden",                      "getactivedexsells_MP",           &zus_getactivedexsells,          false },
    { "hidden",                      "getactivecrowdsales_MP",         &zus_getactivecrowdsales,        false },
    { "hidden",                      "getsto_MP",                      &zus_getsto,                     false },
    { "hidden",                      "getorderbook_MP",                &zus_getorderbook,               false },
    { "hidden",                      "gettrade_MP",                    &zus_gettrade,                   false },
    { "hidden",                      "gettransaction_MP",              &zus_gettransaction,             false },
    { "hidden",                      "listblocktransactions_MP",       &zus_listblocktransactions,      false },
#ifdef ENABLE_WALLET
    { "hidden",                      "listtransactions_MP",            &zus_listtransactions,           false },
#endif
};

void RegisterOmniDataRetrievalRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
