#include "zurbank/rpcpayload.h"

#include "zurbank/createpayload.h"
#include "zurbank/rpcvalues.h"
#include "zurbank/rpcrequirements.h"
#include "zurbank/zurbank.h"
#include "zurbank/sp.h"
#include "zurbank/tx.h"

#include "rpc/server.h"
#include "utilstrencodings.h"

#include <univalue.h>

using std::runtime_error;
using namespace mastercore;

UniValue zus_createpayload_simplesend(const UniValue& params, bool fHelp)
{
   if (fHelp || params.size() != 2)
        throw runtime_error(
            "zus_createpayload_simplesend propertyid \"amount\"\n"

            "\nCreate the payload for a simple send transaction.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens to send\n"
            "2. amount               (string, required) the amount to send\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_simplesend", "1 \"100.0\"")
            + HelpExampleRpc("zus_createpayload_simplesend", "1, \"100.0\"")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);
    int64_t amount = ParseAmount(params[1], isPropertyDivisible(propertyId));

    std::vector<unsigned char> payload = CreatePayload_SimpleSend(propertyId, amount);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_sendall(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_createpayload_sendall ecosystem\n"

            "\nCreate the payload for a send all transaction.\n"

            "\nArguments:\n"
            "1. ecosystem              (number, required) the ecosystem of the tokens to send (1 for main ecosystem, 2 for test ecosystem)\n"

            "\nResult:\n"
            "\"payload\"               (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_sendall", "2")
            + HelpExampleRpc("zus_createpayload_sendall", "2")
        );

    uint8_t ecosystem = ParseEcosystem(params[0]);

    std::vector<unsigned char> payload = CreatePayload_SendAll(ecosystem);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_dexsell(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
            "zus_createpayload_dexsell propertyidforsale \"amountforsale\" \"amountdesired\" paymentwindow minacceptfee action\n"

            "\nCreate a payload to place, update or cancel a sell offer on the traditional distributed ZUS/ZUR exchange.\n"

            "\nArguments:\n"

            "1. propertyidforsale    (number, required) the identifier of the tokens to list for sale (must be 1 for ZUS or 2 for TZUS)\n"
            "2. amountforsale        (string, required) the amount of tokens to list for sale\n"
            "3. amountdesired        (string, required) the amount of zurcoins desired\n"
            "4. paymentwindow        (number, required) a time limit in blocks a buyer has to pay following a successful accepting order\n"
            "5. minacceptfee         (string, required) a minimum mining fee a buyer has to pay to accept the offer\n"
            "6. action               (number, required) the action to take (1 for new offers, 2 to update\", 3 to cancel)\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_dexsell", "1 \"1.5\" \"0.75\" 25 \"0.0005\" 1")
            + HelpExampleRpc("zus_createpayload_dexsell", "1, \"1.5\", \"0.75\", 25, \"0.0005\", 1")
        );

    uint32_t propertyIdForSale = ParsePropertyId(params[0]);
    uint8_t action = ParseDExAction(params[5]);

    int64_t amountForSale = 0; // depending on action
    int64_t amountDesired = 0; // depending on action
    uint8_t paymentWindow = 0; // depending on action
    int64_t minAcceptFee = 0;  // depending on action

    if (action <= CMPTransaction::UPDATE) { // actions 3 permit zero values, skip check
        amountForSale = ParseAmount(params[1], true); // TMSC/MSC is divisible
        amountDesired = ParseAmount(params[2], true); // ZUR is divisible
        paymentWindow = ParseDExPaymentWindow(params[3]);
        minAcceptFee = ParseDExFee(params[4]);
    }

    std::vector<unsigned char> payload = CreatePayload_DExSell(propertyIdForSale, amountForSale, amountDesired, paymentWindow, minAcceptFee, action);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_dexaccept(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "zus_createpayload_dexaccept propertyid \"amount\"\n"

            "\nCreate the payload for an accept offer for the specified token and amount.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the token to purchase\n"
            "2. amount               (string, required) the amount to accept\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_dexaccept", "1 \"15.0\"")
            + HelpExampleRpc("zus_createpayload_dexaccept", "1, \"15.0\"")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);
    int64_t amount = ParseAmount(params[1], isPropertyDivisible(propertyId));

    std::vector<unsigned char> payload = CreatePayload_DExAccept(propertyId, amount);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_sto(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "zus_createpayload_sto propertyid \"amount\" ( distributionproperty )\n"

            "\nCreates the payload for a send-to-owners transaction.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. propertyid             (number, required) the identifier of the tokens to distribute\n"
            "2. amount                 (string, required) the amount to distribute\n"
            "3. distributionproperty   (number, optional) the identifier of the property holders to distribute to\n"
            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_sto", "3 \"5000\"")
            + HelpExampleRpc("zus_createpayload_sto", "3, \"5000\"")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);
    int64_t amount = ParseAmount(params[1], isPropertyDivisible(propertyId));
    uint32_t distributionPropertyId = (params.size() > 2) ? ParsePropertyId(params[2]) : propertyId;

    std::vector<unsigned char> payload = CreatePayload_SendToOwners(propertyId, amount, distributionPropertyId);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_issuancefixed(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 9)
        throw runtime_error(
            "zus_createpayload_issuancefixed ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\" \"amount\"\n"

            "\nCreates the payload for a new tokens issuance with fixed supply.\n"

            "\nArguments:\n"
            "1. ecosystem            (string, required) the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"
            "2. type                 (number, required) the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"
            "3. previousid           (number, required) an identifier of a predecessor token (use 0 for new tokens)\n"
            "4. category             (string, required) a category for the new tokens (can be \"\")\n"
            "5. subcategory          (string, required) a subcategory for the new tokens  (can be \"\")\n"
            "6. name                 (string, required) the name of the new tokens to create\n"
            "7. url                  (string, required) an URL for further information about the new tokens (can be \"\")\n"
            "8. data                 (string, required) a description for the new tokens (can be \"\")\n"
            "9. amount               (string, required) the number of tokens to create\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_issuancefixed", "2 1 0 \"Companies\" \"Zurcoin Mining\" \"Quantum Miner\" \"\" \"\" \"1000000\"")
            + HelpExampleRpc("zus_createpayload_issuancefixed", "2, 1, 0, \"Companies\", \"Zurcoin Mining\", \"Quantum Miner\", \"\", \"\", \"1000000\"")
        );

    uint8_t ecosystem = ParseEcosystem(params[0]);
    uint16_t type = ParsePropertyType(params[1]);
    uint32_t previousId = ParsePreviousPropertyId(params[2]);
    std::string category = ParseText(params[3]);
    std::string subcategory = ParseText(params[4]);
    std::string name = ParseText(params[5]);
    std::string url = ParseText(params[6]);
    std::string data = ParseText(params[7]);
    int64_t amount = ParseAmount(params[8], type);

    RequirePropertyName(name);

    std::vector<unsigned char> payload = CreatePayload_IssuanceFixed(ecosystem, type, previousId, category, subcategory, name, url, data, amount);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_issuancecrowdsale(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 13)
        throw runtime_error(
            "zus_createpayload_issuancecrowdsale ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\" propertyiddesired tokensperunit deadline earlybonus issuerpercentage\n"

            "\nCreates the payload for a new tokens issuance with crowdsale.\n"

            "\nArguments:\n"
            "1. ecosystem            (string, required) the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"
            "2. type                 (number, required) the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"
            "3. previousid           (number, required) an identifier of a predecessor token (0 for new crowdsales)\n"
            "4. category             (string, required) a category for the new tokens (can be \"\")\n"
            "5. subcategory          (string, required) a subcategory for the new tokens  (can be \"\")\n"
            "6. name                 (string, required) the name of the new tokens to create\n"
            "7. url                  (string, required) an URL for further information about the new tokens (can be \"\")\n"
            "8. data                 (string, required) a description for the new tokens (can be \"\")\n"
            "9. propertyiddesired    (number, required) the identifier of a token eligible to participate in the crowdsale\n"
            "10. tokensperunit       (string, required) the amount of tokens granted per unit invested in the crowdsale\n"
            "11. deadline            (number, required) the deadline of the crowdsale as Unix timestamp\n"
            "12. earlybonus          (number, required) an early bird bonus for participants in percent per week\n"
            "13. issuerpercentage    (number, required) a percentage of tokens that will be granted to the issuer\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_issuancecrowdsale", "2 1 0 \"Companies\" \"Zurcoin Mining\" \"Quantum Miner\" \"\" \"\" 2 \"100\" 1483228800 30 2")
            + HelpExampleRpc("zus_createpayload_issuancecrowdsale", "2, 1, 0, \"Companies\", \"Zurcoin Mining\", \"Quantum Miner\", \"\", \"\", 2, \"100\", 1483228800, 30, 2")
        );

    uint8_t ecosystem = ParseEcosystem(params[0]);
    uint16_t type = ParsePropertyType(params[1]);
    uint32_t previousId = ParsePreviousPropertyId(params[2]);
    std::string category = ParseText(params[3]);
    std::string subcategory = ParseText(params[4]);
    std::string name = ParseText(params[5]);
    std::string url = ParseText(params[6]);
    std::string data = ParseText(params[7]);
    uint32_t propertyIdDesired = ParsePropertyId(params[8]);
    int64_t numTokens = ParseAmount(params[9], type);
    int64_t deadline = ParseDeadline(params[10]);
    uint8_t earlyBonus = ParseEarlyBirdBonus(params[11]);
    uint8_t issuerPercentage = ParseIssuerBonus(params[12]);

    RequirePropertyName(name);
    RequireSameEcosystem(ecosystem, propertyIdDesired);

    std::vector<unsigned char> payload = CreatePayload_IssuanceVariable(ecosystem, type, previousId, category, subcategory, name, url, data, propertyIdDesired, numTokens, deadline, earlyBonus, issuerPercentage);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_issuancemanaged(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 8)
        throw runtime_error(
            "zus_createpayload_issuancemanaged ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\"\n"

            "\nCreates the payload for a new tokens issuance with manageable supply.\n"

            "\nArguments:\n"
            "1. ecosystem            (string, required) the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"
            "2. type                 (number, required) the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"
            "3. previousid           (number, required) an identifier of a predecessor token (use 0 for new tokens)\n"
            "4. category             (string, required) a category for the new tokens (can be \"\")\n"
            "5. subcategory          (string, required) a subcategory for the new tokens  (can be \"\")\n"
            "6. name                 (string, required) the name of the new tokens to create\n"
            "7. url                  (string, required) an URL for further information about the new tokens (can be \"\")\n"
            "8. data                 (string, required) a description for the new tokens (can be \"\")\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_issuancemanaged", "2 1 0 \"Companies\" \"Zurcoin Mining\" \"Quantum Miner\" \"\" \"\"")
            + HelpExampleRpc("zus_createpayload_issuancemanaged", "2, 1, 0, \"Companies\", \"Zurcoin Mining\", \"Quantum Miner\", \"\", \"\"")
        );

    uint8_t ecosystem = ParseEcosystem(params[0]);
    uint16_t type = ParsePropertyType(params[1]);
    uint32_t previousId = ParsePreviousPropertyId(params[2]);
    std::string category = ParseText(params[3]);
    std::string subcategory = ParseText(params[4]);
    std::string name = ParseText(params[5]);
    std::string url = ParseText(params[6]);
    std::string data = ParseText(params[7]);

    RequirePropertyName(name);

    std::vector<unsigned char> payload = CreatePayload_IssuanceManaged(ecosystem, type, previousId, category, subcategory, name, url, data);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_closecrowdsale(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_createpayload_closecrowdsale propertyid\n"

            "\nCreates the payload to manually close a crowdsale.\n"

            "\nArguments:\n"
            "1. propertyid             (number, required) the identifier of the crowdsale to close\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_closecrowdsale", "70")
            + HelpExampleRpc("zus_createpayload_closecrowdsale", "70")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    std::vector<unsigned char> payload = CreatePayload_CloseCrowdsale(propertyId);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_grant(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "zus_createpayload_grant propertyid \"amount\" ( \"memo\" )\n"

            "\nCreates the payload to issue or grant new units of managed tokens.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens to grant\n"
            "2. amount               (string, required) the amount of tokens to create\n"
            "3. memo                 (string, optional) a text note attached to this transaction (none by default)\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_grant", "51 \"7000\"")
            + HelpExampleRpc("zus_createpayload_grant", "51, \"7000\"")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);
    int64_t amount = ParseAmount(params[1], isPropertyDivisible(propertyId));
    std::string memo = (params.size() > 2) ? ParseText(params[2]): "";

    std::vector<unsigned char> payload = CreatePayload_Grant(propertyId, amount, memo);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_revoke(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "zus_createpayload_revoke propertyid \"amount\" ( \"memo\" )\n"

            "\nCreates the payload to revoke units of managed tokens.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens to revoke\n"
            "2. amount               (string, required) the amount of tokens to revoke\n"
            "3. memo                 (string, optional) a text note attached to this transaction (none by default)\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_revoke", "51 \"100\"")
            + HelpExampleRpc("zus_createpayload_revoke", "51, \"100\"")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);
    int64_t amount = ParseAmount(params[1], isPropertyDivisible(propertyId));
    std::string memo = (params.size() > 2) ? ParseText(params[2]): "";

    std::vector<unsigned char> payload = CreatePayload_Revoke(propertyId, amount, memo);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_changeissuer(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_createpayload_changeissuer propertyid\n"

            "\nCreats the payload to change the issuer on record of the given tokens.\n"

            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_changeissuer", "3")
            + HelpExampleRpc("zus_createpayload_changeissuer", "3")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    std::vector<unsigned char> payload = CreatePayload_ChangeIssuer(propertyId);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_trade(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "zus_createpayload_trade propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\"\n"

            "\nCreates the payload to place a trade offer on the distributed token exchange.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. propertyidforsale    (number, required) the identifier of the tokens to list for sale\n"
            "2. amountforsale        (string, required) the amount of tokens to list for sale\n"
            "3. propertiddesired     (number, required) the identifier of the tokens desired in exchange\n"
            "4. amountdesired        (string, required) the amount of tokens desired in exchange\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_trade", "31 \"250.0\" 1 \"10.0\"")
            + HelpExampleRpc("zus_createpayload_trade", "31, \"250.0\", 1, \"10.0\"")
        );

    uint32_t propertyIdForSale = ParsePropertyId(params[0]);
    int64_t amountForSale = ParseAmount(params[1], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(params[2]);
    int64_t amountDesired = ParseAmount(params[3], isPropertyDivisible(propertyIdDesired));
    RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
    RequireDifferentIds(propertyIdForSale, propertyIdDesired);
    RequireDifferentIds(propertyIdForSale, propertyIdDesired);

    std::vector<unsigned char> payload = CreatePayload_MetaDExTrade(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_canceltradesbyprice(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "zus_createpayload_canceltradesbyprice propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\"\n"

            "\nCreates the payload to cancel offers on the distributed token exchange with the specified price.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. propertyidforsale    (number, required) the identifier of the tokens listed for sale\n"
            "2. amountforsale        (string, required) the amount of tokens to listed for sale\n"
            "3. propertiddesired     (number, required) the identifier of the tokens desired in exchange\n"
            "4. amountdesired        (string, required) the amount of tokens desired in exchange\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_canceltradesbyprice", "31 \"100.0\" 1 \"5.0\"")
            + HelpExampleRpc("zus_createpayload_canceltradesbyprice", "31, \"100.0\", 1, \"5.0\"")
        );

    uint32_t propertyIdForSale = ParsePropertyId(params[0]);
    int64_t amountForSale = ParseAmount(params[1], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(params[2]);
    int64_t amountDesired = ParseAmount(params[3], isPropertyDivisible(propertyIdDesired));
    RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
    RequireDifferentIds(propertyIdForSale, propertyIdDesired);

    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelPrice(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_canceltradesbypair(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "zus_createpayload_canceltradesbypair propertyidforsale propertiddesired\n"

            "\nCreates the payload to cancel all offers on the distributed token exchange with the given currency pair.\n"

            "\nArguments:\n"
            "1. propertyidforsale    (number, required) the identifier of the tokens listed for sale\n"
            "2. propertiddesired     (number, required) the identifier of the tokens desired in exchange\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_canceltradesbypair", "1 31")
            + HelpExampleRpc("zus_createpayload_canceltradesbypair", "1, 31")
        );

    uint32_t propertyIdForSale = ParsePropertyId(params[0]);
    uint32_t propertyIdDesired = ParsePropertyId(params[1]);
    RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
    RequireDifferentIds(propertyIdForSale, propertyIdDesired);

    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelPair(propertyIdForSale, propertyIdDesired);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_cancelalltrades(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_createpayload_cancelalltrades ecosystem\n"

            "\nCreates the payload to cancel all offers on the distributed token exchange.\n"

            "\nArguments:\n"
            "1. ecosystem            (number, required) the ecosystem of the offers to cancel (1 for main ecosystem, 2 for test ecosystem)\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_cancelalltrades", "1")
            + HelpExampleRpc("zus_createpayload_cancelalltrades", "1")
        );

    uint8_t ecosystem = ParseEcosystem(params[0]);

    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelEcosystem(ecosystem);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_enablefreezing(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_createpayload_enablefreezing propertyid\n"

            "\nCreates the payload to enable address freezing for a centrally managed property.\n"

            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_enablefreezing", "3")
            + HelpExampleRpc("zus_createpayload_enablefreezing", "3")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    std::vector<unsigned char> payload = CreatePayload_EnableFreezing(propertyId);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_disablefreezing(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "zus_createpayload_disablefreezing propertyid\n"

            "\nCreates the payload to disable address freezing for a centrally managed property.\n"
            "\nIMPORTANT NOTE:  Disabling freezing for a property will UNFREEZE all frozen addresses for that property!"

            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_disablefreezing", "3")
            + HelpExampleRpc("zus_createpayload_disablefreezing", "3")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    std::vector<unsigned char> payload = CreatePayload_DisableFreezing(propertyId);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_freeze(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "zus_createpayload_freeze \"toaddress\" propertyid amount \n"

            "\nCreates the payload to freeze an address for a centrally managed token.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. toaddress            (string, required) the address to freeze tokens for\n"
            "2. propertyid           (number, required) the property to freeze tokens for (must be managed type and have freezing option enabled)\n"
            "3. amount               (string, required) the amount of tokens to freeze (note: this is unused - once frozen an address cannot send any transactions)\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_freeze", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 1 \"100\"")
            + HelpExampleRpc("zus_createpayload_freeze", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 1, \"100\"")
        );

    std::string refAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);
    int64_t amount = ParseAmount(params[2], isPropertyDivisible(propertyId));

    std::vector<unsigned char> payload = CreatePayload_FreezeTokens(propertyId, amount, refAddress);

    return HexStr(payload.begin(), payload.end());
}

UniValue zus_createpayload_unfreeze(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "zus_createpayload_unfreeze \"toaddress\" propertyid amount \n"

            "\nCreates the payload to unfreeze an address for a centrally managed token.\n"

            "\nNote: if the server is not synchronized, amounts are considered as divisible, even if the token may have indivisible units!\n"

            "\nArguments:\n"
            "1. toaddress            (string, required) the address to unfreeze tokens for\n"
            "2. propertyid           (number, required) the property to unfreeze tokens for (must be managed type and have freezing option enabled)\n"
            "3. amount               (string, required) the amount of tokens to unfreeze (note: this is unused)\n"

            "\nResult:\n"
            "\"payload\"             (string) the hex-encoded payload\n"

            "\nExamples:\n"
            + HelpExampleCli("zus_createpayload_unfreeze", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 1 \"100\"")
            + HelpExampleRpc("zus_createpayload_unfreeze", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 1, \"100\"")
        );

    std::string refAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);
    int64_t amount = ParseAmount(params[2], isPropertyDivisible(propertyId));

    std::vector<unsigned char> payload = CreatePayload_UnfreezeTokens(propertyId, amount, refAddress);

    return HexStr(payload.begin(), payload.end());
}

static const CRPCCommand commands[] =
{ //  category                         name                                      actor (function)                         okSafeMode
  //  -------------------------------- ----------------------------------------- ---------------------------------------- ----------
    { "omni layer (payload creation)", "zus_createpayload_simplesend",          &zus_createpayload_simplesend,          true },
    { "omni layer (payload creation)", "zus_createpayload_sendall",             &zus_createpayload_sendall,             true },
    { "omni layer (payload creation)", "zus_createpayload_dexsell",             &zus_createpayload_dexsell,             true },
    { "omni layer (payload creation)", "zus_createpayload_dexaccept",           &zus_createpayload_dexaccept,           true },
    { "omni layer (payload creation)", "zus_createpayload_sto",                 &zus_createpayload_sto,                 true },
    { "omni layer (payload creation)", "zus_createpayload_grant",               &zus_createpayload_grant,               true },
    { "omni layer (payload creation)", "zus_createpayload_revoke",              &zus_createpayload_revoke,              true },
    { "omni layer (payload creation)", "zus_createpayload_changeissuer",        &zus_createpayload_changeissuer,        true },
    { "omni layer (payload creation)", "zus_createpayload_trade",               &zus_createpayload_trade,               true },
    { "omni layer (payload creation)", "zus_createpayload_issuancefixed",       &zus_createpayload_issuancefixed,       true },
    { "omni layer (payload creation)", "zus_createpayload_issuancecrowdsale",   &zus_createpayload_issuancecrowdsale,   true },
    { "omni layer (payload creation)", "zus_createpayload_issuancemanaged",     &zus_createpayload_issuancemanaged,     true },
    { "omni layer (payload creation)", "zus_createpayload_closecrowdsale",      &zus_createpayload_closecrowdsale,      true },
    { "omni layer (payload creation)", "zus_createpayload_canceltradesbyprice", &zus_createpayload_canceltradesbyprice, true },
    { "omni layer (payload creation)", "zus_createpayload_canceltradesbypair",  &zus_createpayload_canceltradesbypair,  true },
    { "omni layer (payload creation)", "zus_createpayload_cancelalltrades",     &zus_createpayload_cancelalltrades,     true },
    { "omni layer (payload creation)", "zus_createpayload_enablefreezing",      &zus_createpayload_enablefreezing,      true },
    { "omni layer (payload creation)", "zus_createpayload_disablefreezing",     &zus_createpayload_disablefreezing,     true },
    { "omni layer (payload creation)", "zus_createpayload_freeze",              &zus_createpayload_freeze,              true },
    { "omni layer (payload creation)", "zus_createpayload_unfreeze",            &zus_createpayload_unfreeze,            true },
};

void RegisterOmniPayloadCreationRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
