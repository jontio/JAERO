#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QDebug>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::poulatepublicvars()
{
    audiomskmodulatorsettings.freq_center=ui->spinBoxTXFreq->value();
    audiomskmodulatorsettings.secondsbeforereadysignalemited=ui->doubleSpinboxminpreamble->value();
    Preamble=ui->linePreamble->text();
    Preamble.replace("\\n","\n");
    Preamble.replace("\\s",QChar(22));
    Preamble.replace("\\0",QChar(0));
    Preamble.replace("\\1",QChar(12));
    Preamble.replace("\\a",QChar(7));
    QStringList preambles = Preamble.split("|");
    Preamble2.clear();
    Preamble1=preambles.at(0);
    if(Preamble1.indexOf("\\r")>=0){Preamble2+=QChar(22);Preamble2+=QChar(22);}
    Preamble1.replace("\\r",QChar(18));
    if(preambles.size()>1)Preamble2+=preambles.at(1);
    Postamble=ui->linePostamble->text();
    Postamble.replace("\\n","\n");
    Postamble.replace("\\s",QChar(22));
    Postamble.replace("\\0",QChar(0));
    Postamble.replace("\\1",QChar(12));
    Postamble.replace("\\a",QChar(7));
    Serialportname=ui->comboBoxPTTdevice->currentText();
    modulatordevicetype=AUDIO;
    if(ui->comboBoxJDDS->currentText()=="PTT")modulatordevicetype=AUDIO;
    if(ui->comboBoxJDDS->currentText()=="JDDS")modulatordevicetype=JDDS;
    beaconminidle=ui->spinBoxbeaconminidle->value();
    beaconmaxidle=ui->spinBoxbeaconmaxidle->value();
    scrapeingenabled=ui->checkBoxenablescrapeing->isChecked();

    //put scraping into the format needed by the webscraper
    scrapemapcontainer.scrapemap.clear();
    ScrapeItem scrapeitem;
    QString scrapeid;
    for(int row=0;row<ui->tableWidgetscrapings->rowCount();row++)
    {
        for(int column=0;column<ui->tableWidgetscrapings->columnCount();column++)
        {
            QTableWidgetItem *tableitem = ui->tableWidgetscrapings->item(row,column);
            if(!tableitem)continue;
            switch(column)
            {
            case SCRAPE_ID:
                scrapeid=tableitem->text();
                break;
            case SCRAPE_URL:
                scrapeitem.url=tableitem->text();
                break;
            case SCRAPE_REGEXPR:
                scrapeitem.rx.setPattern(tableitem->text());
                break;
            case SCRAPE_MINIMAL:
                if(tableitem->text().trimmed().toUpper()=="YES")scrapeitem.rx.setMinimal(true);
                 else scrapeitem.rx.setMinimal(false);
                break;
            case SCRAPE_REFRESHVALUE:
                scrapeitem.refreshintervalsecs=tableitem->text().trimmed().toInt();
                break;
            case SCRAPE_DROP_HTMLTAGS:
                if(tableitem->text().trimmed().toUpper()=="YES")scrapeitem.removehtmltags=true;
                 else scrapeitem.removehtmltags=false;
                break;
            case SCRAPE_DEFAULTVALUE:
                scrapeitem.valueifmissing=tableitem->text();
                break;
            }
        }
        scrapemapcontainer.scrapemap.insert(scrapeid,scrapeitem);
    }


}


void SettingsDialog::populatesettings()
{

    //populate serial ports
    ui->comboBoxPTTdevice->clear();
    ui->comboBoxPTTdevice->addItem("None");
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->comboBoxPTTdevice->addItem(info.portName());
    }

    //load settings
    QSettings settings("Jontisoft", "JMSK");
    ui->doubleSpinboxminpreamble->setValue(settings.value("doubleSpinboxminpreamble",1.5).toDouble());
    ui->spinBoxTXFreq->setValue(settings.value("spinBoxTXFreq",1000).toInt());
    ui->linePreamble->setText(settings.value("linePreamble","\\r|\\aNOCALL\\a \\aNOCALL\\a \\aNOCALL\\a\\n").toString());
    ui->linePostamble->setText(settings.value("linePostamble","\\n\\a73 NOCALL\\a\\n").toString());
    ui->comboBoxPTTdevice->setEditable(false);
    ui->comboBoxPTTdevice->setCurrentText(settings.value("comboBoxPTTdevice","None").toString());
    ui->comboBoxJDDS->setCurrentText(settings.value("comboBoxJDDS","PTT").toString());
    ui->spinBoxbeaconminidle->setValue(settings.value("spinBoxbeaconminidle",110).toInt());
    ui->spinBoxbeaconmaxidle->setValue(settings.value("spinBoxbeaconmaxidle",130).toInt());
    ui->checkBoxenablescrapeing->setChecked(settings.value("checkBoxenablescrapeing",false).toBool());

    //load scrapings
    ui->tableWidgetscrapings->setRowCount(settings.value("scrapings-rows",0).toInt());
    for(int row=0;row<ui->tableWidgetscrapings->rowCount();row++)
    {
        for(int column=0;column<ui->tableWidgetscrapings->columnCount();column++)
        {
            QString str=((QString)"scrapings-%1-%2").arg(row).arg(column);
            QTableWidgetItem *newItem = new QTableWidgetItem(settings.value(str,"").toString());
            ui->tableWidgetscrapings->setItem(row, column, newItem);
        }
    }

    poulatepublicvars();
}


void SettingsDialog::accept()
{    

    //save settings
    QSettings settings("Jontisoft", "JMSK");
    settings.setValue("doubleSpinboxminpreamble", ui->doubleSpinboxminpreamble->value());
    settings.setValue("spinBoxTXFreq", ui->spinBoxTXFreq->value());
    settings.setValue("linePreamble", ui->linePreamble->text());
    settings.setValue("linePostamble", ui->linePostamble->text());
    settings.setValue("comboBoxPTTdevice", ui->comboBoxPTTdevice->currentText());
    settings.setValue("comboBoxJDDS", ui->comboBoxJDDS->currentText());
    settings.setValue("spinBoxbeaconminidle", ui->spinBoxbeaconminidle->value());
    settings.setValue("spinBoxbeaconmaxidle", ui->spinBoxbeaconmaxidle->value());
    settings.setValue("checkBoxenablescrapeing", ui->checkBoxenablescrapeing->isChecked());



    //save scrapings
    settings.setValue("scrapings-rows",ui->tableWidgetscrapings->rowCount());
    for(int row=0;row<ui->tableWidgetscrapings->rowCount();row++)
    {
        for(int column=0;column<ui->tableWidgetscrapings->columnCount();column++)
        {
            QString str=((QString)"scrapings-%1-%2").arg(row).arg(column);
            if(ui->tableWidgetscrapings->item(row,column))settings.setValue(str,ui->tableWidgetscrapings->item(row,column)->text());
        }
    }


    poulatepublicvars();
    QDialog::accept();
}

void SettingsDialog::on_spinBoxbeaconmaxidle_editingFinished()
{
    if(ui->spinBoxbeaconmaxidle->value()<ui->spinBoxbeaconminidle->value())ui->spinBoxbeaconminidle->setValue(ui->spinBoxbeaconmaxidle->value());
}

void SettingsDialog::on_spinBoxbeaconminidle_editingFinished()
{
    if(ui->spinBoxbeaconmaxidle->value()<ui->spinBoxbeaconminidle->value())ui->spinBoxbeaconmaxidle->setValue(ui->spinBoxbeaconminidle->value());
}

void SettingsDialog::on_pushButtonadd_clicked()
{
    ui->tableWidgetscrapings->setRowCount(ui->tableWidgetscrapings->rowCount()+1);
}

void SettingsDialog::on_pushButtondel_clicked()
{
    ui->tableWidgetscrapings->removeRow(ui->tableWidgetscrapings->currentRow());
}
