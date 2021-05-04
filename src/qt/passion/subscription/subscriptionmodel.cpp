//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/subscription/subscriptionmodel.h"
#include "qt/passion/subscription/subsite.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "activemasternode.h"
#include "sync.h"
#include "uint256.h"
#include "wallet/wallet.h"
#include "guiutil.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QString>
#include <QMapIterator>
#include <QDateTime>


SubscriptionModel::SubscriptionModel(QObject *parent) : QAbstractTableModel(parent){
    updateMNList();
}

void SubscriptionModel::updateMNList(){
    LogPrintf("Load List of Sites\n");
    int end = nodes.size();
    nodes.clear();
    collateralTxAccepted.clear();
    beginInsertRows(QModelIndex(), nodes.size(), nodes.size());
    #ifdef _WIN32
        QString path = QString::fromStdWString(GetDataDir().generic_wstring());
    #else
        QString path = QString::fromStdString(GetDataDir().native());
    #endif

    QFile siteFile(path + "/sites.conf");
    if (siteFile.open(QIODevice::ReadOnly))
    {
        //TODO SUBSCRIPTION ERROR HANDLING
       QTextStream in(&siteFile);
       //SubSite *site = nullptr;
       while (!in.atEnd())
       {
          QString line = in.readLine();
          if(!line.startsWith(QLatin1Char('#'))) {
              QStringList list2 = line.split(QLatin1Char(' '));
              SubSite *site = new SubSite();              
              site->setSiteName(list2[0]);
              site->setSiteDomain(list2[1]);
              site->setSiteKey(list2[2]);
              site->setSiteAddr(list2[3]);
              site->setSiteFee(list2[4].toInt());
              site->setSiteAddrRegister(list2[5]);
              site->setSiteAddrRegisterBalance(getAddressBalance(list2[5])); //getAddressBalance(list2[5])
              site->setSiteExpire(list2[6].toInt());
              site->setSiteState(list2[7].toInt());
              LogPrintf("SideName: %s", site->getSiteDomain().toStdString().c_str());
              nodes.insert(list2[0],site);
          }
       }
       siteFile.close();
    } else {
        LogPrintf("Failed to open file! %s\n", path.toStdString().c_str());
    }
    endInsertRows();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(end, 9, QModelIndex()) );
}

QString SubscriptionModel::getAddressBalance(QString address) {
    /*
    std::map<QString, std::vector<COutput>> mapCoins;
    if(wModel) {
        CAmount nSum = 0;
        this->wModel->listCoins(mapCoins);    
        for (PAIRTYPE(QString, std::vector<COutput>) coins : mapCoins) {
            QString sWalletAddress = coins.first;            
            if(sWalletAddress == address) {
                for(const COutput& out: coins.second) {
                    nSum += out.tx->vout[out.i].nValue;
                }
            }
        }
        return GUIUtil::formatBalance(nSum);
    }
    */
    CAmount test = 5*COIN;
    return GUIUtil::formatBalance(test);
}
int SubscriptionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return nodes.size();
}

int SubscriptionModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 9;
}

void SubscriptionModel::writeSitesToDisk() {
    // Create a new file
    #ifdef _WIN32
        QString path = QString::fromStdWString(GetDataDir().generic_wstring());
    #else
        QString path = QString::fromStdString(GetDataDir().native());
    #endif
    QFile siteFile(path + "/sites.conf");
    if (siteFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
       QTextStream fout(&siteFile);
       SubSite *site;
       QMapIterator<QString, SubSite*> i(nodes);
       while (i.hasNext()) {
           i.next();
           site = i.value();
           fout << i.key() <<  " " << site->getSiteDomain() <<
                                " " << site->getSiteKey() <<
                                " " << site->getSiteAddr() <<
                                " " << site->getSiteFee() <<
                                " " << site->getSiteAddrRegister() <<
                                " " << site->getSiteExpire() <<
                                " " << site->getSiteState() << '\n';

       }
       siteFile.close();
    } else {
        qDebug("Fehler beim öffnen der Datei");
    }
}


QVariant SubscriptionModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    int row = index.row();
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case NAME:
                return nodes.uniqueKeys().value(row);
                //return nodes.values().value(row)->getSiteName();
            case DOMAIN:
                return nodes.values().value(row)->getSiteDomain();
            case KEY:
                return nodes.values().value(row)->getSiteKey();
            case ADDRESS:
                return nodes.values().value(row)->getSiteAddr();
            case REGISTERADDRESS:
                return nodes.values().value(row)->getSiteAddrRegister();
            case REGISTERADDRESSBALANCE:
                return nodes.values().value(row)->getSiteAddrRegisterAmount();
            case SITEFEE:
                return nodes.values().value(row)->getSiteFee();
            case EXPIRE:
                return nodes.values().value(row)->getSiteExpire();
            case STATUS:
                return nodes.values().value(row)->getSiteState();
        }
    }

    return QVariant();
}
bool SubscriptionModel::setData(QModelIndex &modelIndex, const QVariant insert, int role)
{
    int idx = modelIndex.row();
    beginRemoveRows(QModelIndex(), idx, idx);
    int timestamp = QDateTime::currentSecsSinceEpoch();
    nodes.values().value(idx)->setSiteExpire(timestamp+3780);
    endRemoveRows();
    writeSitesToDisk();
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, 8, QModelIndex()) );
}
void SubscriptionModel::setWallet(WalletModel* w) {
    wModel = w;
}
bool SubscriptionModel::removeMn(const QModelIndex& modelIndex) {
    QString alias = modelIndex.data(Qt::DisplayRole).toString();
    int idx = modelIndex.row();
    beginRemoveRows(QModelIndex(), idx, idx);
    nodes.take(alias);
    endRemoveRows();
    writeSitesToDisk();
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, 8, QModelIndex()) );
    return true;
}

bool SubscriptionModel::addMn(const QString domain, const QString key, const QString addr, const QString registerAddr,int sitefee, int expire, int status){
    beginInsertRows(QModelIndex(), nodes.size(), nodes.size());
    QString name = GetRandomString();
    SubSite *site = new SubSite();
    site->setSiteName(name);
    site->setSiteDomain(domain);
    site->setSiteKey(key);
    site->setSiteAddr(addr);
    site->setSiteAddrRegister(registerAddr);
    site->setSiteFee(sitefee);
    site->setSiteAddrRegisterBalance("0"); //getAddressBalance
    site->setSiteExpire(expire);
    site->setSiteState(status);
    nodes.insert(name,site);
    endInsertRows();
    writeSitesToDisk();
    return true;
}

QString SubscriptionModel::GetRandomString()
{
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   const int randomStringLength = 12; // assuming you want random strings of 12 characters

   QString randomString;
   for(int i=0; i<randomStringLength; ++i)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}
