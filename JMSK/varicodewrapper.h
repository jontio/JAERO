#ifndef VARICODEWRAPPER_H
#define VARICODEWRAPPER_H

#include <QObject>
#include <QIODevice>
#include <QVector>
#include <QByteArray>
#include "../varicode/varicode.h"

class VariCodeWrapper :  public QIODevice
{
    Q_OBJECT
public:
    explicit VariCodeWrapper(QObject *parent = 0);
    ~VariCodeWrapper();
    QByteArray &Decode(QVector<short> &bits);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
signals:
    void DecodedBytes(const QByteArray &decodedbytes);
public slots:
    void DecodeBits(QVector<short> &bits);
    void DecodeBytes(const QByteArray &bitsinbytes);
private:
    VARICODE_DEC varicode_dec_states;
    QByteArray decodedbytes;
    QVector<short> sbits;
};

#endif // VARICODEWRAPPER_H
