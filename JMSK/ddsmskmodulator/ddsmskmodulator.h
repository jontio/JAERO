#ifndef DDSMSKMODULATOR_H
#define DDSMSKMODULATOR_H

#include <QIODevice>
#include <QPointer>
#include <QObject>
#include <QTimer>
#include "Slip.h"

class DDSMSKModulator : public QIODevice
{
    Q_OBJECT
public:
#define JPKT_ON              0x01    /* turn on/off dds 0=stop 1=PSK 2=FSK              */
#define JPKT_FREQ            0x02    /* set freq of dds                                 */
#define JPKT_PHASE           0x03    /* set phase of dds                                */
#define JPKT_Data            0x05    /* Data to arduino                                 */
#define JPKT_REQ             0x06    /* packet from arduino saying it wants more data   */
#define JPKT_DATA_ACK        0x07    /* ACK recept for given data                       */
#define JPKT_DATA_NACK       0x08    /* NACK recept for given data (no room)            */
#define JPKT_SYMBOL_PERIOD   0x09    /* set symbol period in microseconds               */
#define JPKT_PILOT_ON        0x0A    /* turn on/off dds single frequency                */
#define JPKT_SET_FREQS       0x0B    /* set multi freq for later referencing            */
#define JPKT_GEN_ACK         0x0C    /* general ack                                     */
#define JPKT_GEN_NAK         0x0D    /* general nack                                    */
    struct Settings
    {
        double freq_center;
        double fb;
        double secondsbeforereadysignalemited;

        QString serialportname;

        Settings()
        {
            freq_center=32000000;//Hz
            fb=125;//bps
            secondsbeforereadysignalemited=0;
            serialportname.clear();
        }
    };
    explicit DDSMSKModulator(QObject *parent = 0);
    void ConnectSourceDevice(QIODevice *datasourcedevice);
    void DisconnectSourceDevice();

    //this class is a device
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    void setSettings(Settings settings);

    void stoptimeout();

signals:
    void ReadyState(bool state);
    void SlipMsg(QString msg);
    void WarningMsg(QString msg);

    void statechanged(bool open);
    void closed();
    void opened();

public slots:
    void start();
    void stop();
    void startstop(bool on)
    {
        if(on)start();
         else stop();
    }
    void stopgraceful();
private slots:
    void handlerxslippacket(QByteArray pkt);
    void TimeoutSlot();
private:

    QByteArray reqbuffer;
    QByteArray buffer;
    QByteArray ba;

    void queueJDDSData(QByteArray &data);
    void setJDDSstate(bool state);
    void setJDDSfreqs(QList<float> freqs);
    void setJDDSSymbolRate(double rate);

    QTimer *JDDSTimeout;

    bool ignorereq;

    Slip *slip;
    Settings settings;
    QPointer<QIODevice> pdatasourcedevice;
    int bitcounter;
    int bitstosendbeforereadystatesignal;
};

#endif // DDSMSKMODULATOR_H
