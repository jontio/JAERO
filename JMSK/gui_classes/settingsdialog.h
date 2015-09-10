#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSerialPortInfo>
#include "../audiomskmodulator.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    enum Device{AUDIO,JDDS};
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    void populatesettings();

    int beaconminidle;
    int beaconmaxidle;
    Device  modulatordevicetype;
    QString Serialportname;
    QString Preamble;
    QString Preamble1;
    QString Preamble2;
    QString Postamble;
    AudioMskModulator::Settings audiomskmodulatorsettings;

private:
    Ui::SettingsDialog *ui;    
    void poulatepublicvars();
protected:
    void accept();

private slots:
    void on_spinBoxbeaconmaxidle_editingFinished();
    void on_spinBoxbeaconminidle_editingFinished();
};

#endif // SETTINGSDIALOG_H
