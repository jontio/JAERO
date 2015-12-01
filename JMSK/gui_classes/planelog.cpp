#include "planelog.h"
#include "ui_planelog.h"

#include <QDateTime>
#include <QDebug>
#include <QFontMetrics>
#include <QSettings>
#include <QPlainTextEdit>

PlaneLog::PlaneLog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlaneLog)
{
    ui->setupUi(this);
    wantedheightofrow=3;
    ui->tableWidget->horizontalHeader()->setAutoScroll(true);


    //load settings
    QSettings settings("Jontisoft", "JAERO");
    QFontMetrics fm(ui->tableWidget->font());
    ui->tableWidget->setRowCount(settings.value("tableWidget-rows",0).toInt());
    for(int row=0;row<ui->tableWidget->rowCount();row++)
    {
        ui->tableWidget->setRowHeight(row,fm.height()*wantedheightofrow);
        for(int column=0;column<ui->tableWidget->columnCount();column++)
        {
            QString str=((QString)"tableWidget-%1-%2").arg(row).arg(column);
            QTableWidgetItem *newItem = new QTableWidgetItem(settings.value(str,"").toString());
            if(column<4)newItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            ui->tableWidget->setItem(row, column, newItem);
        }
    }


}

PlaneLog::~PlaneLog()
{

    //save settings
    QSettings settings("Jontisoft", "JAERO");
    settings.setValue("tableWidget-rows",ui->tableWidget->rowCount());
    for(int row=0;row<ui->tableWidget->rowCount();row++)
    {
        for(int column=0;column<ui->tableWidget->columnCount();column++)
        {
            QString str=((QString)"tableWidget-%1-%2").arg(row).arg(column);
            if(ui->tableWidget->item(row,column))settings.setValue(str,ui->tableWidget->item(row,column)->text());
        }
    }


    delete ui;    
}

void PlaneLog::update(ISUItem &isuitem)
{
    ui->tableWidget->setSortingEnabled(false);//!!!!!

    QFontMetrics fm(ui->tableWidget->font());

    int rows = ui->tableWidget->rowCount();
    QString AESIDstr=((QString)"").sprintf("%06X",isuitem.AESID);
    bool found = false;
    int idx=-1;
    for(int i = 0; i < rows; ++i)
    {
        if(ui->tableWidget->item(i, 0)->text() == AESIDstr)
        {
            found = true;
            idx=i;
            break;
        }
    }
    QTableWidgetItem *AESitem;
    QTableWidgetItem *REGitem;
    QTableWidgetItem *LastHearditem;
    QTableWidgetItem *Countitem;
    QTableWidgetItem *LastMessageitem;
    if(!found)
    {
        ui->tableWidget->insertRow(0);
        QFontMetrics fm(ui->tableWidget->font());
        ui->tableWidget->setRowHeight(0,fm.height()*wantedheightofrow);
        idx=0;
        AESitem = new QTableWidgetItem;
        REGitem = new QTableWidgetItem;
        LastHearditem = new QTableWidgetItem;
        Countitem = new QTableWidgetItem;
        LastMessageitem = new QTableWidgetItem;
        ui->tableWidget->setItem(0,0,AESitem);
        ui->tableWidget->setItem(0,1,REGitem);
        ui->tableWidget->setItem(0,2,LastHearditem);
        ui->tableWidget->setItem(0,3,Countitem);
        ui->tableWidget->setItem(0,4,LastMessageitem);
        AESitem->setText(AESIDstr);
        Countitem->setText("0");

        AESitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        REGitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        Countitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        LastHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    }
    AESitem = ui->tableWidget->item(idx, 0);
    REGitem = ui->tableWidget->item(idx, 1);
    LastHearditem = ui->tableWidget->item(idx, 2);
    Countitem = ui->tableWidget->item(idx, 3);
    LastMessageitem = ui->tableWidget->item(idx, 4);

    if(!isuitem.REG.isEmpty())REGitem->setText(isuitem.REG);
     else REGitem->setText("Unknown");
    LastHearditem->setText(QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss"));

    Countitem->setText(QString::number(Countitem->text().toInt()+1));

    QString tmp=LastMessageitem->text();
    if(tmp.count('\n')>=10)
    {
        int idx=tmp.indexOf("\n");
        tmp=tmp.right(tmp.size()-idx-1);
    }

    if(LastMessageitem->text().isEmpty()&&!isuitem.humantext.isEmpty())LastMessageitem->setText("✈: "+QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ")+isuitem.humantext);
     else if(!isuitem.humantext.isEmpty())LastMessageitem->setText(tmp+"\n✈: "+QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ")+isuitem.humantext);


    ui->tableWidget->setSortingEnabled(true);//allow sorting again
}

void PlaneLog::on_pushButtonsmall_clicked()
{
    wantedheightofrow=3;
    QFontMetrics fm(ui->tableWidget->font());
    int rows = ui->tableWidget->rowCount();
    for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
}

void PlaneLog::on_pushButtonlarge_clicked()
{
    wantedheightofrow=10;
    QFontMetrics fm(ui->tableWidget->font());
    int rows = ui->tableWidget->rowCount();
    for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
}

void PlaneLog::on_pushButtonstopsort_clicked()
{
    ui->tableWidget->sortItems(-1);

}

void PlaneLog::on_pushButtonsrink_clicked()
{
    QFontMetrics fm(ui->tableWidget->font());
    ui->tableWidget->setColumnWidth(ui->tableWidget->columnCount()-1,fm.width('_')*10);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
}

void PlaneLog::on_pushButtonexpand_clicked()
{
    QFontMetrics fm(ui->tableWidget->font());
    ui->tableWidget->setColumnWidth(ui->tableWidget->columnCount()-1,fm.width('_')*(220+100));

    //you can add images like this
    //QPixmap pixmap( ":/images/clear.png" );
    //ui->tableWidget->item(0, 0)->setData(Qt::DecorationRole, pixmap);
}

void PlaneLog::on_pushButtonclear_clicked()
{
    ui->tableWidget->clearContents();
    for(int rows = 0; ui->tableWidget->rowCount(); rows++)ui->tableWidget->removeRow(0);
}
