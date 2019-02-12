// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2019 The PivX developers
// Copyright (c) 2018-2019 The KYD developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Params.h"
#include "chainparams.h"
#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress>& vSeedsOut, const SeedSpec6* data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//   What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
static Checkpoints::MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of
    (0, uint256("0x00000a063178bcfbd2c7c7dc62702ec982b10d470032916eb4436a5865100d9b"))
    (250, uint256("0x0000027a906e79807e5366cb898801ddae8c1c613079b2b8d86f91d50bca9280"))
    (2500, uint256("0xc120830f40226255b0b4c9f787c99af92699712fc52ca190fc23000707813f92"))
    (5250, uint256("0x745c5e4c4e49d7f9d31d87bfe2d5ab9b94f1073c1a7363ecb217ee03104d6858"))
    (122500, uint256("2004719bd307f37b0baaa095bb1ad5433a790e5e5b68dcb6ab8ff40672159b6c"));

static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
    1549420078, // * UNIX timestamp of last checkpoint block
    251748,      // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the SetBestChain debug.log lines)
    2000        // * estimated number of transactions per day after checkpoint
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("0x001"));
static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
    1740710,
    0,
    250};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("0x001"));
static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
    1454124731,
    0,
    100};

libzerocoin::ZerocoinParams* CChainParams::Zerocoin_Params(bool useModulusV1) const
{
    assert(this);
    static CBigNum bnHexModulus = 0;
    if (!bnHexModulus)
        bnHexModulus.SetHex(zerocoinModulus);
    static libzerocoin::ZerocoinParams ZCParamsHex = libzerocoin::ZerocoinParams(bnHexModulus);
    static CBigNum bnDecModulus = 0;
    if (!bnDecModulus)
        bnDecModulus.SetDec(zerocoinModulus);
    static libzerocoin::ZerocoinParams ZCParamsDec = libzerocoin::ZerocoinParams(bnDecModulus);

    if (useModulusV1)
        return &ZCParamsHex;

    return &ZCParamsDec;
}

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0xa5;
        pchMessageStart[1] = 0xc2;
        pchMessageStart[2] = 0xd1;
        pchMessageStart[3] = 0xe6;
        vAlertPubKey = ParseHex("04fd8606e6bbc284001d0ec860080d59d86b1d3a82dd26fde49fc8724de70f4e36fcec5ec2ac113e78e391b59b09c49fb17fc580d8f50f1e7eb1a105edd25e14b1");
        nDefaultPort = 12244;
        bnProofOfWorkLimit = ~uint256(0) >> 20; // KYD starting difficulty is 1 / 2^20
        bnPOSWorkLimit = ~uint256(0) >> 24; // KYD max POS difficulty is 1 / 2^24
        nSubsidyHalvingInterval = 210000;
        nMaxReorganizationDepth = 100;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0;
        nTargetTimespan = 1 * 60; // KYD: 1 day
        nTargetSpacing = 1 * 30;  // KYD: 1 minute
        nPOSTargetSpacing = 60;
        nPOSDGWForkBlock = 55000;
        nMaturity = 20;
        nMasternodeCountDrift = 20;
        nMaxMoneyOut = 21000000 * COIN;

        nWalletForkBlock = 275;
        nDiffForkBlock = 150000;

        /** Height or Time Based Activations **/
        nLastPOWBlock = 250;
        nModifierUpdateBlock = 1;
        nZerocoinStartHeight = INT_MAX-1; //Never Activated
        nZerocoinStartTime = INT_MAX-1; //Never Activated
        nBlockEnforceSerialRange = 1; //Enforce serial range starting this block
        nBlockRecalculateAccumulators = INT_MAX-1; //Trigger a recalculation of accumulators
        nBlockFirstFraudulent = INT_MAX-1; //First block that bad serials emerged
        nBlockLastGoodCheckpoint = INT_MAX-1; //Last valid accumulator checkpoint
        nBlockEnforceInvalidUTXO = INT_MAX-1; //Start enforcing the invalid UTXO's and staking rewards
        nInvalidAmountFiltered = 0*COIN; //Amount of invalid coins filtered through exchanges, that should be considered valid
        nBlockZerocoinV2 = INT_MAX-1; //!> The block that zerocoin v2 becomes active - roughly Tuesday, May 8, 2018 4:00:00 AM GMT
        nEnforceNewSporkKey = 1549497600; //!> Sporks signed after (GMT): 02/07/2019 @ 12:00am (UTC) must use the new spork key
        nRejectOldSporkKey = 1549497600; //!> Fully reject old spork key after (GMT): Friday, June 1, 2018 12:00:00 AM

         const char* pszTimestamp = "KYD - Trust between community and developer";
         CMutableTransaction txNew;
         txNew.vin.resize(1);
         txNew.vout.resize(1);
         txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
         txNew.vout[0].nValue = 0 * COIN;
         txNew.vout[0].scriptPubKey = CScript() << ParseHex("046e4f58d4517c352854232f9f9e021ee812f4b49df81b8c912fbb622c41678353980a8b7de4bddc393ec263ab64dc696f5cbf89ba20d1bb97bcab94bff5c80b4b") << OP_CHECKSIG;
         genesis.vtx.push_back(txNew);
         genesis.hashPrevBlock = 0;
         genesis.hashMerkleRoot = genesis.BuildMerkleTree();
         genesis.nVersion = 1;
         genesis.nTime = 1544339974;
         genesis.nBits = 0x1e0fffff;
         genesis.nNonce = 8544699;

         hashGenesisBlock = genesis.GetHash();
         assert(hashGenesisBlock == uint256("0x00000a063178bcfbd2c7c7dc62702ec982b10d470032916eb4436a5865100d9b"));
         assert(genesis.hashMerkleRoot == uint256("0x67a8d3310ad55d31172e813894733b947de8c5b17dbfc309f7ad07a66ce06e16"));

         vSeeds.push_back(CDNSSeedData("Seed1", "seed1.kydcoin.io"));
         vSeeds.push_back(CDNSSeedData("Seed2", "seed2.kydcoin.io"));
         vSeeds.push_back(CDNSSeedData("Seed3", "seed3.kydcoin.io"));
         vSeeds.push_back(CDNSSeedData("Seed4", "seed4.kydcoin.io"));

         base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 78);
         base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 85);
         base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 153);
         base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x02)(0x2D)(0x25)(0x33).convert_to_container<std::vector<unsigned char> >();
         base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x02)(0x21)(0x31)(0x2B).convert_to_container<std::vector<unsigned char> >();
         //  BIP44 coin type is from https://github.com/satoshilabs/slips/blob/master/slip-0044.md
         base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x00)(0x77).convert_to_container<std::vector<unsigned char> >();

         convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
        strSporkKey = "04aab04c0811c0a0ceec6f15174c83c351c6bc2fa89f633f11ee6c4e0d5afb2c41718acf73145dcdbd12271e6a1c5fc8a1e6e242d86e3ae257ddf0831fd090231b";
        strSporkKeyOld = "03e4668cc19a96b25e35a025387c0796ee84d81770b3bba3a1deffab6e22715b83";
        strObfuscationPoolDummyAddress = "YKcUbM3ryKwfdjbd33FnAYFspaxDp4vWLs";
        nStartMasternodePayments = 1523435473; //Wed, 25 Jun 2014 20:36:16 GMT

        /** Zerocoin */
        zerocoinModulus = "25195908475657893494027183240048398571429282126204032027777137836043662020707595556264018525880784"
            "4069182906412495150821892985591491761845028084891200728449926873928072877767359714183472702618963750149718246911"
            "6507761337985909570009733045974880842840179742910064245869181719511874612151517265463228221686998754918242243363"
            "7259085141865462043576798423387184774447920739934236584823824281198163815010674810451660377306056201619676256133"
            "8441436038339044149526344321901146575444541784240209246165157233507787077498171257724679629263863563732899121548"
            "31438167899885040445364023527381951378636564391212010397122822120720357";
        nMaxZerocoinSpendsPerTransaction = 7; // Assume about 20kb each
        nMinZerocoinMintFee = 1 * CENT; //high fee required for zerocoin mints
        nMintRequiredConfirmations = 20; //the maximum amount of confirmations until accumulated in 19
        nRequiredAccumulation = 1;
        nDefaultSecurityLevel = 100; //full security level for accumulators
        nZerocoinHeaderVersion = 4; //Block headers must be this version once zerocoin is active
        nZerocoinRequiredStakeDepth = 200; //The required confirmations for a zkyd to be stakable

        nBudget_Fee_Confirmations = 6; // Number of confirmations for the finalization fee
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
 class CTestNetParams : public CMainParams
 {
 public:
     CTestNetParams()
     {
         networkID = CBaseChainParams::TESTNET;
         strNetworkID = "test";
         pchMessageStart[0] = 0x53;
         pchMessageStart[1] = 0x64;
         pchMessageStart[2] = 0x75;
         pchMessageStart[3] = 0x86;
         vAlertPubKey = ParseHex("04207f3e4229bdd4f408e77918571292e149c52b3ff92ef0ec55c5c4e34ae633c331e24e6e70173b03c18549284e29eea43f74d5b32eba631a5d0c277486103c66");
         nDefaultPort = 22244;
         nEnforceBlockUpgradeMajority = 51;
         nRejectBlockOutdatedMajority = 75;
         nToCheckBlockUpgradeMajority = 100;
         nMinerThreads = 0;
         nTargetTimespan = 1 * 60; // KYD: 1 day
         nTargetSpacing = 1 * 60;  // KYD: 1 minute
         nLastPOWBlock = 200;
         nMaturity = 60;
         nMasternodeCountDrift = 4;
         nModifierUpdateBlock = 51197; //approx Mon, 17 Apr 2017 04:00:00 GMT
         nMaxMoneyOut = 43199500 * COIN;
         nZerocoinStartHeight = INT_MAX-1; //Never Activated
         nZerocoinStartTime = INT_MAX-1; //Never Activated
         nBlockEnforceSerialRange = 1; //Enforce serial range starting this block
         nBlockRecalculateAccumulators = 9908000; //Trigger a recalculation of accumulators
         nBlockFirstFraudulent = 9891737; //First block that bad serials emerged
         nBlockLastGoodCheckpoint = 9891730; //Last valid accumulator checkpoint
         nBlockEnforceInvalidUTXO = 9902850; //Start enforcing the invalid UTXO's
         nInvalidAmountFiltered = 0; //Amount of invalid coins filtered through exchanges, that should be considered valid
         nBlockZerocoinV2 = 444020; //!> The block that zerocoin v2 becomes active
         nEnforceNewSporkKey = 1521604800; //!> Sporks signed after Wednesday, March 21, 2018 4:00:00 AM GMT must use the new spork key
         nRejectOldSporkKey = 1522454400; //!> Reject old spork key after Saturday, March 31, 2018 12:00:00 AM GMT

         //! Modify the testnet genesis block so the timestamp is valid for a later start.
         genesis.nTime = 1523855288;
         genesis.nNonce = 2733486;

         hashGenesisBlock = genesis.GetHash();
         // assert(hashGenesisBlock == uint256("0x0000040df09b15ba874400ba995f342b82573864b9ee10c255dc4448ce334438"));

         vFixedSeeds.clear();
         vSeeds.clear();

         base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 140); // Testnet kyd addresses start with 'x' or 'y'
         base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 29);  // Testnet kyd script addresses start with '8' or '9'
         base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 240);     // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
         // Testnet kyd BIP32 pubkeys start with 'DRKV'
         base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x3a)(0x80)(0x61)(0xa0).convert_to_container<std::vector<unsigned char> >();
         // Testnet kyd BIP32 prvkeys start with 'DRKP'
         base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x3a)(0x80)(0x58)(0x37).convert_to_container<std::vector<unsigned char> >();
         // Testnet kyd BIP44 coin type is '1' (All coin's testnet default)
         base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x00)(0x01).convert_to_container<std::vector<unsigned char> >();

         convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

         fMiningRequiresPeers = true;
         fAllowMinDifficultyBlocks = true;
         fDefaultConsistencyChecks = false;
         fRequireStandard = true;
         fMineBlocksOnDemand = false;
         fTestnetToBeDeprecatedFieldRPC = true;

         nPoolMaxTransactions = 2;
         strSporkKey = "0467b0816d0d1575a98c6c6cf0b4a300f978c090119d7986e2b4798cc457aab21bb15104230de3c256401bc4f04f7a00012c107c227555386ccfb9a4db081cf97d";
         strSporkKeyOld = "0467b0816d0d1575a98c6c6cf0b4a300f978c090119d7986e2b4798cc457aab21bb15104230de3c256401bc4f04f7a00012c107c227555386ccfb9a4db081cf97d";
         strObfuscationPoolDummyAddress = "";
         nStartMasternodePayments = 1420837558; //Fri, 09 Jan 2015 21:05:58 GMT
         nBudget_Fee_Confirmations = 3; // Number of confirmations for the finalization fee. We have to make this very short
                                        // here because we only have a 8 block finalization window on testnet
     }
     const Checkpoints::CCheckpointData& Checkpoints() const
     {
         return dataTestnet;
     }
 };
 static CTestNetParams testNetParams;

 /**
  * Regression test
  */
 class CRegTestParams : public CTestNetParams
 {
 public:
     CRegTestParams()
     {
         networkID = CBaseChainParams::REGTEST;
         strNetworkID = "regtest";
         strNetworkID = "regtest";
         pchMessageStart[0] = 0x14;
         pchMessageStart[1] = 0x54;
         pchMessageStart[2] = 0x95;
         pchMessageStart[3] = 0x64;
         nSubsidyHalvingInterval = 150;
         nEnforceBlockUpgradeMajority = 750;
         nRejectBlockOutdatedMajority = 950;
         nToCheckBlockUpgradeMajority = 1000;
         nMinerThreads = 1;
         nTargetTimespan = 24 * 60 * 60; // KYD: 1 day
         nTargetSpacing = 1 * 60;        // KYD: 1 minutes
         bnProofOfWorkLimit = ~uint256(0) >> 1;
         genesis.nTime = 1523854631;
         genesis.nBits = bnProofOfWorkLimit.GetCompact();
         genesis.nNonce = 7065194;

         hashGenesisBlock = genesis.GetHash();
         nDefaultPort = 32244;
         //assert(hashGenesisBlock == uint256("0x5f66c55fd2f344b7e18547b69a6520aa5f3eb46f5d4e77444bbf0e93c60d7526"));

         vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
         vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

         fMiningRequiresPeers = false;
         fAllowMinDifficultyBlocks = true;
         fDefaultConsistencyChecks = true;
         fRequireStandard = false;
         fMineBlocksOnDemand = true;
         fTestnetToBeDeprecatedFieldRPC = false;
     }
     const Checkpoints::CCheckpointData& Checkpoints() const
     {
         return dataRegtest;
     }
 };
 static CRegTestParams regTestParams;

 /**
  * Unit test
  */
 class CUnitTestParams : public CMainParams, public CModifiableParams
 {
 public:
     CUnitTestParams()
     {
         networkID = CBaseChainParams::UNITTEST;
         strNetworkID = "unittest";
         nDefaultPort = 30222;
         vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
         vSeeds.clear();      //! Unit test mode doesn't have any DNS seeds.

         fMiningRequiresPeers = false;
         fDefaultConsistencyChecks = true;
         fAllowMinDifficultyBlocks = false;
         fMineBlocksOnDemand = true;
     }

     const Checkpoints::CCheckpointData& Checkpoints() const
     {
         // UnitTest share the same checkpoints as MAIN
         return data;
     }

     //! Published setters to allow changing values in unit test cases
     virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval) { nSubsidyHalvingInterval = anSubsidyHalvingInterval; }
     virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority) { nEnforceBlockUpgradeMajority = anEnforceBlockUpgradeMajority; }
     virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) { nRejectBlockOutdatedMajority = anRejectBlockOutdatedMajority; }
     virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) { nToCheckBlockUpgradeMajority = anToCheckBlockUpgradeMajority; }
     virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks) { fDefaultConsistencyChecks = afDefaultConsistencyChecks; }
     virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) { fAllowMinDifficultyBlocks = afAllowMinDifficultyBlocks; }
     virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
 };
 static CUnitTestParams unitTestParams;


 static CChainParams* pCurrentParams = 0;

 CModifiableParams* ModifiableParams()
 {
     assert(pCurrentParams);
     assert(pCurrentParams == &unitTestParams);
     return (CModifiableParams*)&unitTestParams;
 }

 const CChainParams& Params()
 {
     assert(pCurrentParams);
     return *pCurrentParams;
 }

 CChainParams& Params(CBaseChainParams::Network network)
 {
     switch (network) {
     case CBaseChainParams::MAIN:
         return mainParams;
     case CBaseChainParams::TESTNET:
         return testNetParams;
     case CBaseChainParams::REGTEST:
         return regTestParams;
     case CBaseChainParams::UNITTEST:
         return unitTestParams;
     default:
         assert(false && "Unimplemented network");
         return mainParams;
     }
 }

 void SelectParams(CBaseChainParams::Network network)
 {
     SelectBaseParams(network);
     pCurrentParams = &Params(network);
 }

 bool SelectParamsFromCommandLine()
 {
     CBaseChainParams::Network network = NetworkIdFromCommandLine();
     if (network == CBaseChainParams::MAX_NETWORK_TYPES)
         return false;

     SelectParams(network);
     return true;
 }
