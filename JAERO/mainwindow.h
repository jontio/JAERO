#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QLabel>
#include "audiomskdemodulator.h"
#include "audiomskmodulator.h"
#include "varicodepipedecoder.h"
#include "varicodepipeencoder.h"
#include "gui_classes/settingsdialog.h"
#include "aerol.h"
#include "gui_classes/planelog.h"
#include <QFile>

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

    AeroL *aerol;

    //modulator
    AudioMskModulator *audiomskmodulator;
    AudioMskModulator::Settings audiomskmodulatorsettings;
    VariCodePipeEncoder *varicodepipeencoder;

    SettingsDialog *settingsdialog;

    void acceptsettings();

    PlaneLog *planelog;

    void log(QString &text);
    QFile filelog;
    QTextStream outlogstream;

protected:
    void closeEvent(QCloseEvent *event);


private slots:
    void DataCarrierDetectStatusSlot(bool dcd);
    void SignalStatusSlot(bool signal);
    void EbNoSlot(double EbNo);
    void WarningTextSlot(QString warning);
    void PeakVolumeSlot(double Volume);
    void PlottablesSlot(double freq_est,double freq_center,double bandwidth);
    void AboutSlot();
    void on_comboBoxbps_currentIndexChanged(const QString &arg1);
    void on_comboBoxlbw_currentIndexChanged(const QString &arg1);
    void on_comboBoxafc_currentIndexChanged(const QString &arg1);
    void on_actionCleanConsole_triggered();
    void on_comboBoxdisplay_currentIndexChanged(const QString &arg1);
    void on_actionConnectToUDPPort_toggled(bool arg1);
    void on_actionRawOutput_triggered();
    void on_action_Settings_triggered();
    void on_action_PlaneLog_triggered();
    void ACARSslot(ACARSItem &acarsitem);
    void ERRorslot(QString &error);
};

#endif // MAINWINDOW_H
