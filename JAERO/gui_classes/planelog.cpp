#include "planelog.h"
#include "ui_planelog.h"

#include <QDateTime>
#include <QDebug>
#include <QFontMetrics>
#include <QSettings>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStandardPaths>
#include <QFileDialog>
#include <QImage>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>
#include <QClipboard>
#include <QScrollBar>
#include <QMessageBox>
#include <QMenuBar>
#include "settingsdialog.h"

void PlaneLog::imageUpdateslot(const QPixmap &image)
{
    ui->toolButtonimg->setIcon(image);
}

void PlaneLog::dbUpdateslot(bool ok, int ref, const QStringList &dbitem)
{
    Q_UNUSED(ref);
    ui->label_type->clear();
    ui->label_owner->clear();

    if(!ok)
    {
        if(dbitem.size())
        {
             dbUpdateerrorslot(dbitem[0]);
        } else dbUpdateerrorslot("Error: Unknown");
    }
    else
    {

        if(dbitem.size()!=5)
        {
            dbUpdateerrorslot("Error: Database illformed");
            return;
        }


        if(!selectedAESitem)return;
        if(selectedAESitem->row()<0)return;
        if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
        if(selectedAESitem->text()!=dbitem[0])return;
        int row=selectedAESitem->row();

        //if sat reg and lookup reg are very different then lookup is wrong
        QTableWidgetItem *Notesitem;
        QTableWidgetItem *REGitem;
        Notesitem = ui->tableWidget->item(row, 7);
        REGitem = ui->tableWidget->item(row, 1);
        if(REGitem->text().right(2).toLower()!=dbitem[1].right(2).toLower())
        {
            if(Notesitem->text().right(1)=="\u2063")ui->plainTextEditnotes->clear();
            dbUpdateerrorslot("Database and sat regs differ.\nTry updating database.");
            return;
        }

        ui->label_type->setText(dbitem[3]);
        ui->label_owner->setText(dbitem[4]);

        ui->plainTextEditdatabase->clear();
        ui->plainTextEditdatabase->appendPlainText("Reg. ID         \t"+dbitem[1]);
        ui->plainTextEditdatabase->appendPlainText("Model           \t"+dbitem[2]);
        ui->plainTextEditdatabase->appendPlainText("Type            \t"+dbitem[3]);
        ui->plainTextEditdatabase->appendPlainText("Owner           \t"+dbitem[4]);
        //ui->plainTextEditdatabase->appendPlainText("Call Sign (Last)\t"+dbitem[4]);
        //ui->plainTextEditdatabase->appendPlainText("Flight (Last)   \t"+dbitem[5]);
//        QDateTime timestamp;
//        timestamp.setTime_t(((QString)dbitem[8]).toUInt());
//        ui->plainTextEditdatabase->appendPlainText("Updated (Local) \t"+timestamp.toString("yy-MM-dd hh:mm:ss"));

        if((!ui->plainTextEditnotes->toPlainText().isEmpty())&&(Notesitem->text().right(1)!="\u2063"))return;
        ui->plainTextEditnotes->setPlainText(dbitem[3]+"\n"+dbitem[4]+"\n\u2063");
        Notesitem->setText(dbitem[3]+"\n"+dbitem[4]+"\n\u2063");


    }

}

void PlaneLog::dbUpdateerrorslot(const QString &error)
{
    ui->label_type->clear();
    ui->label_owner->clear();
    ui->plainTextEditdatabase->clear();
    ui->plainTextEditdatabase->setPlainText(error);
}

void PlaneLog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatescrollbar();
}

void PlaneLog::updatescrollbar()
{
    double dvertscrollbarval=wantedscrollprop*((double)ui->textEditmessages->verticalScrollBar()->maximum());
    ui->textEditmessages->verticalScrollBar()->setValue(dvertscrollbarval);
}

PlaneLog::PlaneLog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlaneLog)
{
    ui->setupUi(this);
    wantedheightofrow=3;

    ui->label_type->clear();
    ui->label_owner->clear();

    selectedAESitem=NULL;

    planesfolder=((const char*) 0);

    connect(ui->textEditmessages->verticalScrollBar(),SIGNAL(valueChanged(int)),this,SLOT(messagesliderchsnged(int)));

    //create image lookup controller and connect result to us
    ic=new ImageController(this);
    connect(ic,SIGNAL(result(QPixmap)),this,SLOT(imageUpdateslot(QPixmap)));

    //create database lookup controller and connect result to us
    dbc=new DataBaseTextUser(this);
    connect(dbc,SIGNAL(result(bool,int,QStringList)),this,SLOT(dbUpdateslot(bool,int,QStringList)));

    ui->actionLeftRight->setVisible(false);
    ui->actionUpDown->setVisible(false);
    ui->tableWidget->horizontalHeader()->setAutoScroll(true);
    ui->tableWidget->setColumnHidden(5,true);
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));

    //cant i just use a qmainwindow 2 times???
    QMainWindow * mainWindow = new QMainWindow();
    mainWindow->setCentralWidget(ui->widget);
    toolBar = new QToolBar();
    toolBar->addAction(ui->actionClear);
    toolBar->addAction(ui->actionImport_log);
    toolBar->addAction(ui->actionExport_log);
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

    QMenuBar *menubar=new QMenuBar();
    QMenu *menu_file=new QMenu("File");
    menu_file->addAction(ui->actionImport_log);
    menu_file->addAction(ui->actionExport_log);
    menu_file->addAction(ui->actionClear);
    menubar->addMenu(menu_file);
    mainWindow->setMenuBar(menubar);

    connect(toolBar,SIGNAL(topLevelChanged(bool)),this,SLOT(updatescrollbar()));
    connect(toolBar,SIGNAL(visibilityChanged(bool)),this,SLOT(updatescrollbar()));

    //load settings
    QSettings settings("Jontisoft", settings_name);
    QFontMetrics fm(ui->tableWidget->font());
    ui->tableWidget->setRowCount(settings.value("tableWidget-rows",0).toInt());
    for(int row=0;row<ui->tableWidget->rowCount();row++)
    {
        ui->tableWidget->setRowHeight(row,fm.height()*wantedheightofrow);
        for(int column=0;column<ui->tableWidget->columnCount();column++)
        {
            QString str=((QString)"tableWidget-%1-%2").arg(row).arg(column);
            QTableWidgetItem *newItem = new QTableWidgetItem(settings.value(str,"").toString());
            if(column<7)newItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            //if(column<7)
            newItem->setFlags((newItem->flags()&~Qt::ItemIsEditable)|Qt::ItemIsSelectable);
            ui->tableWidget->setItem(row, column, newItem);
        }
    }
    ui->tableWidget->selectRow(0);
    ui->splitter_2->restoreState(settings.value("splitter_2").toByteArray());
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    restoreGeometry(settings.value("logwindow").toByteArray());
    wantedscrollprop=settings.value("wantedscrollprop",1.0).toDouble();

    ui->splitter_2->setStretchFactor(0, 2);
    ui->splitter_2->setStretchFactor(1, 2);
    ui->splitter_2->setStretchFactor(2, 10);

}

void PlaneLog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}

void PlaneLog::showEvent(QShowEvent *event)
{
    updateinfopain();
    event->accept();
}


PlaneLog::~PlaneLog()
{

    //save settings
    //other options could be then easy to backup if need be
    //QSettings settings(QSettings::IniFormat, QSettings::UserScope,"Jontisoft", "JAERO");
    //QSettings settings(afilename,QSettings::IniFormat);
    QSettings settings("Jontisoft", settings_name);
    settings.setValue("tableWidget-rows",ui->tableWidget->rowCount());
    for(int row=0;row<ui->tableWidget->rowCount();row++)
    {
        for(int column=0;column<ui->tableWidget->columnCount();column++)
        {
            QString str=((QString)"tableWidget-%1-%2").arg(row).arg(column);
            if(ui->tableWidget->item(row,column))settings.setValue(str,ui->tableWidget->item(row,column)->text());
        }
    }
    settings.setValue("splitter", ui->splitter->saveState());
    settings.setValue("splitter_2", ui->splitter_2->saveState());
    settings.setValue("logwindow", saveGeometry());
    settings.setValue("wantedscrollprop",wantedscrollprop);


    delete ui;    
}

void PlaneLog::ACARSslot(ACARSItem &acarsitem)
{
    if(!acarsitem.valid)return;

    ui->tableWidget->setSortingEnabled(false);//!!!!!

    int rows = ui->tableWidget->rowCount();
    QString AESIDstr=((QString)"").sprintf("%06X",acarsitem.isuitem.AESID);
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
    QTableWidgetItem *MessageCountitem;
    QTableWidgetItem *Notesitem;
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
        MessageCountitem = new QTableWidgetItem;
        Notesitem = new QTableWidgetItem;
        ui->tableWidget->setItem(idx,0,AESitem);
        ui->tableWidget->setItem(idx,1,REGitem);
        ui->tableWidget->setItem(idx,2,FirstHearditem);
        ui->tableWidget->setItem(idx,3,LastHearditem);
        ui->tableWidget->setItem(idx,4,Countitem);
        ui->tableWidget->setItem(idx,5,LastMessageitem);
        ui->tableWidget->setItem(idx,6,MessageCountitem);
        ui->tableWidget->setItem(idx,7,Notesitem);
        AESitem->setText(AESIDstr);
        Countitem->setText("0");
        MessageCountitem->setText("0");

        AESitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        REGitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        Countitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        FirstHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        LastHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        MessageCountitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

        AESitem->setFlags(AESitem->flags()&~Qt::ItemIsEditable);
        REGitem->setFlags(REGitem->flags()&~Qt::ItemIsEditable);
        Countitem->setFlags(Countitem->flags()&~Qt::ItemIsEditable);
        FirstHearditem->setFlags(FirstHearditem->flags()&~Qt::ItemIsEditable);
        LastHearditem->setFlags(LastHearditem->flags()&~Qt::ItemIsEditable);
        Notesitem->setFlags(Notesitem->flags()&~Qt::ItemIsEditable);

        FirstHearditem->setText(QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss"));
    }
    AESitem = ui->tableWidget->item(idx, 0);
    REGitem = ui->tableWidget->item(idx, 1);
    FirstHearditem = ui->tableWidget->item(idx, 2);
    LastHearditem = ui->tableWidget->item(idx, 3);
    Countitem = ui->tableWidget->item(idx, 4);
    LastMessageitem = ui->tableWidget->item(idx, 5);
    MessageCountitem = ui->tableWidget->item(idx, 6);

    REGitem->setText(acarsitem.PLANEREG);
    LastHearditem->setText(QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss"));

    Countitem->setText(QString::number(Countitem->text().toInt()+1));

    QString tmp=LastMessageitem->text();
    if(tmp.count('\n')>=29)
    {
        int idx=tmp.indexOf("\n");
        tmp=tmp.right(tmp.size()-idx-1);
    }

   /* if(acarsitem.downlink&&arincparser.parseDownlinkmessage(message))//if we are on a downlink then process downlink message
    {
        if(!arincparser.flightid.isEmpty())humantext+=" Flight "+arincparser.flightid;
        if(arincparser.info.size()>2)
        {
            arincparser.info.replace("\n","\n\t");
            humantext+="\n\n\t"+message+"\n\n\t"+arincparser.info;
        }
         else humantext+="\n\n\t"+message+"\n";
    }
     else humantext+="\n\n\t"+message+"\n";
*/
    if(!acarsitem.message.isEmpty())
    {

        MessageCountitem->setText(QString::number(MessageCountitem->text().toInt()+1));
        QString message=acarsitem.message;
        message.replace('\r','\n');
        message.replace("\n\n","\n");
        if(message.right(1)=="\n")message.chop(1);
        if(message.left(1)=="\n")message.remove(0,1);
        arincparser.parseDownlinkmessage(acarsitem);
        arincparser.parseUplinkmessage(acarsitem);
        message.replace('\n',"●");//● instead of \n\t

        QByteArray TAKstr;
        TAKstr+=acarsitem.TAK;
        if(acarsitem.TAK==0x15)TAKstr[0]='!';
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';        

        if(acarsitem.nonacars)tmp+="✈: "+QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ")+(((QString)"").sprintf("AES:%06X GES:%02X   %s       ●●",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.PLANEREG.data()));
         else
         {
            if((acarsitem.downlink)&&!arincparser.downlinkheader.flightid.isEmpty())
            {
                tmp+="✈: "+QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ")+(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c Flight %s●●",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI,arincparser.downlinkheader.flightid.toLatin1().data()));
            }
             else tmp+="✈: "+QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ")+(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c●●",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI));
         }

        if((!acarsitem.nonacars)&&arincparser.arincmessage.info.size()>2)
        {
            arincparser.arincmessage.info.replace("\n","●");
            LastMessageitem->setText(tmp+message+"●●"+arincparser.arincmessage.info);
        }
         else LastMessageitem->setText(tmp+message+"●");

        if(selectedAESitem&&(idx==selectedAESitem->row()))updateinfopain();

    }


    ui->tableWidget->setSortingEnabled(true);//allow sorting again
}

void PlaneLog::on_actionClear_triggered()
{

    /*QTableWidgetItem *Notesitem;
    for(int rows = 0; rows<ui->tableWidget->rowCount(); rows++)
    {
        Notesitem = ui->tableWidget->item(rows, 7);
        Notesitem->setText("");
    }*/

    //confirm
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
    msgBox.setText("This will erase this window log\nAre you sure?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setWindowTitle("Confirm log deletion");
    switch(msgBox.exec())
    {
    case QMessageBox::No:
        return;
        break;
    case QMessageBox::Yes:
        break;
    default:
        return;
        break;
    }

    ui->tableWidget->clearContents();
    for(int rows = 0; ui->tableWidget->rowCount(); rows++){
       ui->tableWidget->removeRow(0);
    }
    ui->toolButtonimg->setIcon(QPixmap(":/images/Plane_clip_art.svg"));
    ui->labelAES->clear();
    ui->labelREG->clear();
    ui->labelfirst->clear();
    ui->labellast->clear();
    ui->label_owner->clear();
    ui->label_type->clear();
    disconnect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->plainTextEditnotes->clear();
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->textEditmessages->clear();
    ui->plainTextEditdatabase->clear();
    selectedAESitem=NULL;


    // remove from registry also, the application gets very slow to load if not removed.
    QSettings settings("Jontisoft", settings_name);
    QStringList keys = settings.childKeys();
    for(int a = 0; a<keys.size(); a++){

        QString key = keys.at(a);

        if(key.startsWith("tableWidget")){
            settings.remove(key);
        }
    }

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
    int big=fm.width('_')*(220+50);
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

void PlaneLog::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousColumn);
    if(currentRow==previousRow)return;
    selectedAESitem=ui->tableWidget->item(currentRow, 0);
    updateinfopain();
}

void PlaneLog::updateinfopain()
{

    if(!selectedAESitem)return;
    if(selectedAESitem->row()<0)return;
    if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
    int row=selectedAESitem->row();

    QTableWidgetItem *LastMessageitem=ui->tableWidget->item(row, 5);
    QTableWidgetItem *AESitem = ui->tableWidget->item(row, 0);
    QTableWidgetItem *REGitem = ui->tableWidget->item(row, 1);
    QTableWidgetItem *FirstHearditem = ui->tableWidget->item(row, 2);
    QTableWidgetItem *LastHearditem = ui->tableWidget->item(row, 3);
    QTableWidgetItem *Notesitem = ui->tableWidget->item(row, 7);

    ui->labelAES->setText(AESitem->text());
    ui->labelREG->setText(REGitem->text());
    ui->labelfirst->setText(FirstHearditem->text());
    ui->labellast->setText(LastHearditem->text());

    QString str=LastMessageitem->text();
    str.replace("●","\n\t");
    str.replace("✈: ","\n");

    //remember scroll val
    if(ui->textEditmessages->verticalScrollBar()->maximum()>0)wantedscrollprop=((double)ui->textEditmessages->verticalScrollBar()->value())/((double)ui->textEditmessages->verticalScrollBar()->maximum());
    ui->textEditmessages->setText(str);
    updatescrollbar();

    //Qt is great. so simple. look, just one line
    ic->asyncImageLookupFromAES(planesfolder,AESitem->text());

    //db lookup request
    ui->label_type->clear();
    ui->label_owner->clear();
    ui->plainTextEditdatabase->clear();
    if(!planesfolder.isNull())dbc->request(planesfolder,AESitem->text(),NULL);

    //old code. blocking
    /*QString imagefilename=imagesfolder+"/"+AESitem->text()+".png";
    if(QFileInfo(imagefilename).exists())ui->toolButtonimg->setIcon(QPixmap(imagefilename));
    else
    {
        imagefilename=imagesfolder+"/"+AESitem->text()+".jpg";
        if(QFileInfo(imagefilename).exists())ui->toolButtonimg->setIcon(QPixmap(imagefilename));
        else ui->toolButtonimg->setIcon(QPixmap(":/images/Plane_clip_art.svg"));
    }*/

    disconnect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->plainTextEditnotes->clear();
    ui->plainTextEditnotes->setPlainText(Notesitem->text());
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));

}

void PlaneLog::messagesliderchsnged(int value)
{
    if(ui->textEditmessages->verticalScrollBar()->maximum()>0)wantedscrollprop=((double)value)/((double)ui->textEditmessages->verticalScrollBar()->maximum());
}

void PlaneLog::on_toolButtonimg_clicked()
{

    if(!selectedAESitem)return;
    if(selectedAESitem->row()<0)return;
    if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
    int row=selectedAESitem->row();

    QTableWidgetItem *AESitem = ui->tableWidget->item(row, 0);
    QTableWidgetItem *REGitem = ui->tableWidget->item(row, 1);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(AESitem->text());

    //if found {REG} is database reg else we guess what it should be from the txed reg that maybe different

    //db reg number
    QString regstr;
    QString str=ui->plainTextEditdatabase->toPlainText();
    QStringList list=str.split('\n');
    if(list.size()>3)
    {
        QRegExp rx("Reg. ID([\\s]*)(.+)");
        if(rx.indexIn(list[0])!=-1)
        {
            regstr=rx.cap(2).toLower();
        }
    }

    /*QString str=ui->plainTextEditdatabase->toPlainText();
    QStringList list=str.split('\n');
    if(list.size()>3)
    {
        QRegExp rx("Reg. ID([\\s]*)(.+)");
        if(rx.indexIn(list[0])!=-1)
        {
            QString url="http://www.flightradar24.com/data/airplanes/"+rx.cap(2).toLower();
            QDesktopServices::openUrl(QUrl(url));
            return;
        }
    }*/

    if(regstr.isEmpty())
    {
        regstr=REGitem->text().toLower().trimmed();
        regstr.replace(".","");
        QRegExp rx("([a-z_0-9]*)");
        if((regstr.size()==7)&&(rx.indexIn(regstr)!=-1))
        {
            if(rx.cap(1).size()==7)
            {
                regstr.insert(2,'-');
            }
        }
    }

    QString url=planelookup;
    url.replace("{AES}",AESitem->text());
    url.replace("{REG}",regstr);
    QDesktopServices::openUrl(QUrl(url));
}

void PlaneLog::contextMenuEvent(QContextMenuEvent *event)
{
    //tablewidget menu
    if(!ui->tableWidget->rect().contains(ui->tableWidget->mapFromGlobal(event->globalPos())))
    {
        event->ignore();
        return;
    }
    QMenu menu(this);
    menu.addAction(ui->actionCopy);
    menu.exec(event->globalPos());
    event->accept();
}

void PlaneLog::on_actionCopy_triggered()
{
    QClipboard *clipboard = QApplication::clipboard();

    if(ui->tableWidget->selectedItems().size())
    {
        clipboard->setText(ui->tableWidget->selectedItems()[0]->text());
    }

    /*QString str;
    foreach(QTableWidgetItem *item,ui->tableWidget->selectedItems())
    {
        str+=(item->text()+"\t");
    }
    str.chop(1);
    clipboard->setText(str);*/
}

void PlaneLog::plainTextEditnotesChanged()
{
    if(!selectedAESitem)return;
    if(selectedAESitem->row()<0)return;
    if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
    int row=selectedAESitem->row();


    QTableWidgetItem *Notesitem = ui->tableWidget->item(row, 7);
    Notesitem->setText(ui->plainTextEditnotes->toPlainText().replace("\u2063",""));
}

void PlaneLog::on_actionExport_log_triggered()
{
    //ask for name
    QSettings settings("Jontisoft", settings_name);
    QString filename=QFileDialog::getSaveFileName(this,tr("Save as"), settings.value("exportimportloc",QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]+"/jaerologwindow.csv").toString(), tr("CSV file (*.csv)"));
    if(filename.isEmpty())return;
    settings.setValue("exportimportloc",filename);

    //open file
    QFile outfile(filename);
    if (!outfile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
        msgBox.setText("Cant open file for saving");
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
    QTextStream outtext(&outfile);
    outtext.setCodec("UTF-8");

    //write file
    for(int row=0;row<ui->tableWidget->rowCount();row++)
    {
        QString line="\"";
        for(int column=0;column<ui->tableWidget->columnCount();column++)
        {
            QString cell;
            if(ui->tableWidget->item(row,column))cell=ui->tableWidget->item(row,column)->text();

            //" escape
            cell.replace("\"","\\\"");

            //notes replacement
            if(column==7)
            {
                cell.remove("\r");
                cell.replace("\n","●");
            }

            line+=cell.trimmed()+"\",\"";
        }
        line.chop(2);
        line+="\n";
        outtext<<line;
    }


}

void PlaneLog::on_actionImport_log_triggered()
{

    //ask for name
    QSettings settings("Jontisoft", settings_name);
    QString filename=QFileDialog::getOpenFileName(this,tr("Open file"), settings.value("exportimportloc",QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]+"/jaerologwindow.csv").toString(), tr("CSV file (*.csv)"));
    if(filename.isEmpty())return;
    settings.setValue("exportimportloc",filename);

    //open file
    QFile infile(filename);
    if (!infile.open(QIODevice::ReadOnly| QIODevice::Text))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
        msgBox.setText("Cant open file for reading");
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
    QTextStream intext(&infile);
    intext.setCodec("UTF-8");

    //confirm
    if(ui->tableWidget->rowCount())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
        msgBox.setText("This will replace this window log\nAre you sure?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setWindowTitle("Confirm log replacement");
        switch(msgBox.exec())
        {
        case QMessageBox::No:
            return;
            break;
        case QMessageBox::Yes:
            break;
        default:
            return;
            break;
        }
    }

    //clear everything
    ui->tableWidget->clearContents();
    for(int rows = 0; ui->tableWidget->rowCount(); rows++)ui->tableWidget->removeRow(0);
    ui->toolButtonimg->setIcon(QPixmap(":/images/Plane_clip_art.svg"));
    ui->labelAES->clear();
    ui->labelREG->clear();
    ui->labelfirst->clear();
    ui->labellast->clear();
    ui->label_owner->clear();
    ui->label_type->clear();
    disconnect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->plainTextEditnotes->clear();
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->textEditmessages->clear();
    ui->plainTextEditdatabase->clear();
    selectedAESitem=NULL;

    //look at all rows in the file
    QTableWidgetItem *AESitem;
    QTableWidgetItem *REGitem;
    QTableWidgetItem *FirstHearditem;
    QTableWidgetItem *LastHearditem;
    QTableWidgetItem *Countitem;
    QTableWidgetItem *LastMessageitem;
    QTableWidgetItem *MessageCountitem;
    QTableWidgetItem *Notesitem;
    int ec=0;
    while (!intext.atEnd())
    {

        //read row
        QString line = intext.readLine();
        line.chop(1);
        line.remove(0,1);
        line.replace("\\\"","\x10");
        QStringList items=line.split("\",\"");
        if(items.size()<8)
        {
            ec++;
            if(ec<20)qDebug()<<"csv col count to small ("<<items.size()<<")";
            continue;
        }
        for(int i=0;i<items.size();i++)items[i].replace("\x10","\"");

        //insert row
        int idx=ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(idx);
        QFontMetrics fm(ui->tableWidget->font());
        ui->tableWidget->setRowHeight(idx,fm.height()*wantedheightofrow);
        AESitem = new QTableWidgetItem;
        REGitem = new QTableWidgetItem;
        FirstHearditem = new QTableWidgetItem;
        LastHearditem = new QTableWidgetItem;
        Countitem = new QTableWidgetItem;
        LastMessageitem = new QTableWidgetItem;
        MessageCountitem = new QTableWidgetItem;
        Notesitem = new QTableWidgetItem;
        ui->tableWidget->setItem(idx,0,AESitem);
        ui->tableWidget->setItem(idx,1,REGitem);
        ui->tableWidget->setItem(idx,2,FirstHearditem);
        ui->tableWidget->setItem(idx,3,LastHearditem);
        ui->tableWidget->setItem(idx,4,Countitem);
        ui->tableWidget->setItem(idx,5,LastMessageitem);
        ui->tableWidget->setItem(idx,6,MessageCountitem);
        ui->tableWidget->setItem(idx,7,Notesitem);
        AESitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        REGitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        Countitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        FirstHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        LastHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        MessageCountitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        AESitem->setFlags(AESitem->flags()&~Qt::ItemIsEditable);
        REGitem->setFlags(REGitem->flags()&~Qt::ItemIsEditable);
        Countitem->setFlags(Countitem->flags()&~Qt::ItemIsEditable);
        FirstHearditem->setFlags(FirstHearditem->flags()&~Qt::ItemIsEditable);
        LastHearditem->setFlags(LastHearditem->flags()&~Qt::ItemIsEditable);
        Notesitem->setFlags(Notesitem->flags()&~Qt::ItemIsEditable);
        AESitem->setText(items[0]);
        REGitem->setText(items[1]);
        FirstHearditem->setText(items[2]);
        LastHearditem->setText(items[3]);
        Countitem->setText(items[4]);
        LastMessageitem->setText(items[5]);
        MessageCountitem->setText(items[6]);
        items[7].replace("●","\n");
        Notesitem->setText(items[7].trimmed());

    }


}
