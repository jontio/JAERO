#include "planelog.h"
#include "ui_planelog.h"

#include <QDateTime>
#include <QDebug>
#include <QFontMetrics>
#include <QSettings>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QToolBar>

PlaneLog::PlaneLog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlaneLog)
{
    ui->setupUi(this);
    wantedheightofrow=3;
    ui->tableWidget->horizontalHeader()->setAutoScroll(true);

    //cant i just use a qmainwindow 2 times???
    QMainWindow * mainWindow = new QMainWindow();
    mainWindow->setCentralWidget(ui->widget);
    toolBar = new QToolBar();
    toolBar->addAction(ui->actionClear);
    toolBar->addSeparator();
    toolBar->addSeparator();
    toolBar->addAction(ui->actionUpDown);
    toolBar->addAction(ui->actionLeftRight);
    toolBar->addAction(ui->actionStopSorting);
    mainWindow->addToolBar(toolBar);
    QHBoxLayout * layout = new QHBoxLayout();
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(mainWindow);
    setLayout(layout);

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
            if(column<5)newItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            ui->tableWidget->setItem(row, column, newItem);
        }
    }

}

void PlaneLog::closeEvent(QCloseEvent *event)
{
    if(toolBar)toolBar->setHidden(true);
    event->accept();
}

void PlaneLog::showEvent(QShowEvent *event)
{
    toolBar->setHidden(false);
    event->accept();
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

void PlaneLog::ACARSslot(ACARSItem &acarsitem)
{

    if(!acarsitem.valid)return;

    ui->tableWidget->setSortingEnabled(false);//!!!!!


    int rows = ui->tableWidget->rowCount();
    QString AESIDstr=((QString)"").sprintf("%06X",acarsitem.isuitem->AESID);
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
    QTableWidgetItem *FirstHearditem;
    QTableWidgetItem *LastHearditem;
    QTableWidgetItem *Countitem;
    QTableWidgetItem *LastMessageitem;
    if(!found)
    {
        idx=ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(idx);
        QFontMetrics fm(ui->tableWidget->font());
        ui->tableWidget->setRowHeight(idx,fm.height()*wantedheightofrow);
        AESitem = new QTableWidgetItem;
        REGitem = new QTableWidgetItem;
        FirstHearditem = new QTableWidgetItem;
        LastHearditem = new QTableWidgetItem;
        Countitem = new QTableWidgetItem;
        LastMessageitem = new QTableWidgetItem;
        ui->tableWidget->setItem(idx,0,AESitem);
        ui->tableWidget->setItem(idx,1,REGitem);
        ui->tableWidget->setItem(idx,2,FirstHearditem);
        ui->tableWidget->setItem(idx,3,LastHearditem);
        ui->tableWidget->setItem(idx,4,Countitem);
        ui->tableWidget->setItem(idx,5,LastMessageitem);
        AESitem->setText(AESIDstr);
        Countitem->setText("0");

        AESitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        REGitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        Countitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        FirstHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        LastHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

        FirstHearditem->setText(QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss"));
    }
    AESitem = ui->tableWidget->item(idx, 0);
    REGitem = ui->tableWidget->item(idx, 1);
    FirstHearditem = ui->tableWidget->item(idx, 2);
    LastHearditem = ui->tableWidget->item(idx, 3);
    Countitem = ui->tableWidget->item(idx, 4);
    LastMessageitem = ui->tableWidget->item(idx, 5);

    REGitem->setText(acarsitem.PLANEREG);
    LastHearditem->setText(QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss"));

    Countitem->setText(QString::number(Countitem->text().toInt()+1));

    QString tmp=LastMessageitem->text();
    if(tmp.count('\n')>=10)
    {
        int idx=tmp.indexOf("\n");
        tmp=tmp.right(tmp.size()-idx-1);
    }

    if(!acarsitem.message.isEmpty())
    {
        if(!LastMessageitem->text().isEmpty())tmp+="\n";
        LastMessageitem->setText(tmp+"✈: "+acarsitem.message);
    }


    ui->tableWidget->setSortingEnabled(true);//allow sorting again
}


    //you can add images like this
    //QPixmap pixmap( ":/images/clear.png" );
    //ui->tableWidget->item(0, 0)->setData(Qt::DecorationRole, pixmap);


void PlaneLog::on_actionClear_triggered()
{
    ui->tableWidget->clearContents();
    for(int rows = 0; ui->tableWidget->rowCount(); rows++)ui->tableWidget->removeRow(0);
}

void PlaneLog::on_actionUpDown_triggered()
{
    QFontMetrics fm(ui->tableWidget->font());
    int big=fm.height()*12;
    int small=fm.height()*3;

    //if adjusted by user then restore
    int rows = ui->tableWidget->rowCount();
    if(rows<1)return;
    double ave=0;
    for(int i = 0; i < rows; i++)
    {
        ave+=(double)ui->tableWidget->rowHeight(i);
    }
    ave=ave/((double)rows);
    double val=qMin(qAbs(ave-(double)big),qAbs(ave-(double)small));
    if(val>1.0)
    {
        int rows = ui->tableWidget->rowCount();
        for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
        return;
    }

    if(wantedheightofrow>5)
    {
        wantedheightofrow=3;
        QFontMetrics fm(ui->tableWidget->font());
        int rows = ui->tableWidget->rowCount();
        for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
    }
     else
     {
        wantedheightofrow=12;
        QFontMetrics fm(ui->tableWidget->font());
        int rows = ui->tableWidget->rowCount();
        for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
     }
}

void PlaneLog::on_actionLeftRight_triggered()
{
    QFontMetrics fm(ui->tableWidget->font());
    int big=fm.width('_')*(220+20);
    int small=fm.width('_')*(10);
    if(ui->tableWidget->columnWidth(ui->tableWidget->columnCount()-1)>(big-2))
    {
        ui->tableWidget->setColumnWidth(ui->tableWidget->columnCount()-1,small);
        ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    }
     else ui->tableWidget->setColumnWidth(ui->tableWidget->columnCount()-1,big);
}

void PlaneLog::on_actionStopSorting_triggered()
{
    ui->tableWidget->sortItems(-1);
}
