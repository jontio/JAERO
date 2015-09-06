#include "serialppt.h"
#include <QDebug>

SerialPPT::SerialPPT(QObject *parent) : QObject(parent)
{

}

bool SerialPPT::setportname(QString portname)
{
    if(!pserialport.isNull())
    {
        pserialport.data()->close();
        pserialport.data()->deleteLater();
        pserialport.clear();
    }
    if(portname=="None")return true;
    pserialport = new QSerialPort(portname,this);
    if(!pserialport.data()->open(QIODevice::ReadWrite))return false;

    setPPT(false);

    return true;

}

void SerialPPT::setPPT(bool state)
{
    if(pserialport.isNull())
    {
        return;
    }
    if(!pserialport.data()->isOpen())
    {
        emit Warning("Can not open serial port");
        return;
    }
    pserialport.data()->clear();
    bool suc1 = pserialport.data()->setDataTerminalReady(state);
    bool suc2 = pserialport.data()->setRequestToSend(state);
    if(suc1&&suc2)return;
    emit Warning("Cant set serial port lines");
}

