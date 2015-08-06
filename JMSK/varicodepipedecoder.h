#ifndef VARICODEPIPEDECODER_H
#define VARICODEPIPEDECODER_H

#include <QObject>
#include <QIODevice>
#include <QPointer>
#include <QVector>
#include <QByteArray>
#include "../varicode/varicode.h"

class VariCodePipeDecoder : public QIODevice
{
    Q_OBJECT
public:
    explicit VariCodePipeDecoder(QObject *parent = 0);
    ~VariCodePipeDecoder();
    void ConnectSinkDevice(QIODevice *device);
    void DisconnectSinkDevice();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
signals:
public slots:
private:
    bool Start();
    void Stop();
    QByteArray &Decode(QVector<short> &bits);
    QPointer<QIODevice> psinkdevice;
    VARICODE_DEC varicode_dec_states;
    QByteArray decodedbytes;
    QVector<short> sbits;
};

#endif // VARICODEPIPEDECODER_H
