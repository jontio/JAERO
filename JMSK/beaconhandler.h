#ifndef BEACONHANDLER_H
#define BEACONHANDLER_H

#include <QObject>
#include <QTimer>

class BeaconHandler : public QObject
{
    Q_OBJECT
public:
    struct Settings{
        int beaconminidle;
        int beaconmaxidle;
        int backoff;
        int maxbackoffs;
        Settings()
        {
            beaconminidle=110;
            beaconmaxidle=130;
            backoff=10;
            maxbackoffs=10000;
        }
        bool operator==(const Settings&  other)
        {
            if(beaconminidle!=other.beaconminidle)return false;
            if(beaconmaxidle!=other.beaconmaxidle)return false;
            if(backoff!=other.backoff)return false;
            if(maxbackoffs!=other.maxbackoffs)return false;
            return true;
        }
        bool operator!=(const Settings&  other)
        {
            return !(operator ==(other));
        }
    };
    explicit BeaconHandler(QObject *parent = 0);
    void setSettings(Settings settings);
signals:
    void DoTransmissionNow();
    void Stoped();
    void Started();
public slots:
    void Start();
    void Stop();
    void StartStop(bool state)
    {
        if(state)Start();
         else Stop();
    }
    void SignalStatus(bool gotsignal);
    void TransmissionStatus(bool istransmitting);
private slots:
    void MainTimeOut();
private:
    QTimer *maintimer;
    Settings settings;
    bool gotsignal;
    bool istransmitting;
    int backoffcnt;
};

#endif // BEACONHANDLER_H
