#ifndef SLIP_H
#define SLIP_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QIODevice>

#define BYTE uchar

/* SLIP special character codes
 */
#define SLIP_END             0xC0    /* indicates end of packet */
#define SLIP_ESC             0xDB    /* indicates byte stuffing */
#define SLIP_ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0xDE    /* ESC ESC_ESC means ESC data byte */

class Slip : public QObject
{
    Q_OBJECT
public:
    explicit Slip(QObject *parent = 0);
    ~Slip();
    void sendpacket(const QByteArray &data);
    void setPortName(const QString serialPortName);
    QString getPortName();
    void setBaudRate(qint32 serialPortBaudRate);
    bool open(QIODevice::OpenMode mode);
    void close();
    bool isOpen();
    void setmaxbuffersize(uint size);
signals:
    void serialPortError(QSerialPort::SerialPortError serialPortError);
    void testMsg(QString msg);
    void rxpacket(QByteArray pkt);
public slots:
private slots:
    void handleReadyRead();
private:
    QSerialPort *m_serialPort;
    QByteArray  m_inbuffer;
    char lastbyte;
    uint maxbuffersize;
};

#endif // SLIP_H
