// Copyright (c) 2019 The Passion developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SETTINGSSUBSCRIBEOPTIONSWIDGET_H
#define SETTINGSSUBSCRIBEOPTIONSWIDGET_H

#include <QWidget>
#include <QDataWidgetMapper>
#include "qt/passion/pwidget.h"
namespace Ui {
class SettingsSubscribeOptionsWidget;
}

class SettingsSubscribeOptionsWidget : public PWidget
{
    Q_OBJECT

public:
    explicit SettingsSubscribeOptionsWidget(PassionGUI* _window, QWidget *parent = nullptr);
    ~SettingsSubscribeOptionsWidget();

    void setMapper(QDataWidgetMapper *mapper);

Q_SIGNALS:
    void saveSettings();
    void discardSettings();

public Q_SLOTS:
    void onResetClicked();

private:
    Ui::SettingsSubscribeOptionsWidget *ui;
};

#endif // SETTINGSSUBSCRIBEOPTIONSWIDGET_H
