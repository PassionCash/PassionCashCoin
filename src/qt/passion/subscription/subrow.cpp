//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/subscription/subrow.h"
#include "qt/passion/subscription/ui_subrow.h"
#include "qt/passion/qtutils.h"
#include <QDateTime>
#include <QString>

SubRow::SubRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SubRow)
{
    ui->setupUi(this);
    setCssProperty(ui->labelDomain, "text-list-title1");
    setCssProperty(ui->labelKey, "text-list-body2");
    setCssProperty(ui->lblValidTitle, "text-list-title1");
    setCssProperty(ui->lblValidValue, "text-list-caption-medium");
    setCssProperty(ui->lblPaymentAddressTitle, "text-list-title1");
    setCssProperty(ui->lblPaymentAddressValue, "text-list-caption-medium");
    setCssProperty(ui->lblRegisteredAddressTitle, "text-list-title1");
    setCssProperty(ui->lblRegisteredAddressValue, "text-list-caption-medium");
    setCssProperty(ui->lblBalanceRegisteredAddressTitle, "text-list-title1");
    setCssProperty(ui->lblBalanceRegisteredAddressValue, "text-list-caption-medium");
    setCssProperty(ui->lblSiteFeeTitle, "text-list-title1");
    setCssProperty(ui->lblSiteFeeValue, "text-list-caption-medium");
    setCssBtnPrimary(ui->btnVisitSite);
    ui->btnVisitSite->setVisible(false);
    ui->lblDivisory->setStyleSheet("background-color:#bababa;");
}

void SubRow::updateView(QString domain, QString key, QString paymentaddress,QString sitefee, QString registeraddress, QString registerAddressBalance, QString expire, QString status){
    ui->labelDomain->setText(domain);
    ui->labelKey->setText(key);
    ui->lblValidTitle->setText("Time left");
    int expi = expire.toInt() + 3600;
    int timestamp = QDateTime::currentSecsSinceEpoch();
    int hours = (expi - timestamp)/3600;
    int min = ((expi - timestamp)%3600/60);
    QString strh = QString::number(hours);
    QString strmin = QString::number(min).rightJustified(2, '0');
    if((expi-timestamp)<0) {
        ui->lblValidValue->setText("0:00");
        ui->lblValidValue->setStyleSheet("color:#AA0000");
    } else {
        ui->lblValidValue->setText(strh+"h:" + strmin+"min");
        ui->lblValidValue->setStyleSheet("color:#58EEA5");
    }
    ui->lblPaymentAddressTitle->setText("Payment to:");
    ui->lblPaymentAddressValue->setText(paymentaddress);
    ui->lblRegisteredAddressValue->setText(registeraddress);
    ui->lblBalanceRegisteredAddressValue->setText(registerAddressBalance);
    ui->lblSiteFeeTitle->setText("Fee / h");
    ui->lblSiteFeeValue->setText(sitefee + " PASSION");
}

SubRow::~SubRow(){
    delete ui;
}
