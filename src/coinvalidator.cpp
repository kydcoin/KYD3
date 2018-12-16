// Copyright (c) 2017 Michael Madgett
// Copyright (c) 2017 The Blocknet developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coinvalidator.h"
#include "chainparams.h"

#include <fstream>
#include "util.h"

/**
 * Returns true if the tx is not associated with any infractions.
 * @param txId
 * @return
 */
bool CoinValidator::IsCoinValid(const uint256 &txId) const {
    // A coin is valid if its tx is not in the infractions list
    boost::mutex::scoped_lock l(lock);
    return infMap.count(txId.ToString()) == 0;
}
bool CoinValidator::IsCoinValid(uint256 &txId) const {
    boost::mutex::scoped_lock l(lock);
    return infMap.count(txId.ToString()) == 0;
}

/**
 * Returns true if the exploited coin is being sent to the redeem address. This checks amounts against
 * the exploit db.
 * @param scripts
 * @return
 */
bool CoinValidator::RedeemAddressVerified(std::vector<RedeemData> &exploited,
                                          std::vector<RedeemData> &recipients) {
    boost::mutex::scoped_lock l(lock);
    if (recipients.empty())
        return false;

    static const std::string redeemAddress = "B7nPQHKmX8DPkBFaBtaNQWc9SxD3uYpYv6";
    std::set<std::string> explSeen;

    // Add up all exploited inputs by send from address
    CAmount totalExploited = 0;
    for (auto &expl : exploited) {
        if (!infMap.count(expl.txid)) // fail if infraction not found
            return false;

        // Get address of tx
        CTxDestination explDest;
        if (!ExtractDestination(expl.scriptPubKey, explDest))  // if bad destination then fail
            return false;
        CBitcoinAddress explAddress(explDest);
        std::string explAddr = explAddress.ToString();

        // If we've already added up infractions for this utxo address, skip
        std::string guid = expl.txid + "-" + explAddr;
        if (explSeen.count(guid))
            continue;

        // Find out how much exploited coin we need to spend in this utxo
        CAmount exploitedAmount = 0;
        std::vector<InfractionData> &infs = infMap[expl.txid];
        for (auto &inf : infs) {
            if (inf.address == explAddr)
                exploitedAmount += inf.amount;
        }

        // Add to total exploited
        totalExploited += exploitedAmount;

        // Mark that we've seen all infractions for this utxo address
        explSeen.insert(guid);
    }

    if (totalExploited == 0) // no exploited coin, return
        return true;

    // Add up total redeem amount
    CAmount totalRedeem = 0;
    for (auto &rec : recipients) {
        CTxDestination recipientDest;
        if (!ExtractDestination(rec.scriptPubKey, recipientDest)) // if bad recipient destination then fail
            return false;
        CBitcoinAddress recipientAddress(recipientDest);
        // If recipient address matches the redeem address count spend amount
        if (recipientAddress.ToString() == redeemAddress)
            totalRedeem += rec.amount;
    }

    // Allow spending inputs if the total redeem amount spent is greater than or equal to exploited amount
    bool success = totalRedeem >= totalExploited;
    if (!success && totalRedeem > 0)
        LogPrintf("Coin Validator: Failed to Redeem: minimum amount required for this transaction (not including network fee): %f BLOCK\n", (double)totalExploited/(double)COIN);
    return success;
}

/**
 * Returns true if the validator has loaded the hash into memory.
 * @return
 */
bool CoinValidator::IsLoaded() const {
    boost::mutex::scoped_lock l(lock);
    return infMapLoaded;
}

/**
 * Clears the hash from memory.
 * @return
 */
void CoinValidator::Clear() {
    boost::mutex::scoped_lock l(lock);
    infMap.clear();
    infMapLoaded = false;
}

/**
 * Get infractions for the specified criteria.
 * @return
 */
std::vector<InfractionData> CoinValidator::GetInfractions(const uint256 &txId) {
    boost::mutex::scoped_lock l(lock);
    return infMap[txId.ToString()];
}
std::vector<InfractionData> CoinValidator::GetInfractions(uint256 &txId) {
    boost::mutex::scoped_lock l(lock);
    return infMap[txId.ToString()];
}
std::vector<InfractionData> CoinValidator::GetInfractions(CBitcoinAddress &address) {
    boost::mutex::scoped_lock l(lock);
    std::vector<InfractionData> infs;
    for (auto &item : infMap) {
        for (const InfractionData &inf : item.second)
            if (inf.address == address.ToString())
                infs.push_back(inf);
    }
    return infs;
}

/**
 * Loads the infraction list from code.
 * @return
 */
bool CoinValidator::LoadStatic() {
    boost::mutex::scoped_lock l(lock);

    // Ignore if we've already loaded the list
    if (infMapLoaded)
        return false;
    infMapLoaded = true;

    // Clear old data
    infMap.clear();

    // Load infractions into memory
    std::vector<string> infractions = getExplList();
    for (std::string &line : infractions) {
        bool result = addLine(line, infMap);
        if (!result) {
            LogPrintf("Coin Validator: Failed to read infraction: %s\n", line);
            assert(result);
        }
    }

    return true;
}

/**
 * Adds the data to internal hash.
 * @return
 */
bool CoinValidator::addLine(std::string &line, std::map<std::string, std::vector<InfractionData>> &map) {
    std::stringstream os(line);
    os.imbue(std::locale::classic());
    std::string t;
    std::string a;
    CAmount amt = 0;
    double amtd = 0;
    os >> t >> a >> amt >> amtd;

    if (t.empty() || a.empty() || amt == 0 || amtd == 0)
        return false;

    // Make sure parsed line matches expected
    assert(line == t + "\t" + a + "\t" + std::to_string(amt) + "\t" + CoinValidator::AmountToString(amtd));

    const InfractionData inf(t, a, amt, amtd);
    std::vector<InfractionData> &infs = map[inf.txid];
    infs.push_back(inf);

    return true;
}

/**
 * Get block height from line.
 * @return
 */
int CoinValidator::getBlockHeight(std::string &line) {
    std::stringstream os(line);
    os.imbue(std::locale::classic());
    int t = 0; os >> t;
    if (t > 0)
        return t;
    else
        return 0;
}

/**
 * Return the string representation of the amount.
 * @param amount
 * @return
 */
std::string CoinValidator::AmountToString(double amount) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::fixed << amount;
    return os.str();
}

/**
 * Singleton
 * @return
 */
CoinValidator& CoinValidator::instance() {
    static CoinValidator validator;
    return validator;
}

/**
 * Prints the string representation of the infraction.
 * @return
 */
std::string InfractionData::ToString() const {
    return txid + "\t" + address + "\t" + std::to_string(amount) + "\t" + CoinValidator::AmountToString(amountH);
}

/**
 * Return the infraction list.
 * @return
 */
std::vector<string> CoinValidator::getExplList() {
    std::vector<string> r = {};
    return r;
}
