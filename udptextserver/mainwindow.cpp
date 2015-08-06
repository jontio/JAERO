#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    udpsocket = new QUdpSocket(this);
    connect(udpsocket,SIGNAL(readyRead()),this,SLOT(readyReadSlot()));
    udpsocket->bind(QHostAddress::Any,8765);
}

void MainWindow::readyReadSlot()
{
    while (udpsocket->hasPendingDatagrams())
    {

        //get packet
        QByteArray datagram;
        datagram.resize(udpsocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        udpsocket->readDatagram(datagram.data(), datagram.size(),&sender, &senderPort);


        //display packet. gerr with qplaintext
        const QTextCursor old_cursor = ui->plainTextEdit->textCursor();
        const int old_scrollbar_value = ui->plainTextEdit->verticalScrollBar()->value();
        const bool is_scrolled_down = old_scrollbar_value == ui->plainTextEdit->verticalScrollBar()->maximum();
        ui->plainTextEdit->moveCursor(QTextCursor::End);
        ui->plainTextEdit->textCursor().insertText(datagram);
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
}

MainWindow::~MainWindow()
{
    delete ui;
}
