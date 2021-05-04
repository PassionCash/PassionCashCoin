//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SUBSCRIPTIONWIDGET_H
#define SUBSCRIPTIONWIDGET_H

#include <QWidget>
#include "qt/passion/pwidget.h"
#include "qt/passion/subscription/subsite.h"
#include "coincontrol.h"
#include "qt/passion/furabstractlistitemdelegate.h"
#include "qt/passion/subscription/subscriptionmodel.h"
#include "qt/passion/subscription/subscriptiontipmenu.h"
#include <QTimer>
#include <QString>

class PassionGUI;
class WalletModel;

namespace Ui {
class SubscriptionWidget;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class SubscriptionWidget : public PWidget
{
    Q_OBJECT

public:

    explicit SubscriptionWidget(PassionGUI *parent = nullptr);
    ~SubscriptionWidget();

    void loadWalletModel() override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private Q_SLOTS:
    void onRegisterClicked();
    void onPayClicked();
    void onVisitMNClicked();
    void onFundMNClicked();
    void changeTheme(bool isLightTheme, QString &theme) override;
    void onMNClicked(const QModelIndex &index);
    void onPayMNClicked();
    void onDeleteMNClicked();
    void onInfoMNClicked();
    void updateListState();
    void RegisterToSite();
    QString getNewAddress();
    QString signRegisterMessage(const QString strSignAddress, const QString message);

private:
    Ui::SubscriptionWidget *ui;
    FurAbstractListItemDelegate *delegate;
    SubscriptionModel *subscriptionModel = nullptr;
    SubscriptionTipMenu* menu = nullptr;
    QModelIndex index;
    QTimer *timer = nullptr;
    CCoinControl *coinControl = nullptr;
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn& sendCoinsReturn, const QString& msgArg = QString(), bool fPrepare = false);
    void startAlias(QString strAlias);
};

#endif // MASTERNODESWIDGET_H
