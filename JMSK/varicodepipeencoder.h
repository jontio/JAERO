#ifndef VARICODEPIPEENCODER_H
#define VARICODEPIPEENCODER_H

#include <QObject>
#include <QIODevice>
#include <QPointer>
#include <QVector>
#include <QByteArray>
#include "../varicode/varicode.h"
#include "DSP.h"

class VariCodePipeEncoder : public QIODevice
{
    Q_OBJECT
public:
    explicit VariCodePipeEncoder(QObject *parent = 0);
    ~VariCodePipeEncoder();
    void ConnectSourceDevice(QIODevice *device);
    void DisconnectSourceDevice();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
signals:
public slots:
private:
    bool Start();
    void Stop();
    QVector<short> &Encode(QByteArray &bytes);
    QPointer<QIODevice> psourcedevice;
    QByteArray encodedbytes;
    QVector<short> sbits;
    QByteArray bytesin;
    BaceConverter bc;
};

#endif // VARICODEPIPEENCODER_H
