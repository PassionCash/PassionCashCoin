// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2020 The Passion developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-payments.h"
#include "addrman.h"
#include "chainparams.h"
#include "fs.h"
#include "budget/budgetmanager.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "netmessagemaker.h"
#include "net_processing.h"
#include "spork.h"
#include "sync.h"
#include "util.h"
#include "utilmoneystr.h"


/** Object for who's going to get paid on which blocks */
CMasternodePayments masternodePayments;

RecursiveMutex cs_vecPayments;
RecursiveMutex cs_mapMasternodeBlocks;
RecursiveMutex cs_mapMasternodePayeeVotes;

static const int MNPAYMENTS_DB_VERSION = 1;

//
// CMasternodePaymentDB
//

CMasternodePaymentDB::CMasternodePaymentDB()
{
    pathDB = GetDataDir() / "mnpayments.dat";
    strMagicMessage = "MasternodePayments";
}

bool CMasternodePaymentDB::Write(const CMasternodePayments& objToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ssObj << MNPAYMENTS_DB_VERSION;
    ssObj << strMagicMessage;                   // masternode cache file specific magic message
    ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
    ssObj << objToSave;
    uint256 hash = Hash(ssObj.begin(), ssObj.end());
    ssObj << hash;

    // open output file, and associate with CAutoFile
    FILE* file = fsbridge::fopen(pathDB, "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathDB.string());

    // Write and commit header, data
    try {
        fileout << ssObj;
    } catch (const std::exception& e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    fileout.fclose();

    LogPrint(BCLog::MASTERNODE,"Written info to mnpayments.dat  %dms\n", GetTimeMillis() - nStart);

    return true;
}

CMasternodePaymentDB::ReadResult CMasternodePaymentDB::Read(CMasternodePayments& objToLoad)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE* file = fsbridge::fopen(pathDB, "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull()) {
        error("%s : Failed to open file %s", __func__, pathDB.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = fs::file_size(pathDB);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    std::vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char*)&vchData[0], dataSize);
        filein >> hashIn;
    } catch (const std::exception& e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssObj(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssObj.begin(), ssObj.end());
    if (hashIn != hashTmp) {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    int version;
    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header
        ssObj >> version;
        ssObj >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp) {
            error("%s : Invalid masternode payement cache magic message", __func__);
            return IncorrectMagicMessage;
        }


        // de-serialize file header (network specific magic number) and ..
        ssObj >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp))) {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize data into CMasternodePayments object
        ssObj >> objToLoad;
    } catch (const std::exception& e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrint(BCLog::MASTERNODE,"Loaded info from mnpayments.dat (dbversion=%d) %dms\n", version, GetTimeMillis() - nStart);
    LogPrint(BCLog::MASTERNODE,"  %s\n", objToLoad.ToString());

    return Ok;
}

uint256 CMasternodePaymentWinner::GetHash() const
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << std::vector<unsigned char>(payee.begin(), payee.end());
    ss << nBlockHeight;
    ss << vinMasternode.prevout;
    return ss.GetHash();
}

std::string CMasternodePaymentWinner::GetStrMessage() const
{
    return vinMasternode.prevout.ToStringShort() + std::to_string(nBlockHeight) + HexStr(payee);
}

bool CMasternodePaymentWinner::IsValid(CNode* pnode, std::string& strError)
{
    CMasternode* pmn = mnodeman.Find(vinMasternode.prevout);

    if (!pmn) {
        strError = strprintf("Unknown Masternode %s", vinMasternode.prevout.hash.ToString());
        LogPrint(BCLog::MASTERNODE,"CMasternodePaymentWinner::IsValid - %s\n", strError);
        mnodeman.AskForMN(pnode, vinMasternode);
        return false;
    }

    if (pmn->protocolVersion < ActiveProtocol()) {
        strError = strprintf("Masternode protocol too old %d - req %d", pmn->protocolVersion, ActiveProtocol());
        LogPrint(BCLog::MASTERNODE,"CMasternodePaymentWinner::IsValid - %s\n", strError);
        return false;
    }

    int n = mnodeman.GetMasternodeRank(vinMasternode, nBlockHeight - 100, ActiveProtocol());

    if (n > MNPAYMENTS_SIGNATURES_TOTAL) {
        //It's common to have masternodes mistakenly think they are in the top 10
        // We don't want to print all of these messages, or punish them unless they're way off
        if (n > MNPAYMENTS_SIGNATURES_TOTAL * 2) {
            strError = strprintf("Masternode not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL * 2, n);
            LogPrint(BCLog::MASTERNODE,"CMasternodePaymentWinner::IsValid - %s\n", strError);
            //if (masternodeSync.IsSynced()) Misbehaving(pnode->GetId(), 20);
        }
        return false;
    }

    return true;
}

void CMasternodePaymentWinner::Relay()
{
    CInv inv(MSG_MASTERNODE_WINNER, GetHash());
    g_connman->RelayInv(inv);
}

void DumpMasternodePayments()
{
    int64_t nStart = GetTimeMillis();

    CMasternodePaymentDB paymentdb;
    LogPrint(BCLog::MASTERNODE,"Writing info to mnpayments.dat...\n");
    paymentdb.Write(masternodePayments);

    LogPrint(BCLog::MASTERNODE,"Budget dump finished  %dms\n", GetTimeMillis() - nStart);
}

bool IsBlockValueValid(int nHeight, CAmount& nExpectedValue, CAmount nMinted)
{
    if (!masternodeSync.IsSynced()) {
        //there is no budget data to use to check anything
        //super blocks will always be on these blocks, max 100 per budgeting
        if (nHeight % Params().GetConsensus().nBudgetCycleBlocks < 100) {
            if (Params().IsTestnet()) {
                return true;
            }
            nExpectedValue += g_budgetman.GetTotalBudget(nHeight);
        }
    } else {
        // we're synced and have data so check the budget schedule
        // if the superblock spork is enabled
        if (sporkManager.IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)) {
            // add current payee amount to the expected block value
            CAmount expectedPayAmount;
            if (g_budgetman.GetExpectedPayeeAmount(nHeight, expectedPayAmount)) {
                nExpectedValue += expectedPayAmount;
            }
        }
    }

    return nMinted <= nExpectedValue;
}

bool IsBlockPayeeValid(const CBlock& block, int nBlockHeight)
{
    TrxValidationStatus transactionStatus = TrxValidationStatus::InValid;

    if (!masternodeSync.IsSynced()) { //there is no budget data to use to check anything -- find the longest chain
        LogPrint(BCLog::MASTERNODE, "Client not synced, skipping block payee checks\n");
        return true;
    }

    const bool isPoSActive = Params().GetConsensus().NetworkUpgradeActive(nBlockHeight, Consensus::UPGRADE_POS);
    const CTransaction& txNew = *(isPoSActive ? block.vtx[1] : block.vtx[0]);

    //check if it's a budget block
    if (sporkManager.IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)) {
        if (g_budgetman.IsBudgetPaymentBlock(nBlockHeight)) {
            transactionStatus = g_budgetman.IsTransactionValid(txNew, block.GetHash(), nBlockHeight);
            if (transactionStatus == TrxValidationStatus::Valid) {
                return true;
            }

            if (transactionStatus == TrxValidationStatus::InValid) {
                LogPrint(BCLog::MASTERNODE,"Invalid budget payment detected %s\n", txNew.ToString().c_str());
                if (sporkManager.IsSporkActive(SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT))
                    return false;

                LogPrint(BCLog::MASTERNODE,"Budget enforcement is disabled, accepting block\n");
            }
        }
    }

    // If we end here the transaction was either TrxValidationStatus::InValid and Budget enforcement is disabled, or
    // a double budget payment (status = TrxValidationStatus::DoublePayment) was detected, or no/not enough masternode
    // votes (status = TrxValidationStatus::VoteThreshold) for a finalized budget were found
    // In all cases a masternode will get the payment for this block

    //check for masternode payee
    if (masternodePayments.IsTransactionValid(txNew, nBlockHeight))
        return true;
    LogPrint(BCLog::MASTERNODE,"Invalid mn payment detected %s\n", txNew.ToString().c_str());

    if (sporkManager.IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT))
        return false;
    LogPrint(BCLog::MASTERNODE,"Masternode payment enforcement is disabled, accepting block\n");
    return true;
}


void FillBlockPayee(CMutableTransaction& txNew, const int nHeight, bool fProofOfStake)
{
    if (nHeight == 0) return;
    if (!sporkManager.IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) ||           // if superblocks are not enabled
            !g_budgetman.FillBlockPayee(txNew, nHeight, fProofOfStake) ) {    // or this is not a superblock,
        // ... or there's no budget with enough votes, then pay a masternode
        masternodePayments.FillBlockPayee(txNew, nHeight, fProofOfStake);
    }
}

std::string GetRequiredPaymentsString(int nBlockHeight)
{
    if (sporkManager.IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && g_budgetman.IsBudgetPaymentBlock(nBlockHeight)) {
        return g_budgetman.GetRequiredPaymentsString(nBlockHeight);
    } else {
        return masternodePayments.GetRequiredPaymentsString(nBlockHeight);
    }
}

void CMasternodePayments::FillBlockPayee(CMutableTransaction& txNew, const int nHeight, bool fProofOfStake)
{
    if (nHeight == 0) return;

    bool hasPayment = true;
    bool bDevFee = sporkManager.IsSporkActive(SPORK_30_DEVELOPMENT_FEE);
    CScript payee;
    CAmount mn_payments_total = 0;
    CAmount devFeeAmount = 0;
    CAmount blockValue = GetBlockValue(nHeight);
    if(bDevFee) {
        devFeeAmount = (blockValue * Params().GetConsensus().nDevFee) / 100;
    }

    int mnTransactionCount = 0;
    for(int mnlevel = CMasternode::LevelValue::MIN; mnlevel <= CMasternode::LevelValue::MAX; ++mnlevel) {
            //spork
            if (!masternodePayments.GetBlockPayee(nHeight + 1, mnlevel, payee)) {
                //no masternode detected
                const CMasternode* winningNode = mnodeman.GetCurrentMasterNode(mnlevel, 1);
                if (winningNode) {
                    payee = GetScriptForDestination(winningNode->pubKeyCollateralAddress.GetID());                    
                    hasPayment = true;

                } else {
                    LogPrint(BCLog::MASTERNODE,"CreateNewBlock: Failed to detect masternode to pay\n");
                    hasPayment = false;
                }
            }
            if (hasPayment) {
                CAmount masternodePayment = GetMasternodePayment(mnlevel, blockValue);
                if (fProofOfStake) {
                    /**For Proof Of Stake vout[0] must be null
                     * Stake reward can be split into many different outputs, so we must
                     * use vout.size() to align with several different cases.
                     * An additional output is appended as the masternode payment
                     */
                    txNew.vout.emplace_back(masternodePayment,payee);
                    mn_payments_total += masternodePayment;
                    mnTransactionCount++;
                }
            }
        }
        if (fProofOfStake) {
            //subtract mn payment from the stake reward
            if (!txNew.vout[1].IsZerocoinMint()) {
                unsigned int i = txNew.vout.size();
                //LogPrintf("MN Transactioncount = %i\n",mnTransactionCount);
                if (i == (mnTransactionCount + 2)) {
                    // Majority of cases; do it quick and move on
                    txNew.vout[i - (mnTransactionCount + 1)].nValue -= mn_payments_total + devFeeAmount;
                } else if (i > (mnTransactionCount + 2)) {
                    // special case, stake is split between (i-1) outputs
                    unsigned int outputs = i-(mnTransactionCount + 1);
                    CAmount mnPaymentSplit = (mn_payments_total+devFeeAmount) / outputs;
                    CAmount mnPaymentRemainder = (mn_payments_total+devFeeAmount) - (mnPaymentSplit * outputs);
                    for (unsigned int j=1; j<=outputs; j++) {
                        txNew.vout[j].nValue -= mnPaymentSplit;
                    }
                    // in case it's not an even division, take the last bit of dust from the last one
                    txNew.vout[outputs].nValue -= mnPaymentRemainder;
                }
                if(bDevFee) {
                    CTxDestination devAddr = DecodeDestination(Params().GetConsensus().strDevFeeAddress);
                    CScript devpayee = GetScriptForDestination(devAddr);
                    txNew.vout.emplace_back(devFeeAmount,devpayee);
                }
            }
        } else { 
            //Add a new vout to the transaction
            txNew.vout.emplace_back(mn_payments_total,payee);        
            txNew.vout[0].nValue = blockValue - mn_payments_total - devFeeAmount;
            if(bDevFee) {
                CTxDestination devAddr = DecodeDestination(Params().GetConsensus().strDevFeeAddress);
                CScript devpayee = GetScriptForDestination(devAddr);
                txNew.vout.emplace_back(devFeeAmount,devpayee);
            }
        }
}

void CMasternodePayments::ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (!masternodeSync.IsBlockchainSynced()) return;

    if (fLiteMode) return; //disable all Masternode related functionality


    if (strCommand == NetMsgType::GETMNWINNERS) { //Masternode Payments Request Sync
        if (fLiteMode) return;   //disable all Masternode related functionality

        int nCountNeeded;
        vRecv >> nCountNeeded;

        if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
            if (pfrom->HasFulfilledRequest(NetMsgType::GETMNWINNERS)) {
                LogPrintf("CMasternodePayments::ProcessMessageMasternodePayments() : mnget - peer already asked me for the list\n");
                LOCK(cs_main);
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
        }

        pfrom->FulfilledRequest(NetMsgType::GETMNWINNERS);
        masternodePayments.Sync(pfrom, nCountNeeded);
        LogPrint(BCLog::MASTERNODE, "mnget - Sent Masternode winners to peer %i\n", pfrom->GetId());
    } else if (strCommand == NetMsgType::MNWINNER) { //Masternode Payments Declare Winner
        //this is required in litemodef
        CMasternodePaymentWinner winner;
        vRecv >> winner;

        if (pfrom->nVersion < ActiveProtocol()) return;

        int nHeight = mnodeman.GetBestHeight();

        if (masternodePayments.mapMasternodePayeeVotes.count(winner.GetHash())) {
            LogPrint(BCLog::MASTERNODE, "mnw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString().c_str(), nHeight);
            masternodeSync.AddedMasternodeWinner(winner.GetHash());
            return;
        }

        int nFirstBlock = nHeight - (mnodeman.CountEnabled() * 1.25);
        if (winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > nHeight + 20) {
            LogPrint(BCLog::MASTERNODE, "mnw - winner out of range - FirstBlock %d Height %d bestHeight %d\n", nFirstBlock, winner.nBlockHeight, nHeight);
            return;
        }

        // reject old signature version
        if (winner.nMessVersion != MessageVersion::MESS_VER_HASH) {
            LogPrint(BCLog::MASTERNODE, "mnw - rejecting old message version %d\n", winner.nMessVersion);
            return;
        }

        std::string strError = "";
        if (!winner.IsValid(pfrom, strError)) {
            // if(strError != "") LogPrint(BCLog::MASTERNODE,"mnw - invalid message - %s\n", strError);
            return;
        }

        if (!masternodePayments.CanVote(winner.vinMasternode.prevout, winner.nBlockHeight)) {
            //  LogPrint(BCLog::MASTERNODE,"mnw - masternode already voted - %s\n", winner.vinMasternode.prevout.ToStringShort());
            return;
        }

        const CMasternode* pmn = mnodeman.Find(winner.vinMasternode.prevout);
        if (!pmn || !winner.CheckSignature(pmn->pubKeyMasternode.GetID())) {
            if (masternodeSync.IsSynced()) {
                LogPrintf("CMasternodePayments::ProcessMessageMasternodePayments() : mnw - invalid signature\n");
                LOCK(cs_main);
                Misbehaving(pfrom->GetId(), 20);
            }
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, winner.vinMasternode);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);

        //   LogPrint(BCLog::MASTERNODE, "mnw - winning vote - Addr %s Height %d bestHeight %d - %s\n", address2.ToString().c_str(), winner.nBlockHeight, nHeight, winner.vinMasternode.prevout.ToStringShort());

        if (masternodePayments.AddWinningMasternode(winner)) {
            winner.Relay();
            masternodeSync.AddedMasternodeWinner(winner.GetHash());
        }
    }
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, int mnlevel, CScript& payee)
{
    if (mapMasternodeBlocks.count(nBlockHeight)) {
        return mapMasternodeBlocks[nBlockHeight].GetPayee(mnlevel, payee);
    }

    return false;
}

// Is this masternode scheduled to get paid soon?
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CMasternodePayments::IsScheduled(const CMasternode& mn,int nSameLevelMNCount, int mnlevel, int nNotBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    int nHeight = mnodeman.GetBestHeight();

    CScript mnpayee;
    mnpayee = GetScriptForDestination(mn.pubKeyCollateralAddress.GetID());

    CScript payee;
    for (int64_t h = nHeight; h <= nHeight + 8; h++) {
        if (h == nNotBlockHeight) continue;
        if (mapMasternodeBlocks.count(h)) {
            if (mapMasternodeBlocks[h].GetPayee(mnlevel,payee)) {
                if (mnpayee == payee) {
                    return true;
                }
            }
        }
    }
    int64_t h_upper_bound = nHeight + 10;
    int64_t h = h_upper_bound - std::min(10, nSameLevelMNCount - 1);
    for( ; h < h_upper_bound; ++h) {
        if(h == nNotBlockHeight) 
            continue;
        auto block_payees = mapMasternodeBlocks.find(h);
        if(block_payees == mapMasternodeBlocks.cend()) 
            continue;
        CScript payee;
        if(!block_payees->second.GetPayee(mnlevel, payee))
            continue;
        if(mnpayee == payee)
            return true;
    }

    return false;
}

bool CMasternodePayments::AddWinningMasternode(CMasternodePaymentWinner& winnerIn)
{
    // check winner height
    if (winnerIn.nBlockHeight - 100 > mnodeman.GetBestHeight() + 1) {
        return false;
    }

    {
        LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);

        if (mapMasternodePayeeVotes.count(winnerIn.GetHash())) {
            return false;
        }

        mapMasternodePayeeVotes[winnerIn.GetHash()] = winnerIn;

        if (!mapMasternodeBlocks.count(winnerIn.nBlockHeight)) {
            CMasternodeBlockPayees blockPayees(winnerIn.nBlockHeight);
            mapMasternodeBlocks[winnerIn.nBlockHeight] = blockPayees;
        }
    }

    mapMasternodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee, winnerIn.payeeLevel, 1);

    return true;
}

bool CMasternodeBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    LOCK(cs_vecPayments);
    CAmount nReward = GetBlockValue(nBlockHeight - 1);
    if(sporkManager.IsSporkActive(SPORK_30_DEVELOPMENT_FEE)) {
        CAmount nReward = GetBlockValue(nBlockHeight - 1);
        const Consensus::Params& consensus = Params().GetConsensus();
        bool bDevFeeFound = false;
        CAmount devFeeFund = nReward * consensus.nDevFee / 100;
        CTxDestination Dest;
        for (const CTxOut& out : txNew.vout) {
            CTxDestination DevFundDestination;
            ExtractDestination(out.scriptPubKey, DevFundDestination);
            if (consensus.strDevFeeAddress == EncodeDestination(DevFundDestination).c_str()) {
                if(out.nValue != devFeeFund) 
                    break;
                bDevFeeFound = true;
            }
        }
        if (!bDevFeeFound)
            return error("Payment for Passiondevelopment (DEV-FEE) not found\n");
    }
        //MulitMN
        std::map<int, int> max_signatures;
        for(CMasternodePayee& payee : vecPayments) {
            if(payee.nVotes < MNPAYMENTS_SIGNATURES_REQUIRED)
                continue;
            auto ins_res = max_signatures.emplace(payee.mnlevel, payee.nVotes);
            if(ins_res.second)
                continue;
            if(payee.nVotes >= ins_res.first->second)
                ins_res.first->second = payee.nVotes;
        }
        // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
        if (max_signatures.size() < MNPAYMENTS_SIGNATURES_REQUIRED) return true;

        std::string strPayeesPossible;
        for(const CMasternodePayee& payee : vecPayments) {
            // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
            if(payee.nVotes < MNPAYMENTS_SIGNATURES_REQUIRED)
                continue;

            auto requiredMasternodePayment = GetMasternodePayment(payee.mnlevel, nReward);
            auto payee_out = std::find_if(txNew.vout.cbegin(), txNew.vout.cend(), [&payee, &requiredMasternodePayment](const CTxOut& out){

                auto is_payee          = payee.scriptPubKey == out.scriptPubKey;
                auto is_value_required = out.nValue >= requiredMasternodePayment;

                if(is_payee && !is_value_required)
                    LogPrintf("Masternode payment is out of drift range. Paid=%s Min=%s\n", FormatMoney(out.nValue).c_str(), FormatMoney(requiredMasternodePayment).c_str());

                return is_payee && is_value_required;
            });

            if(payee_out != txNew.vout.cend()) {
                max_signatures.erase(payee.mnlevel);
                if(max_signatures.size())
                    continue;
                return true;
            }
            
            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            auto address2 = std::to_string(payee.mnlevel) + ":" + EncodeDestination(address1).c_str();

            if(strPayeesPossible == "")
                strPayeesPossible += address2;
            else
                strPayeesPossible += "," + address2;
            
        }
        LogPrintf("CMasternodePayments::IsTransactionValid - Missing required payment to %s\n", strPayeesPossible.c_str());
        return false;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString()
{
    LOCK(cs_vecPayments);

    std::string ret = "Unknown";

    for (CMasternodePayee& payee : vecPayments) {
        CTxDestination address1;
        ExtractDestination(payee.scriptPubKey, address1);
        if (ret != "Unknown") {
            ret += ", " + EncodeDestination(address1) + ":" + std::to_string(payee.mnlevel) + ":" + std::to_string(payee.nVotes);;
        }
        ret = EncodeDestination(address1) + ":" + std::to_string(payee.mnlevel) + ":" + std::to_string(payee.nVotes);
    }
    return ret;
}

std::string CMasternodePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if (mapMasternodeBlocks.count(nBlockHeight)) {
        return mapMasternodeBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if (mapMasternodeBlocks.count(nBlockHeight)) {
        return mapMasternodeBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

void CMasternodePayments::CleanPaymentList(int mnCount, int nHeight)
{
    LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);

    //keep up to five cycles for historical sake
    int nLimit = std::max(int(mnCount * 1.25), 1000);

    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while (it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;

        if (nHeight - winner.nBlockHeight > nLimit) {
            LogPrint(BCLog::MASTERNODE, "CMasternodePayments::CleanPaymentList - Removing old Masternode payment - block %d\n", winner.nBlockHeight);
            masternodeSync.mapSeenSyncMNW.erase((*it).first);
            mapMasternodePayeeVotes.erase(it++);
            mapMasternodeBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight)
{
    if (!fMasterNode) return false;

    if (activeMasternode.vin == nullopt)
        return error("%s: Active Masternode not initialized.", __func__);

    //reference node - hybrid mode
    int n = mnodeman.GetMasternodeRank(*(activeMasternode.vin), nBlockHeight - 100, ActiveProtocol());

    if (n == -1) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock - Unknown Masternode\n");
        return false;
    }

    if (n > MNPAYMENTS_SIGNATURES_TOTAL) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock - Masternode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
        return false;
    }
    int nWinnerBlockHeight = nBlockHeight + 10;
    if (nWinnerBlockHeight <= nLastBlockHeight) return false;
    
    CPubKey pubKeyMasternode; CKey keyMasternode;
    activeMasternode.GetKeys(keyMasternode, pubKeyMasternode);

    std::vector<CMasternodePaymentWinner> winners;
        for(unsigned mnlevel = CMasternode::LevelValue::MIN; mnlevel <= CMasternode::LevelValue::MAX; ++mnlevel) {
            int nCount = 0;
            auto pmn = mnodeman.GetNextMasternodeInQueueForPayment(nWinnerBlockHeight, mnlevel, true, nCount);

            if(!pmn) {
                LogPrintf("CMasternodePayments::ProcessBlock() Failed to find masternode level %d to pay \n", mnlevel);
                continue;
            }
            auto payee = GetScriptForDestination(pmn->pubKeyCollateralAddress.GetID());
            CMasternodePaymentWinner newWinner(*(activeMasternode.vin));
            newWinner.nBlockHeight = nWinnerBlockHeight;
            newWinner.AddPayee(payee, mnlevel);


            LogPrint(BCLog::MASTERNODE,"CMasternodePayments::ProcessBlock() - Signing Winner\n");
            if (newWinner.Sign(keyMasternode, pubKeyMasternode.GetID())) {
                LogPrint(BCLog::MASTERNODE,"CMasternodePayments::ProcessBlock() - AddWinningMasternode\n");
                if (AddWinningMasternode(newWinner)) {
                    winners.emplace_back(newWinner);
                }
            }
        }
        if(!winners.empty()) {
            for(auto& winner : winners) 
                winner.Relay();
            nLastBlockHeight = nBlockHeight;
            return true;
        }
}

void CMasternodePayments::Sync(CNode* node, int nCountNeeded)
{
    LOCK(cs_mapMasternodePayeeVotes);
    int nHeight = mnodeman.GetBestHeight();
      
        auto mn_counts = mnodeman.CountEnabledByLevels();
        int max_mn_count = 0;
        
        for(auto& count : mn_counts) {
            max_mn_count = std::max(max_mn_count, int(count.second * 1.25));
            count.second = int(count.second * 1.25) + 1;
        }
        if(max_mn_count > nCountNeeded) max_mn_count = nCountNeeded;

        int nInvCount = 0;
        std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
        while (it != mapMasternodePayeeVotes.end()) {
            CMasternodePaymentWinner winner = (*it).second;

            if (winner.nBlockHeight >= nHeight - nCountNeeded && winner.nBlockHeight <= nHeight + 20) {
                node->PushInventory(CInv(MSG_MASTERNODE_WINNER, winner.GetHash()));
                nInvCount++;
            }
            it++;
        }         
        std::map<int, CScript> example;
        for(const auto& vote : mapMasternodePayeeVotes) {
            const auto& winner = vote.second;
            if (winner.nBlockHeight <= nHeight) {
                if (winner.nBlockHeight < nHeight - mn_counts[winner.payeeLevel]){
                    LogPrintf("= skip winner at height: %d, level: %d\n", winner.nBlockHeight, winner.payeeLevel);
                    continue;
            }
            int nWinnerIndex = winner.nBlockHeight * winner.payeeLevel;
            std::map<int, CScript>::iterator it = example.find(nWinnerIndex);
            if (it == example.end()){
                example.insert({nWinnerIndex, winner.payee});
            }
            else if (it->second == winner.payee){
                LogPrintf("= payee already in map\n");
                continue;
            } else {
                it->second = winner.payee;
            }
        }
        g_connman->PushMessage(node, CNetMsgMaker(node->GetSendVersion()).Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_MNW, nInvCount));
        nInvCount++;
    }
    LogPrintf("=== total winners = %d\n", nInvCount);
} 

std::string CMasternodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapMasternodePayeeVotes.size() << ", Blocks: " << (int)mapMasternodeBlocks.size();

    return info.str();
}
