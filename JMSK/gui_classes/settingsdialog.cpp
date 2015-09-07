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
    poulatepublicvars();
    QDialog::accept();
}
