//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/subscription/subscriptiontipmenu.h"
#include "qt/passion/subscription/ui_subscriptiontipmenu.h"

#include "qt/passion/passiongui.h"
#include "qt/passion/qtutils.h"
#include <QTimer>

SubscriptionTipMenu::SubscriptionTipMenu(PassionGUI *_window, QWidget *parent) :
    PWidget(_window, parent),
    ui(new Ui::SubscriptionTipMenu)
{
    ui->setupUi(this);
    //ui->btnLast->setVisible(false);
    setCssProperty(ui->container, "container-list-menu");
    setCssProperty(ui->btnVisit, "btn-subscribe-visit");
    setCssProperty(ui->btnFund, "btn-subscribe-fund");
    setCssProperty(ui->btnPay, "btn-subscribe-pay");
    setCssProperty(ui->btnCopy, "btn-subscribe-copy");
    setCssProperty(ui->btnInfo, "btn-subscribe-info");
    //setCssProperty({ui->btnVisit, ui->btnDelete, ui->btnPay, ui->btnInfo, ui->btnFund}, "btn-list-menu");
    connect(ui->btnFund, SIGNAL(clicked()), this, SLOT(fundClicked()));
    connect(ui->btnVisit, SIGNAL(clicked()), this, SLOT(visitClicked()));
    connect(ui->btnCopy, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->btnPay, SIGNAL(clicked()), this, SLOT(payClicked()));
    connect(ui->btnInfo, SIGNAL(clicked()), this, SLOT(infoClicked()));
}

void SubscriptionTipMenu::setVisitBtnText(QString btnText){
    ui->btnVisit->setText(btnText);
}
void SubscriptionTipMenu::setFundBtnText(QString btnText){
    ui->btnFund->setText(btnText);
}

void SubscriptionTipMenu::setDeleteBtnText(QString btnText){
    ui->btnCopy->setText(btnText);
}

void SubscriptionTipMenu::setPayBtnText(QString btnText){
    ui->btnPay->setText(btnText);
}

void SubscriptionTipMenu::setInfoBtnText(QString btnText, int minHeight){
    ui->btnInfo->setText(btnText);
    //ui->btnInfo->setMinimumHeight(minHeight);
}

void SubscriptionTipMenu::setVisitBtnVisible(bool visible){
    ui->btnVisit->setVisible(visible);
}
void SubscriptionTipMenu::setFundBtnVisible(bool visible){
    ui->btnFund->setVisible(visible);
}

void SubscriptionTipMenu::setDeleteBtnVisible(bool visible){
    ui->btnCopy->setVisible(visible);
}

void SubscriptionTipMenu::setPayBtnVisible(bool visible) {
    ui->btnPay->setVisible(visible);
}

void SubscriptionTipMenu::setInfoBtnVisible(bool visible) {
    ui->btnInfo->setVisible(visible);
}

void SubscriptionTipMenu::deleteClicked(){
    hide();
    Q_EMIT onDeleteClicked();
}

void SubscriptionTipMenu::fundClicked(){
    hide();
    Q_EMIT onFundClicked();
}

void SubscriptionTipMenu::visitClicked(){
    hide();
    Q_EMIT onVisitClicked();
}

void SubscriptionTipMenu::payClicked(){
    hide();
    Q_EMIT onPayClicked();
}

void SubscriptionTipMenu::infoClicked() {
    hide();
    Q_EMIT onInfoClicked();
}

void SubscriptionTipMenu::showEvent(QShowEvent *event){
    QTimer::singleShot(5000, this, SLOT(hide()));
}

SubscriptionTipMenu::~SubscriptionTipMenu()
{
    delete ui;
}
