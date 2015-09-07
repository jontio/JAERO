#include "serialppt.h"
#include <QDebug>

SerialPPT::SerialPPT(QObject *parent) : QObject(parent)
{

}

bool SerialPPT::setportname(QString portname)
{
    if(!pserialport.isNull())
    {
        if(portname==pserialport.data()->portName())
        {
            if(!pserialport.data()->isOpen())
            {
                if(!pserialport.data()->open(QIODevice::ReadWrite))
                {
                    emit Warning("Can not open serial port \""+pserialport.data()->portName()+"\"");
                    return false;
                }
                setPPTon();
                setPPToff();
                return true;
            }
            return true;
        }
        pserialport.data()->close();
        pserialport.data()->deleteLater();
        pserialport.clear();
    }
    if(portname=="None")return true;
    pserialport = new QSerialPort(portname,this);
    if(!pserialport.data()->open(QIODevice::ReadWrite))
    {
        emit Warning("Can not open serial port \""+pserialport.data()->portName()+"\"");
        return false;
    }
    //some ports seem to not turn off initally. problem. please fix!!
    setPPTon();
    setPPToff();


    return true;

}

void SerialPPT::setPPT(bool state)
{
    if(pserialport.isNull())
    {
        //qDebug()<<"port null";
        return;
    }
    //qDebug()<<"line = "<<state;
    if(!pserialport.data()->isOpen())
    {
        emit Warning("Can not open serial port \""+pserialport.data()->portName()+"\"");
        return;
    }
    bool suc1 = pserialport.data()->setDataTerminalReady(state);
    bool suc2 = pserialport.data()->setRequestToSend(state);
    if(suc1&&suc2)return;
    emit Warning("Cant set serial port lines");
}

