//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SUBSCRIPTIONTIPMENU_H
#define SUBSCRIPTIONTIPMENU_H

#include "qt/passion/pwidget.h"
#include <QWidget>
#include <QModelIndex>

class PassionGUI;
class WalletModel;

namespace Ui {
class SubscriptionTipMenu;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class SubscriptionTipMenu : public PWidget
{
    Q_OBJECT

public:
    explicit SubscriptionTipMenu(PassionGUI* _window, QWidget *parent = nullptr);
    ~SubscriptionTipMenu() override;

    void setIndex(const QModelIndex &index);
    virtual void showEvent(QShowEvent *event) override;

    void setVisitBtnText(QString btnText);
    void setDeleteBtnText(QString btnText);
    void setFundBtnText(QString btnText);
    void setPayBtnText(QString btnText);
    void setInfoBtnText(QString btnText, int minHeight = 30);
    void setVisitBtnVisible(bool visible);
    void setFundBtnVisible(bool visible);
    void setDeleteBtnVisible(bool visible);
    void setPayBtnVisible(bool visible);
    void setInfoBtnVisible(bool visible);

Q_SIGNALS:
    void onDeleteClicked();
    void onFundClicked();
    void onVisitClicked();
    void onPayClicked();
    void onInfoClicked();

private Q_SLOTS:
    void deleteClicked();
    void visitClicked();
    void fundClicked();
    void payClicked();
    void infoClicked();

private:
    Ui::SubscriptionTipMenu *ui;
    QModelIndex index;
};

#endif // SUBSCRIPTIONTIPMENU_H
