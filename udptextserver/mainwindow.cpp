#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    udpsocket = new QUdpSocket(this);
    connect(udpsocket,SIGNAL(readyRead()),this,SLOT(readyReadSlot()));
    udpsocket->bind(QHostAddress::Any,8765);

    //workaround for strange QUdpSocket bug where readyRead signal may not emit and never emit again filling up incoming buffer and the output seems to freeze. Can someone please do something about this?
    readtimer=new QTimer(this);
    connect(readtimer,SIGNAL(timeout()),this,SLOT(readyReadSlot()));
    readtimer->start(50);
}

void MainWindow::readyReadSlot()
{
    readtimer->stop();
    while (udpsocket->hasPendingDatagrams())
    {
        //get packet
        QByteArray datagram;
        datagram.resize(udpsocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        udpsocket->readDatagram(datagram.data(), datagram.size(),&sender, &senderPort);

        //turn into hex
        QByteArray datagramhex;for(int i=0;i<datagram.size();i++)datagramhex+=QString("%1").arg((uchar) datagram[i], 2, 16, QLatin1Char( '0' )).toUpper()+" ";

        //display packet. gerr with qplaintext
        const QTextCursor old_cursor = ui->plainTextEdit->textCursor();
        const int old_scrollbar_value = ui->plainTextEdit->verticalScrollBar()->value();
        const bool is_scrolled_down = old_scrollbar_value == ui->plainTextEdit->verticalScrollBar()->maximum();
        ui->plainTextEdit->moveCursor(QTextCursor::End);
        ui->plainTextEdit->textCursor().insertText(datagramhex);
        if (old_cursor.hasSelection() || !is_scrolled_down)
        {
            ui->plainTextEdit->setTextCursor(old_cursor);
            ui->plainTextEdit->verticalScrollBar()->setValue(old_scrollbar_value);
        }
        else
        {
            ui->plainTextEdit->moveCursor(QTextCursor::End);
            ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum());
        }

    }
    readtimer->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}
