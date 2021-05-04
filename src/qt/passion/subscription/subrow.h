//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SUBROW_H
#define SUBROW_H

#include <QWidget>

namespace Ui {
class SubRow;
}

class SubRow : public QWidget
{
    Q_OBJECT

public:
    explicit SubRow(QWidget *parent = nullptr);
    ~SubRow();

    void updateView(QString domain, QString key, QString expire, QString paymentaddress, QString sitefee, QString registeraddress, QString registeraddressbalance, QString status);

Q_SIGNALS:
    void onMenuClicked();
private:
    Ui::SubRow *ui;
};

#endif // SUBROW_H
