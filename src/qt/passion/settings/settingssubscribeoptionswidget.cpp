// Copyright (c) 2019 The Passion developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/settings/settingssubscribeoptionswidget.h"
#include "qt/passion/settings/forms/ui_settingssubscribeoptionswidget.h"
#include <QListView>
#include "optionsmodel.h"
#include "walletmodel.h"
#include "clientmodel.h"
#include "qt/passion/qtutils.h"

SettingsSubscribeOptionsWidget::SettingsSubscribeOptionsWidget(PassionGUI* _window, QWidget *parent) :
    PWidget(_window, parent),
    ui(new Ui::SettingsSubscribeOptionsWidget)
{
    ui->setupUi(this);

    this->setStyleSheet(parent->styleSheet());

    // Containers
    ui->left->setProperty("cssClass", "container");
    ui->left->setContentsMargins(10,10,10,10);

    // Title
    setCssTitleScreen(ui->lblTitleHeadline);
    setCssSubtitleScreen(ui->lblSubtitleHeadline);

    // Autofunding Spinbox
    ui->lblSubscribeAutofundingAmount->setProperty("cssClass", "text-main-settings");
    ui->spinBoxSubscribeAutofunding->setProperty("cssClass", "btn-spin-box");
    ui->spinBoxSubscribeAutofunding->setAttribute(Qt::WA_MacShowFocusRect, 0);
    setShadow(ui->spinBoxSubscribeAutofunding);

    // Proxy
    ui->lblSubtitleSubscribeServiceIP->setProperty("cssClass", "text-main-settings");
    initCssEditLine(ui->lineEditSubscribeServiceIP);

    // Buttons
    setCssBtnPrimary(ui->pushButtonSave);
    setCssBtnSecondary(ui->pushButtonReset);
    setCssBtnSecondary(ui->pushButtonClean);

    connect(ui->pushButtonSave, &QPushButton::clicked, [this] { Q_EMIT saveSettings(); });
    connect(ui->pushButtonReset, &QPushButton::clicked, this, &SettingsSubscribeOptionsWidget::onResetClicked);
    connect(ui->pushButtonClean, &QPushButton::clicked, [this] { Q_EMIT discardSettings(); });
}

void SettingsSubscribeOptionsWidget::onResetClicked(){
    if (clientModel) {
        OptionsModel *optionsModel = clientModel->getOptionsModel();
        QSettings settings;
        optionsModel->setWalletDefaultOptions(settings, true);
        optionsModel->setNetworkDefaultOptions(settings, true);
        inform(tr("Options reset succeed"));
    }
}

void SettingsSubscribeOptionsWidget::setMapper(QDataWidgetMapper *mapper){
    mapper->addMapping(ui->lineEditSubscribeServiceIP, OptionsModel::SubscriptionServiceIP);
    mapper->addMapping(ui->chkSubscribeAutofunding, OptionsModel::SubscriptionAutofunding);
    mapper->addMapping(ui->spinBoxSubscribeAutofunding, OptionsModel::SubscriptionAutofundingValue);
}

SettingsSubscribeOptionsWidget::~SettingsSubscribeOptionsWidget(){
    delete ui;
}
