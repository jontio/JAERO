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
    Preamble=ui->linePreamble->text();
    Preamble.replace("\\n","\n");
    Postamble=ui->linePostamble->text();
    Postamble.replace("\\n","\n");
    Serialportname=ui->comboBoxPTTdevice->currentText();


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
    ui->spinBoxTXFreq->setValue(settings.value("spinBoxTXFreq",1000).toInt());
    ui->linePreamble->setText(settings.value("linePreamble","NOCALL NOCALL NOCALL NOCALL NOCALL\\n").toString());
    ui->linePostamble->setText(settings.value("linePostamble","\\n73 NOCALL\\n").toString());
    ui->comboBoxPTTdevice->setEditable(false);
    ui->comboBoxPTTdevice->setCurrentText(settings.value("comboBoxPTTdevice","None").toString());

    poulatepublicvars();
}


void SettingsDialog::accept()
{
    //save settings
    QSettings settings("Jontisoft", "JMSK");
    settings.setValue("spinBoxTXFreq", ui->spinBoxTXFreq->value());
    settings.setValue("linePreamble", ui->linePreamble->text());
    settings.setValue("linePostamble", ui->linePostamble->text());
    settings.setValue("comboBoxPTTdevice", ui->comboBoxPTTdevice->currentText());
    poulatepublicvars();
    QDialog::accept();
}
