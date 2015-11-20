#ifndef AEROL_H
#define AEROL_H

#include <QObject>
#include <QPointer>
#include <QVector>
#include <QDebug>

class AeroLInterleaver
{
public:
    AeroLInterleaver();
    void setSize(int N);
    QVector<int> &interleave(QVector<int> &block);
    QVector<int> &deinterleave(QVector<int> &block);
private:
    QVector<int> matrix;
    int M;
    int N;
    QVector<int> interleaverowpermute;
    QVector<int> interleaverowdepermute;
};

class PreambleDetector
{
public:
    PreambleDetector();
    void setPreamble(QVector<int> _preamble);
    bool setPreamble(quint64 bitpreamble,int len);
    bool Update(int val);
private:
    QVector<int> preamble;
    QVector<int> buffer;
    int buffer_ptr;
};

class AeroL : public QIODevice
{
    Q_OBJECT
public:
    explicit AeroL(QObject *parent = 0);
    ~AeroL();
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
    QVector<short> sbits;
    QByteArray decodedbytes;
    PreambleDetector preambledetector;

    QVector<int> block;
    AeroLInterleaver leaver;

};

#endif // AEROL_H
