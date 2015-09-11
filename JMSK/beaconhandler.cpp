#include "beaconhandler.h"
#include <QDebug>

BeaconHandler::BeaconHandler(QObject *parent) : QObject(parent)
{
    maintimer = new QTimer(this);
    connect(maintimer,SIGNAL(timeout()),this,SLOT(MainTimeOut()));
    backoffcnt=0;
    istransmitting=false;
    gotsignal=false;
    enabled=false;
}

void BeaconHandler::MainTimeOut()
{
    //qDebug()<<"MainTimeOut";
    maintimer->stop();
    if(gotsignal)
    {
        //qDebug()<<"Got a signal backing off";
        backoffcnt++;
        if(backoffcnt>settings.maxbackoffs)
        {
            Stop();
            //qDebug()<<"back off cnt exceeded";
            return;
        }
        int randtxtime=(qrand()%settings.backoff);
        //qDebug()<<"backing off for"<<randtxtime<<"secs";
        if(enabled)maintimer->start(1000*randtxtime);
        return;
    }
    backoffcnt=0;
    emit DoTransmissionNow();
}

void BeaconHandler::Start()
{
    Stop();
    enabled=true;
    maintimer->start(1000);
    emit Started();
}

void BeaconHandler::Stop()
{
    enabled=false;
    maintimer->stop();
    backoffcnt=0;
    emit Stoped();
}

void BeaconHandler::setSettings(Settings _settings)
{
    if(_settings.backoff<1)_settings.backoff=1;
    if(settings!=_settings)
    {
        settings=_settings;
        if(maintimer->isActive())
        {
            int randtxtime=settings.beaconminidle;
            if((settings.beaconmaxidle-settings.beaconminidle)>0)
            {
                randtxtime+=(qrand()%(settings.beaconmaxidle-settings.beaconminidle));
            }
            if(enabled)maintimer->start(1000*randtxtime);
        }
    }
}

void BeaconHandler::SignalStatus(bool _gotsignal)
{
    gotsignal=_gotsignal;
}

void BeaconHandler::TransmissionStatus(bool _istransmitting)
{
    //qDebug()<<"currect trasmission status is"<<_istransmitting;
    if(istransmitting&&!_istransmitting)
    {
        //qDebug()<<"was transmitting but now we are not. starting timeout";
        int randtxtime=settings.beaconminidle;
        if((settings.beaconmaxidle-settings.beaconminidle)>0)
        {
            randtxtime+=(qrand()%(settings.beaconmaxidle-settings.beaconminidle));
        }
        //qDebug()<<"setting timeout for"<<randtxtime<<"seconds";
        if(enabled)maintimer->start(1000*randtxtime);
    }
    istransmitting=_istransmitting;
}
