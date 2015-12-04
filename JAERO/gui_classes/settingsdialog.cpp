#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QFile>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->Transmission));//these are transmission settings
    populatesettings();
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

    msgdisplayformat=ui->comboBoxDisplayformat->currentText();
    dropnontextmsgs=ui->checkBoxdropnontextmsgs->isChecked();
    donotdisplaysus.clear();
    QRegExp rx("([\\da-fA-F]+)");
    int pos = 0;
    while ((pos = rx.indexIn(ui->lineEditdonotdisplaysus->text(), pos)) != -1)
    {
        bool ok = false;
        uint value = rx.cap(1).toUInt(&ok,16);
        if(ok)donotdisplaysus.push_back(value);
        pos += rx.matchedLength();
    }

    audioinputdevice=QAudioDeviceInfo::defaultInputDevice();
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        if(deviceInfo.deviceName()==ui->comboBoxsoundcard->currentText())
        {
            audioinputdevice=deviceInfo;
            break;
        }
    }

    loggingdirectory=ui->lineEditlogdir->text();
    loggingenable=ui->checkBoxlogenable->isChecked();
    widebandwidthenable=ui->checkBoxlogwidebandwidthenable->isChecked();

}


void SettingsDialog::populatesettings()
{

    //populate soundcard
    ui->comboBoxsoundcard->clear();
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        ui->comboBoxsoundcard->addItem(deviceInfo.deviceName());

    //load settings
    QSettings settings("Jontisoft", "JAEROL");
    ui->doubleSpinboxminpreamble->setValue(settings.value("doubleSpinboxminpreamble",1.5).toDouble());
    ui->spinBoxTXFreq->setValue(settings.value("spinBoxTXFreq",1000).toInt());
    ui->linePreamble->setText(settings.value("linePreamble","\\r|\\aNOCALL\\a \\aNOCALL\\a \\aNOCALL\\a\\n").toString());
    ui->linePostamble->setText(settings.value("linePostamble","\\n\\a73 NOCALL\\a\\n").toString());
    ui->comboBoxDisplayformat->setCurrentIndex(settings.value("comboBoxDisplayformat",2).toInt());
    ui->lineEditdonotdisplaysus->setText(settings.value("lineEditdonotdisplaysus","71 18 19").toString());
    ui->checkBoxdropnontextmsgs->setChecked(settings.value("checkBoxdropnontextmsgs",true).toBool());
    ui->comboBoxsoundcard->setCurrentText(settings.value("comboBoxsoundcard","").toString());
    ui->lineEditlogdir->setText(settings.value("lineEditlogdir",QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]+"/JAERO").toString());
    ui->checkBoxlogenable->setChecked(settings.value("checkBoxlogenable",false).toBool());
    ui->checkBoxlogwidebandwidthenable->setChecked(settings.value("checkBoxlogwidebandwidthenable",false).toBool());

    on_lineEditlogdir_editingFinished();

    poulatepublicvars();
}


void SettingsDialog::accept()
{    

    //save settings
    QSettings settings("Jontisoft", "JAEROL");
    settings.setValue("doubleSpinboxminpreamble", ui->doubleSpinboxminpreamble->value());
    settings.setValue("spinBoxTXFreq", ui->spinBoxTXFreq->value());
    settings.setValue("linePreamble", ui->linePreamble->text());
    settings.setValue("linePostamble", ui->linePostamble->text());
    settings.setValue("comboBoxDisplayformat", ui->comboBoxDisplayformat->currentIndex());
    settings.setValue("lineEditdonotdisplaysus", ui->lineEditdonotdisplaysus->text());
    settings.setValue("checkBoxdropnontextmsgs", ui->checkBoxdropnontextmsgs->isChecked());
    settings.setValue("comboBoxsoundcard", ui->comboBoxsoundcard->currentText());
    settings.setValue("lineEditlogdir", ui->lineEditlogdir->text());
    settings.setValue("checkBoxlogenable", ui->checkBoxlogenable->isChecked());
    settings.setValue("checkBoxlogwidebandwidthenable", ui->checkBoxlogwidebandwidthenable->isChecked());

    poulatepublicvars();
    QDialog::accept();
}


void SettingsDialog::on_lineEditlogdir_editingFinished()
{
    QFile file(ui->lineEditlogdir->text());
    ui->lineEditlogdir->setText(file.fileName());
}
