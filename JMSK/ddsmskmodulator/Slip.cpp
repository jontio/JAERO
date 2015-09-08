#include "Slip.h"
#include <QDebug>
#include <windows.h>

Slip::Slip(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
{
    lastbyte=0;
    maxbuffersize=4096;
    connect(m_serialPort, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SIGNAL(serialPortError(QSerialPort::SerialPortError)));
}

Slip::~Slip()
{

}

void Slip::setmaxbuffersize(uint size)
{
    maxbuffersize=size;
}

void Slip::handleReadyRead()
{
    QByteArray ba=m_serialPort->readAll();
    emit testMsg(ba);
    for(int i=0;i<ba.length();i++)
    {

       if((uchar)ba.at(i)==SLIP_END)
       {
           if(m_inbuffer.length())emit rxpacket(m_inbuffer);
           m_inbuffer.clear();
           lastbyte=ba.at(i);
           continue;
       }

       if((uchar)lastbyte==SLIP_ESC)
       {
           switch((uchar)ba.at(i))
           {
           case SLIP_ESC_END:
                   m_inbuffer.push_back(SLIP_END);
                   //qDebug()<<"Slip rx: SLIP_ESC_END";
                   break;
           case SLIP_ESC_ESC:
                   m_inbuffer.push_back(SLIP_ESC);
                   //qDebug()<<"Slip rx: SLIP_ESC_ESC";
                   break;
           default:
                   qDebug()<<"Slip error. Unknowen escape code";
           }
           lastbyte=ba.at(i);
           continue;
       }

       lastbyte=ba.at(i);
       if((uchar)lastbyte!=SLIP_ESC)m_inbuffer.push_back(lastbyte);

       if(m_inbuffer.length()>(int)maxbuffersize)m_inbuffer.clear();

    }
}

void Slip::sendpacket(const QByteArray &data)
{

    //form SLIP packet
    QByteArray data_esc;
    //data_esc.push_back(SLIP_END);
    for(int i=0;i<data.length();i++)
    {
            switch((uchar)data.at(i))
            {
            case SLIP_END:
                    data_esc.push_back(SLIP_ESC);
                    data_esc.push_back(SLIP_ESC_END);
                    //qDebug()<<"Slip send: SLIP_END";
                    break;
            case SLIP_ESC:
                    data_esc.push_back(SLIP_ESC);
                    data_esc.push_back(SLIP_ESC_ESC);
                    //qDebug()<<"Slip send: SLIP_ESC";
                    break;
            default:
                    data_esc.push_back(data.at(i));
            }
    }
    data_esc.push_back(SLIP_END);

    //que it up in the serial buffer
    m_serialPort->write(data_esc);


}

void Slip::setPortName(QString serialPortName)
{
    m_serialPort->setPortName(serialPortName);
}

QString Slip::getPortName()
{
    return m_serialPort->portName();
}

void Slip::setBaudRate(qint32 serialPortBaudRate)
{
    m_serialPort->setBaudRate(serialPortBaudRate);
}

bool Slip::open(QIODevice::OpenMode mode)
{
    if (!m_serialPort->open(mode))
    {
        qDebug() << QObject::tr("Failed to open port %1, error: %2").arg(m_serialPort->portName()).arg(m_serialPort->errorString()) << endl;
        emit serialPortError(QSerialPort::OpenError);
        return false;
    }
    return true;
}

void Slip::close()
{
    m_serialPort->close();
}

bool Slip::isOpen()
{
    m_serialPort->isOpen();
}
