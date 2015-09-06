#ifndef SERIALPPT_H
#define SERIALPPT_H

#include <QObject>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QPointer>

class SerialPPT : public QObject
{
    Q_OBJECT
public:
    explicit SerialPPT(QObject *parent = 0);
    bool setportname(QString portname);
signals:
    void Warning(QString msg);
public slots:
    void setPPT(bool state);
    void setPPTon()
    {
        setPPT(true);
    }
    void setPPToff()
    {
        setPPT(false);
    }
private:
    QPointer<QSerialPort> pserialport;
};

#endif // SERIALPPT_H
