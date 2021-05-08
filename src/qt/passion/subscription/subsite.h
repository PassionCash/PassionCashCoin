// Copyright (c) 2021 The PASSION Developers
#ifndef SUBSITE_H
#define SUBSITE_H

#include <QObject>

class SubSite : public QObject
{
    Q_OBJECT
public:
    explicit SubSite(QObject *parent = nullptr);
    void setSiteName(QString sitename) {
        m_sitename = sitename;
    }
    void setSiteDomain(QString sitedomain) {
        m_sitedomain = sitedomain;
    }
    void setSiteKey(QString sitekey) {
        m_sitekey = sitekey;
    }
    void setSiteAddr(QString siteaddr) {
        m_siteaddr = siteaddr;
    }
    void setSiteAddrRegister(QString siteaddrregister) {
        m_siteregisteraddr = siteaddrregister;
    }
    void setSiteExpire(int siteexpire) {
        m_siteexpire = siteexpire;
    }
    void setSiteState(int sitestate) {
        m_sitestate = sitestate;
    }
    void setSiteAddrRegisterBalance(QString siteaddrregisteramount) {
        m_siteaddrregisteramount = siteaddrregisteramount;
    }
    void setSiteFee(qint64 sitefee) {
        m_sitefee = sitefee;
    }
    QString getSiteName() { return m_sitename; }
    QString getSiteDomain() { return m_sitedomain; }
    QString getSiteKey() { return m_sitekey; }
    QString getSiteAddr() { return m_siteaddr; }
    QString getSiteAddrRegister() { return m_siteregisteraddr; }
    int getSiteExpire() { return m_siteexpire; }
    int getSiteState() { return m_sitestate; }
    QString getSiteAddrRegisterAmount() { return m_siteaddrregisteramount; }
    qint64 getSiteFee() { return m_sitefee; }

private:
    QString m_sitename = "";
    QString m_sitedomain = "";
    QString m_sitekey = "";
    QString m_siteaddr = "";
    QString m_siteregisteraddr = "";
    int m_siteexpire = 0;
    int m_sitestate = 0;
    QString m_siteaddrregisteramount = 0;
    qint64 m_sitefee = 0;
};

#endif // SUBSITE_H
