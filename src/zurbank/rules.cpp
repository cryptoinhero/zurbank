/**
 * @file rules.cpp
 *
 * This file contains consensus rules and restrictions.
 */

#include "zurbank/rules.h"

#include "zurbank/activation.h"
#include "zurbank/consensushash.h"
#include "zurbank/dbtxlist.h"
#include "zurbank/log.h"
#include "zurbank/zurbank.h"
#include "zurbank/notifications.h"
#include "zurbank/utilszurcoin.h"
#include "zurbank/version.h"

#include "chainparams.h"
#include "main.h"
#include "script/standard.h"
#include "uint256.h"
#include "ui_interface.h"

#include <openssl/sha.h>

#include <stdint.h>
#include <limits>
#include <string>
#include <vector>

namespace mastercore
{
/**
 * Returns a mapping of transaction types, and the blocks at which they are enabled.
 */
std::vector<TransactionRestriction> CConsensusParams::GetRestrictions() const
{
    const TransactionRestriction vTxRestrictions[] =
    { //  transaction type                    version        allow 0  activation block
      //  ----------------------------------  -------------  -------  ------------------
        { ZURBANK_MESSAGE_TYPE_ALERT,        0xFFFF,        true,    MSC_ALERT_BLOCK    },
        { ZURBANK_MESSAGE_TYPE_ACTIVATION,   0xFFFF,        true,    MSC_ALERT_BLOCK    },
        { ZURBANK_MESSAGE_TYPE_DEACTIVATION, 0xFFFF,        true,    MSC_ALERT_BLOCK    },

        { MSC_TYPE_SIMPLE_SEND,               MP_TX_PKT_V0,  false,   MSC_SEND_BLOCK     },

        { MSC_TYPE_TRADE_OFFER,               MP_TX_PKT_V0,  false,   MSC_DEX_BLOCK      },
        { MSC_TYPE_TRADE_OFFER,               MP_TX_PKT_V1,  false,   MSC_DEX_BLOCK      },
        { MSC_TYPE_ACCEPT_OFFER_ZUR,          MP_TX_PKT_V0,  false,   MSC_DEX_BLOCK      },

        { MSC_TYPE_CREATE_PROPERTY_FIXED,     MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CREATE_PROPERTY_VARIABLE,  MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CREATE_PROPERTY_VARIABLE,  MP_TX_PKT_V1,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CLOSE_CROWDSALE,           MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },

        { MSC_TYPE_CREATE_PROPERTY_MANUAL,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_GRANT_PROPERTY_TOKENS,     MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_REVOKE_PROPERTY_TOKENS,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_CHANGE_ISSUER_ADDRESS,     MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_ENABLE_FREEZING,           MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_DISABLE_FREEZING,          MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_FREEZE_PROPERTY_TOKENS,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_UNFREEZE_PROPERTY_TOKENS,  MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },

        { MSC_TYPE_SEND_TO_OWNERS,            MP_TX_PKT_V0,  false,   MSC_STO_BLOCK      },
        { MSC_TYPE_SEND_TO_OWNERS,            MP_TX_PKT_V1,  false,   MSC_STOV1_BLOCK    },

        { MSC_TYPE_METADEX_TRADE,             MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_PRICE,      MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_PAIR,       MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_ECOSYSTEM,  MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },

        { MSC_TYPE_SEND_ALL,                  MP_TX_PKT_V0,  false,   MSC_SEND_ALL_BLOCK },

        { MSC_TYPE_OFFER_ACCEPT_A_BET,        MP_TX_PKT_V0,  false,   MSC_BET_BLOCK      },
    };

    const size_t nSize = sizeof(vTxRestrictions) / sizeof(vTxRestrictions[0]);

    return std::vector<TransactionRestriction>(vTxRestrictions, vTxRestrictions + nSize);
}

/**
 * Returns an empty vector of consensus checkpoints.
 *
 * This method should be overwriten by the child classes, if needed.
 */
std::vector<ConsensusCheckpoint> CConsensusParams::GetCheckpoints() const
{
    return std::vector<ConsensusCheckpoint>();
}

/**
 * Returns an empty vector of transaction checkpoints.
 *
 * This method should be overwriten by the child classes, if needed.
 */
std::vector<TransactionCheckpoint> CConsensusParams::GetTransactions() const
{
    return std::vector<TransactionCheckpoint>();
}

/**
 * Returns consensus checkpoints for mainnet, used to verify transaction processing.
 */
std::vector<ConsensusCheckpoint> CMainConsensusParams::GetCheckpoints() const
{
    // block height, block hash and consensus hash
    const ConsensusCheckpoint vCheckpoints[] = {
        // { 562708, uint256S("0000000000000000001e8a2aeca536b0ee2b97ae85f83980f00a329dd93ac557"),
        //           uint256S("f06698346373e7e7df1fa4227f45695d05fcf26096e7200499af23644a1ed762") },
    };

    const size_t nSize = sizeof(vCheckpoints) / sizeof(vCheckpoints[0]);

    return std::vector<ConsensusCheckpoint>(vCheckpoints, vCheckpoints + nSize);
}

/**
 * Returns transactions checkpoints for mainnet, used to verify DB consistency.
 */
std::vector<TransactionCheckpoint> CMainConsensusParams::GetTransactions() const
{
    // block height, transaction hash
    const TransactionCheckpoint vTransactions[] = {
        { 306906, uint256S("b7c66175a99ca0e7b1691905d50a46165adb7a8012d9ec5e1ecf8239f859df6d") },
        { 306906, uint256S("a59ac18eda590dfe9f3671f99f3cec5679e95ea4d1478d5313e2b4706307537b") },
        { 306912, uint256S("ce56ee84abd20bd1386d66e5ab9e3661a9bfad45a7d46490cae2241bbebf91df") },
        { 306913, uint256S("bbdb911227c033697e92d6828d43b15f99bbc24c97147be0aed12dd98f362c28") },
        { 307290, uint256S("69604c02f219136245d309b722816d6f4f77ccead60ded6976219f9a0becceff") },
        { 307403, uint256S("201c38a5b819c870830ddd7583d446f05a12c6b01ec3690e3acf33e49fe8d91f") },
        { 307620, uint256S("7ddac86217f6fc37a11f7de3b8edb78b6f7d10e93cece8f6bf0565ddc95acc76") },
        { 307897, uint256S("22b4ee94c5a31a082e1b8bc77e79a54783262b19f957d6603f16876397546e58") },
        { 308666, uint256S("84fc343c89218faa9f6f31a9ee266837e19d9bc1319737ae66fa2d1fd095d271") },
        { 309030, uint256S("f3493671fe79a9dd5af871ac1f9b715db8e895c80584031091c626e43a73d4e0") },
        { 309031, uint256S("eda3d2bbb8125397f4d4909ea25d845dc451e8a3206035bf0d736bb3ece5d758") },
        { 309052, uint256S("a4abfc7a10a262bec4a9be830fa784cbb712cfa7d19fb28e854c3d7d7ea50719") },
        { 310003, uint256S("6d373615f4938cc9dfacf213ba0f33ec1b63961a61b4869ea69e08c938671367") },
        { 310114, uint256S("17e48d57fe979aa821b30c14edf76b5203f5dc72847ab9350c068870551d6a5d") },
        { 310121, uint256S("a67609d7941c8e7dc33fd1e4a286f7407e6139c61bec3247d20f546e0b3c2e7f") },
        { 310132, uint256S("21aa754a425c7748688a57bb409cf6942a3eb7b087b32e29254fae62e5dbbe71") },
        { 310313, uint256S("18e955f7e8aa7efa2f07768c9916967cea1821c197a2d9f815a51358b82a549d") },
        { 310502, uint256S("6ab48c227a3aed94df462ab2628a87b566d9bc985b27fd8d3e86f52794e3d735") },
        { 310577, uint256S("1b9d32711dd896d18b083c1d3d12f2ba661c5cbcdc2122a224ecfb462a58eb92") },
        { 310985, uint256S("1d8af5bfaf69b3cea356501f681c698017b60d1aef3d1dcdb2e72454ae682add") },
        { 312967, uint256S("5b922dad64c5c4241c694401d627d6c0e33033215f1fb375aa21b435fa032fce") },
        { 313118, uint256S("e152ffe783d54f7dbf2ab846d3c1904037b26788e48617621e1fdefe151be42a") },
        { 313538, uint256S("b01d1594a7e2083ebcd428706045df003f290c4dc7bd6d77c93df9fcca68232f") },
        { 314110, uint256S("189c959a37ecac2045ba3631b87e151e09506e6a492e867bbcc6c89e3bbc91e7") },
        { 314126, uint256S("7244beac4e5165b3386728f2cd4c203dd6144b6676853e5012a87b75135468b0") },
        { 315312, uint256S("8fbd96005aba5671daf8288f89df8026a7ce4782a0bb411937537933956b827b") },
        { 420430, uint256S("64ab528aa6407033ac4925601f132d1ce3bca47ec140d27e7f5a9bad0f37cbc6") },
        { 420551, uint256S("5b81b20508c98a9bda3de68d6b3872f52efd02ebe2cb9b114cfb3a3225f33dd1") },
    };

    const size_t nSize = sizeof(vTransactions) / sizeof(vTransactions[0]);

    return std::vector<TransactionCheckpoint>(vTransactions, vTransactions + nSize);
}

/**
 * Constructor for mainnet consensus parameters.
 */
CMainConsensusParams::CMainConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.10;
    exodusDeadline = 1615795200;
    exodusReward = 100;
    GENESIS_BLOCK = 4020000;
    LAST_EXODUS_BLOCK = 4031000;
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 28800;  // ~2 weeks
    MAX_ACTIVATION_BLOCKS = 172800; // ~12 weeks
    // Waiting period for enabling freezing
    OMNI_FREEZE_WAIT_PERIOD = 57600; // ~4 weeks
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 4020000;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 4032000;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 4020000;
    MSC_DEX_BLOCK = 4032000;
    MSC_SP_BLOCK = 4032000;
    MSC_MANUALSP_BLOCK = 4032000;
    MSC_STO_BLOCK = 4032000;
    MSC_METADEX_BLOCK = 4032000;
    MSC_SEND_ALL_BLOCK = 4032000;
    MSC_BET_BLOCK = 9999999;
    MSC_STOV1_BLOCK = 9999999;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 4032000;
    DEXMATH_FEATURE_BLOCK = 4032000;
    SPCROWDCROSSOVER_FEATURE_BLOCK = 4032000;
    TRADEALLPAIRS_FEATURE_BLOCK = 4032000;
    FEES_FEATURE_BLOCK = 9999999;
    FREEZENOTICE_FEATURE_BLOCK = 9999999;
}

/**
 * Constructor for testnet consensus parameters.
 */
CTestNetConsensusParams::CTestNetConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.00;
    exodusDeadline = 1615795200;
    exodusReward = 100;
    GENESIS_BLOCK = 263000;
    LAST_EXODUS_BLOCK = std::numeric_limits<int>::max();
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 0;
    MAX_ACTIVATION_BLOCKS = 999999;
    // Waiting period for enabling freezing
    OMNI_FREEZE_WAIT_PERIOD = 0;
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 0;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 0;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 0;
    MSC_DEX_BLOCK = 0;
    MSC_SP_BLOCK = 0;
    MSC_MANUALSP_BLOCK = 0;
    MSC_STO_BLOCK = 0;
    MSC_METADEX_BLOCK = 0;
    MSC_SEND_ALL_BLOCK = 0;
    MSC_BET_BLOCK = 999999;
    MSC_STOV1_BLOCK = 0;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 0;
    DEXMATH_FEATURE_BLOCK = 0;
    SPCROWDCROSSOVER_FEATURE_BLOCK = 0;
    TRADEALLPAIRS_FEATURE_BLOCK = 0;
    FEES_FEATURE_BLOCK = 0;
    FREEZENOTICE_FEATURE_BLOCK = 0;
}

/**
 * Constructor for regtest consensus parameters.
 */
CRegTestConsensusParams::CRegTestConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.00;
    exodusDeadline = 1615795200;
    exodusReward = 100;
    GENESIS_BLOCK = 101;
    LAST_EXODUS_BLOCK = std::numeric_limits<int>::max();
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 5;
    MAX_ACTIVATION_BLOCKS = 10;
    // Waiting period for enabling freezing
    OMNI_FREEZE_WAIT_PERIOD = 10;
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 0;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 0;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 0;
    MSC_DEX_BLOCK = 0;
    MSC_SP_BLOCK = 0;
    MSC_MANUALSP_BLOCK = 0;
    MSC_STO_BLOCK = 0;
    MSC_METADEX_BLOCK = 0;
    MSC_SEND_ALL_BLOCK = 0;
    MSC_BET_BLOCK = 999999;
    MSC_STOV1_BLOCK = 999999;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 999999;
    DEXMATH_FEATURE_BLOCK = 999999;
    SPCROWDCROSSOVER_FEATURE_BLOCK = 999999;
    TRADEALLPAIRS_FEATURE_BLOCK = 999999;
    FEES_FEATURE_BLOCK = 999999;
    FREEZENOTICE_FEATURE_BLOCK = 999999;
}

//! Consensus parameters for mainnet
static CMainConsensusParams mainConsensusParams;
//! Consensus parameters for testnet
static CTestNetConsensusParams testNetConsensusParams;
//! Consensus parameters for regtest mode
static CRegTestConsensusParams regTestConsensusParams;

/**
 * Returns consensus parameters for the given network.
 */
CConsensusParams& ConsensusParams(const std::string& network)
{
    if (network == "main") {
        return mainConsensusParams;
    }
    if (network == "test") {
        return testNetConsensusParams;
    }
    if (network == "regtest") {
        return regTestConsensusParams;
    }
    // Fallback:
    return mainConsensusParams;
}

/**
 * Returns currently active consensus parameter.
 */
const CConsensusParams& ConsensusParams()
{
    const std::string& network = Params().NetworkIDString();

    return ConsensusParams(network);
}

/**
 * Returns currently active mutable consensus parameter.
 */
CConsensusParams& MutableConsensusParams()
{
    const std::string& network = Params().NetworkIDString();

    return ConsensusParams(network);
}

/**
 * Resets consensus paramters.
 */
void ResetConsensusParams()
{
    mainConsensusParams = CMainConsensusParams();
    testNetConsensusParams = CTestNetConsensusParams();
    regTestConsensusParams = CRegTestConsensusParams();
}

/**
 * Checks, if the script type is allowed as input.
 */
bool IsAllowedInputType(int whichType, int nBlock)
{
    const CConsensusParams& params = ConsensusParams();

    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return (params.PUBKEYHASH_BLOCK <= nBlock);

        case TX_SCRIPTHASH:
            return (params.SCRIPTHASH_BLOCK <= nBlock);
    }

    return false;
}

/**
 * Checks, if the script type qualifies as output.
 */
bool IsAllowedOutputType(int whichType, int nBlock)
{
    const CConsensusParams& params = ConsensusParams();

    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return (params.PUBKEYHASH_BLOCK <= nBlock);

        case TX_SCRIPTHASH:
            return (params.SCRIPTHASH_BLOCK <= nBlock);

        case TX_MULTISIG:
            return (params.MULTISIG_BLOCK <= nBlock);

        case TX_NULL_DATA:
            return (params.NULLDATA_BLOCK <= nBlock);
    }

    return false;
}

/**
 * Activates a feature at a specific block height, authorization has already been validated.
 *
 * Note: Feature activations are consensus breaking.  It is not permitted to activate a feature within
 *       the next 2048 blocks (roughly 2 weeks), nor is it permitted to activate a feature further out
 *       than 12288 blocks (roughly 12 weeks) to ensure sufficient notice.
 *       This does not apply for activation during initialization (where loadingActivations is set true).
 */
bool ActivateFeature(uint16_t featureId, int activationBlock, uint32_t minClientVersion, int transactionBlock)
{
    PrintToLog("Feature activation requested (ID %d to go active as of block: %d)\n", featureId, activationBlock);

    const CConsensusParams& params = ConsensusParams();

    // check activation block is allowed
    if ((activationBlock < (transactionBlock + params.MIN_ACTIVATION_BLOCKS)) ||
        (activationBlock > (transactionBlock + params.MAX_ACTIVATION_BLOCKS))) {
            PrintToLog("Feature activation of ID %d refused due to notice checks\n", featureId);
            return false;
    }

    // check whether the feature is already active
    if (IsFeatureActivated(featureId, transactionBlock)) {
        PrintToLog("Feature activation of ID %d refused as the feature is already live\n", featureId);
        return false;
    }

    // check feature is recognized and activation is successful
    std::string featureName = GetFeatureName(featureId);
    bool supported = ZURBANK_VERSION >= minClientVersion;
    switch (featureId) {
        case FEATURE_CLASS_C:
            MutableConsensusParams().NULLDATA_BLOCK = activationBlock;
        break;
        case FEATURE_METADEX:
            MutableConsensusParams().MSC_METADEX_BLOCK = activationBlock;
        break;
        case FEATURE_BETTING:
            MutableConsensusParams().MSC_BET_BLOCK = activationBlock;
        break;
        case FEATURE_GRANTEFFECTS:
            MutableConsensusParams().GRANTEFFECTS_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_DEXMATH:
            MutableConsensusParams().DEXMATH_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_SENDALL:
            MutableConsensusParams().MSC_SEND_ALL_BLOCK = activationBlock;
        break;
        case FEATURE_SPCROWDCROSSOVER:
            MutableConsensusParams().SPCROWDCROSSOVER_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_TRADEALLPAIRS:
            MutableConsensusParams().TRADEALLPAIRS_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_FEES:
            MutableConsensusParams().FEES_FEATURE_BLOCK = activationBlock;
        break;
        case FEATURE_STOV1:
            MutableConsensusParams().MSC_STOV1_BLOCK = activationBlock;
        break;
        case FEATURE_FREEZENOTICE:
            MutableConsensusParams().FREEZENOTICE_FEATURE_BLOCK = activationBlock;
        break;
        default:
            supported = false;
        break;
    }

    PrintToLog("Feature activation of ID %d processed. %s will be enabled at block %d.\n", featureId, featureName, activationBlock);
    AddPendingActivation(featureId, activationBlock, minClientVersion, featureName);

    if (!supported) {
        PrintToLog("WARNING!!! AS OF BLOCK %d THIS CLIENT WILL BE OUT OF CONSENSUS AND WILL AUTOMATICALLY SHUTDOWN.\n", activationBlock);
        std::string alertText = strprintf("Your client must be updated and will shutdown at block %d (unsupported feature %d ('%s') activated)\n",
                                          activationBlock, featureId, featureName);
        AddAlert("zurbank", ALERT_BLOCK_EXPIRY, activationBlock, alertText);
        AlertNotify(alertText);
    }

    return true;
}

/**
 * Deactivates a feature immediately, authorization has already been validated.
 *
 * Note: There is no notice period for feature deactivation as:
 *       # It is reserved for emergency use in the event an exploit is found
 *       # No client upgrade is required
 *       # No action is required by users
 */
bool DeactivateFeature(uint16_t featureId, int transactionBlock)
{
    PrintToLog("Immediate feature deactivation requested (ID %d)\n", featureId);

    if (!IsFeatureActivated(featureId, transactionBlock)) {
        PrintToLog("Feature deactivation of ID %d refused as the feature is not yet live\n", featureId);
        return false;
    }

    std::string featureName = GetFeatureName(featureId);
    switch (featureId) {
        case FEATURE_CLASS_C:
            MutableConsensusParams().NULLDATA_BLOCK = 999999;
        break;
        case FEATURE_METADEX:
            MutableConsensusParams().MSC_METADEX_BLOCK = 999999;
        break;
        case FEATURE_BETTING:
            MutableConsensusParams().MSC_BET_BLOCK = 999999;
        break;
        case FEATURE_GRANTEFFECTS:
            MutableConsensusParams().GRANTEFFECTS_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_DEXMATH:
            MutableConsensusParams().DEXMATH_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_SENDALL:
            MutableConsensusParams().MSC_SEND_ALL_BLOCK = 999999;
        break;
        case FEATURE_SPCROWDCROSSOVER:
            MutableConsensusParams().SPCROWDCROSSOVER_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_TRADEALLPAIRS:
            MutableConsensusParams().TRADEALLPAIRS_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_FEES:
            MutableConsensusParams().FEES_FEATURE_BLOCK = 999999;
        break;
        case FEATURE_STOV1:
            MutableConsensusParams().MSC_STOV1_BLOCK = 999999;
        break;
        case FEATURE_FREEZENOTICE:
            MutableConsensusParams().FREEZENOTICE_FEATURE_BLOCK = 999999;
        break;
        default:
            return false;
        break;
    }

    PrintToLog("Feature deactivation of ID %d processed. %s has been disabled.\n", featureId, featureName);

    std::string alertText = strprintf("An emergency deactivation of feature ID %d (%s) has occurred.", featureId, featureName);
    AddAlert("zurbank", ALERT_BLOCK_EXPIRY, transactionBlock + 1024, alertText);
    AlertNotify(alertText);

    return true;
}

/**
 * Returns the display name of a feature ID
 */
std::string GetFeatureName(uint16_t featureId)
{
    switch (featureId) {
        case FEATURE_CLASS_C: return "Class C transaction encoding";
        case FEATURE_METADEX: return "Distributed Meta Token Exchange";
        case FEATURE_BETTING: return "Bet transactions";
        case FEATURE_GRANTEFFECTS: return "Remove grant side effects";
        case FEATURE_DEXMATH: return "DEx integer math update";
        case FEATURE_SENDALL: return "Send All transactions";
        case FEATURE_SPCROWDCROSSOVER: return "Disable crowdsale ecosystem crossovers";
        case FEATURE_TRADEALLPAIRS: return "Allow trading all pairs on the Distributed Exchange";
        case FEATURE_FEES: return "Fee system (inc 0.05% fee from trades of non-Omni pairs)";
        case FEATURE_STOV1: return "Cross-property Send To Owners";
        case FEATURE_FREEZENOTICE: return "Activate the waiting period for enabling freezing";

        default: return "Unknown feature";
    }
}

/**
 * Checks, whether a feature is activated at the given block.
 */
bool IsFeatureActivated(uint16_t featureId, int transactionBlock)
{
    const CConsensusParams& params = ConsensusParams();
    int activationBlock = std::numeric_limits<int>::max();

    switch (featureId) {
        case FEATURE_CLASS_C:
            activationBlock = params.NULLDATA_BLOCK;
            break;
        case FEATURE_METADEX:
            activationBlock = params.MSC_METADEX_BLOCK;
            break;
        case FEATURE_BETTING:
            activationBlock = params.MSC_BET_BLOCK;
            break;
        case FEATURE_GRANTEFFECTS:
            activationBlock = params.GRANTEFFECTS_FEATURE_BLOCK;
            break;
        case FEATURE_DEXMATH:
            activationBlock = params.DEXMATH_FEATURE_BLOCK;
            break;
        case FEATURE_SENDALL:
            activationBlock = params.MSC_SEND_ALL_BLOCK;
            break;
        case FEATURE_SPCROWDCROSSOVER:
            activationBlock = params.SPCROWDCROSSOVER_FEATURE_BLOCK;
            break;
        case FEATURE_TRADEALLPAIRS:
            activationBlock = params.TRADEALLPAIRS_FEATURE_BLOCK;
            break;
        case FEATURE_FEES:
            activationBlock = params.FEES_FEATURE_BLOCK;
            break;
        case FEATURE_STOV1:
            activationBlock = params.MSC_STOV1_BLOCK;
            break;
        case FEATURE_FREEZENOTICE:
            activationBlock = params.FREEZENOTICE_FEATURE_BLOCK;
        break;
        default:
            return false;
    }

    return (transactionBlock >= activationBlock);
}

/**
 * Checks, if the transaction type and version is supported and enabled.
 *
 * In the test ecosystem, transactions, which are known to the client are allowed
 * without height restriction.
 *
 * Certain transactions use a property identifier of 0 (= ZUR) as wildcard, which
 * must explicitly be allowed.
 */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version)
{
    const std::vector<TransactionRestriction>& vTxRestrictions = ConsensusParams().GetRestrictions();

    for (std::vector<TransactionRestriction>::const_iterator it = vTxRestrictions.begin(); it != vTxRestrictions.end(); ++it)
    {
        const TransactionRestriction& entry = *it;
        if (entry.txType != txType || entry.txVersion != version) {
            continue;
        }
        // a property identifier of 0 (= ZUR) may be used as wildcard
        if (OMNI_PROPERTY_ZUR == txProperty && !entry.allowWildcard) {
            continue;
        }
        // transactions are not restricted in the test ecosystem
        if (isTestEcosystemProperty(txProperty)) {
            return true;
        }
        if (txBlock >= entry.activationBlock) {
            return true;
        }
    }

    return false;
}

/**
 * Compares a supplied block, block hash and consensus hash against a hardcoded list of checkpoints.
 */
bool VerifyCheckpoint(int block, const uint256& blockHash)
{
    // optimization; we only checkpoint every 10,000 blocks - skip any further work if block not a multiple of 10K
    if (block % 10000 != 0) return true;

    const std::vector<ConsensusCheckpoint>& vCheckpoints = ConsensusParams().GetCheckpoints();

    for (std::vector<ConsensusCheckpoint>::const_iterator it = vCheckpoints.begin(); it != vCheckpoints.end(); ++it) {
        const ConsensusCheckpoint& checkpoint = *it;
        if (block != checkpoint.blockHeight) {
            continue;
        }

        if (blockHash != checkpoint.blockHash) {
            PrintToLog("%s(): block hash mismatch - expected %s, received %s\n", __func__, checkpoint.blockHash.GetHex(), blockHash.GetHex());
            return false;
        }

        // only verify if there is a checkpoint to verify against
        uint256 consensusHash = GetConsensusHash();
        if (consensusHash != checkpoint.consensusHash) {
            PrintToLog("%s(): consensus hash mismatch - expected %s, received %s\n", __func__, checkpoint.consensusHash.GetHex(), consensusHash.GetHex());
            return false;
        } else {
            break;
        }
    }

    // either checkpoint matched or we don't have a checkpoint for this block
    return true;
}

/**
 * Checks, if a specific transaction exists in the database.
 */
bool VerifyTransactionExistence(int block)
{
    PrintToLog("%s: verifying existence of historical transactions up to block %d..\n", __func__, block);

    const std::vector<TransactionCheckpoint>& vTransactionss = ConsensusParams().GetTransactions();

    for (std::vector<TransactionCheckpoint>::const_iterator it = vTransactionss.begin(); it != vTransactionss.end(); ++it) {
        const TransactionCheckpoint& checkpoint = *it;
        if (block < checkpoint.blockHeight) {
            continue;
        }

        if (!mastercore::pDbTransactionList->exists(checkpoint.txHash)) {
            PrintToLog("%s: ERROR: failed to find historical transaction %s in block %d\n",
                    __func__, checkpoint.txHash.GetHex(), checkpoint.blockHeight);
            return false;
        }
    }

    return true;
}

} // namespace mastercore
