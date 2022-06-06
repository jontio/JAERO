#ifndef ACARSITEM_CONVERTER_H
#define ACARSITEM_CONVERTER_H

#include "jserialize.h"
#include "aerol.h"

class ISUItem_QObject : public JSerialize
{
    Q_OBJECT
public:
    J_SERIALIZE(quint32, AESID)
    J_SERIALIZE(uchar, GESID)
    J_SERIALIZE(uchar, QNO)
    J_SERIALIZE(uchar, SEQNO)
    J_SERIALIZE(uchar, REFNO)
    J_SERIALIZE(uchar, NOOCTLESTINLASTSSU)
    J_SERIALIZE(QByteArray, userdata)
    J_SERIALIZE(int, count)
    void clear()
    {
        AESID=0;
        GESID=0;
        QNO=0;
        SEQNO=0;
        REFNO=0;
        NOOCTLESTINLASTSSU=0;
        userdata.clear();
        count=0;
    }

    ISUItem_QObject& operator =(const ISUItem& other)
    {
        AESID=other.AESID;
        GESID=other.GESID;
        QNO=other.QNO;
        SEQNO=other.SEQNO;
        REFNO=other.REFNO;
        NOOCTLESTINLASTSSU=other.NOOCTLESTINLASTSSU;
        userdata=other.userdata;
        count=other.count;
        return *this;
    }

    ISUItem &toISUItem()
    {
        anISUItem.AESID=AESID;
        anISUItem.GESID=GESID;
        anISUItem.QNO=QNO;
        anISUItem.SEQNO=SEQNO;
        anISUItem.REFNO=REFNO;
        anISUItem.NOOCTLESTINLASTSSU=NOOCTLESTINLASTSSU;
        anISUItem.userdata=userdata;
        anISUItem.count=count;
        return anISUItem;
    }

    operator ISUItem & ()
    {
        return toISUItem();
    }

private:
    ISUItem anISUItem;

};

class ACARSItem_QObject : private JSerialize
{
    Q_OBJECT
public:
    ISUItem_QObject isuitem;
    J_SERIALIZE(char, MODE)
    J_SERIALIZE(uchar, TAK)
    J_SERIALIZE(QByteArray, LABEL)
    J_SERIALIZE(uchar, BI)
    J_SERIALIZE(QByteArray, PLANEREG)
    J_SERIALIZE(QStringList, dblookupresult)
    J_SERIALIZE(bool, nonacars)
    J_SERIALIZE(bool, downlink)
    J_SERIALIZE(bool, valid)
    J_SERIALIZE(bool, hastext)
    J_SERIALIZE(bool, moretocome)
    J_SERIALIZE(QString, message)

    ACARSItem_QObject() : JSerialize()
    {
    }
    ACARSItem_QObject(const ACARSItem& other) : JSerialize()
    {
        this->operator =(other);
    }

    ACARSItem_QObject& operator =(const ACARSItem& other)
    {
        isuitem=other.isuitem;
        MODE=other.MODE;
        TAK=other.TAK;
        LABEL=other.LABEL;
        BI=other.BI;
        PLANEREG=other.PLANEREG;
        dblookupresult=other.dblookupresult;
        nonacars=other.nonacars;
        downlink=other.downlink;
        valid=other.valid;
        hastext=other.hastext;
        moretocome=other.moretocome;
        message=other.message;
        return *this;
    }

    ACARSItem &toACARSItem()
    {
        anACARSItem.isuitem=isuitem;
        anACARSItem.MODE=MODE;
        anACARSItem.TAK=TAK;
        anACARSItem.LABEL=LABEL;
        anACARSItem.BI=BI;
        anACARSItem.PLANEREG=PLANEREG;
        anACARSItem.dblookupresult=dblookupresult;
        anACARSItem.nonacars=nonacars;
        anACARSItem.downlink=downlink;
        anACARSItem.valid=valid;
        anACARSItem.hastext=hastext;
        anACARSItem.moretocome=moretocome;
        anACARSItem.message=message;
        return anACARSItem;
    }

    operator ACARSItem & ()
    {
        return toACARSItem();
    }

    bool fromQByteArray(const QByteArray &ba)
    {
        QDataStream ds(ba);
        ds>>(*this);
        ds>>isuitem;
        if(hasSerializeError()||isuitem.hasSerializeError())return false;
        return true;
    }

    bool toQByteArray(QByteArray &ba)
    {
        ba.clear();
        QDataStream ds(&ba,QIODevice::WriteOnly);
        ds<<(*this);
        ds<<isuitem;
        if(ds.status()!=QDataStream::Ok)return false;
        return true;
    }

    operator const QByteArray & ()
    {
        toQByteArray(ba);
        return ba;
    }

    QByteArray& convert(const ACARSItem &to)
    {
        this->operator =(to);
        toQByteArray(ba);
        return ba;
    }

    ACARSItem& convert(const QByteArray &ba)
    {
        fromQByteArray(ba);
        return toACARSItem();
    }

private:
    ACARSItem anACARSItem;
    QByteArray ba;
};

#endif // ACARSITEM_CONVERTER_H
