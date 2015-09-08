#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QLabel>
#include "audiomskdemodulator.h"
#include "audiomskmodulator.h"
#include "ddsmskmodulator/ddsmskmodulator.h"
#include "varicodepipedecoder.h"
#include "varicodepipeencoder.h"
#include "serialppt.h"
#include "gui_classes/settingsdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    AudioMskDemodulator *audiomskdemodulator;
    AudioMskDemodulator::Settings audiomskdemodulatorsettings;
    QLabel *ebnolabel;
    QLabel *freqlabel;
    QUdpSocket *udpsocket;
    VariCodePipeDecoder *varicodepipedecoder;

    //modulator
    AudioMskModulator *audiomskmodulator;
    DDSMSKModulator *ddsmskmodulator;
    DDSMSKModulator::Settings ddsmskmodulatorsettings;
    AudioMskModulator::Settings audiomskmodulatorsettings;
    VariCodePipeEncoder *varicodepipeencoder;
    SerialPPT *serialPPT;

    SettingsDialog *settingsdialog;

    SettingsDialog::Device modulatordevicetype;

    void connectmodulatordevice(SettingsDialog::Device device);
    void setSerialUser(SettingsDialog::Device device);

private slots:
    void SignalStatusSlot(bool signal);
    void EbNoSlot(double EbNo);
    void WarningTextSlot(QString warning);
    void PeakVolumeSlot(double Volume);
    void PlottablesSlot(double freq_est,double freq_center,double bandwidth);
    void AboutSlot();
    void on_comboBoxbps_currentIndexChanged(const QString &arg1);
    void on_comboBoxlbw_currentIndexChanged(const QString &arg1);
    void on_comboBoxsql_currentIndexChanged(const QString &arg1);
    void on_comboBoxafc_currentIndexChanged(const QString &arg1);
    void on_actionCleanConsole_triggered();
    void on_comboBoxdisplay_currentIndexChanged(const QString &arg1);
    void on_actionConnectToUDPPort_toggled(bool arg1);
    void on_actionRawOutput_triggered();
    void on_action_Settings_triggered();
};

#endif // MAINWINDOW_H
