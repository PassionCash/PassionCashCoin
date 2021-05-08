//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SUBSCRIPTIONMODEL_H
#define SUBSCRIPTIONMODEL_H

#include <QAbstractTableModel>
#include "masternode.h"
#include "qt/passion/subscription/subsite.h"
#include "masternodeconfig.h"
#include "qt/walletmodel.h"
#include "init.h"
#include <QString>

class WalletModel;

class SubscriptionModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SubscriptionModel(QObject *parent = nullptr);
    ~SubscriptionModel() override {
        nodes.clear();
    }

    enum ColumnIndex {
        NAME = 0,
	    DOMAINS = 1,  
        KEY = 2,
	    ADDRESS = 3,
        EXPIRE = 4,
	    STATUS = 5,
        REGISTERADDRESS = 6,
        REGISTERADDRESSBALANCE = 7,
        SITEFEE = 8,
        SETEXPIRE = 9
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(QModelIndex &index, const QVariant insert, int role);
    //QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    bool removeMn(const QModelIndex& index);
    bool addMn(const QString domain, const QString key, const QString addr, const QString registerAddr,int sitefee, int expire, int status);
    void updateMNList();
    void setWallet(WalletModel * wModel);
    QString GetRandomString();
    WalletModel* wModel = nullptr;

private:
    QString getAddressBalance(QString address);
    QMap<QString,SubSite*> nodes;
    QMap<std::string, bool> collateralTxAccepted;
    void writeSitesToDisk();
    int counter;
    QObject *par;
};

#endif // MNMODEL_H
