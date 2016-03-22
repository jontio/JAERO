#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QLabel>
#include "audiooqpskdemodulator.h"
#include "audiomskdemodulator.h"
#include "audioburstoqpskdemodulator.h"
#include "gui_classes/settingsdialog.h"
#include "aerol.h"
#include "gui_classes/planelog.h"
#include <QFile>
#include <QSound>
#include "sbs1.h"

#include "databasetext.h"

#include "arincparse.h"

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
    enum DemodType{NoDemodType,MSK,OQPSK,BURSTOQPSK};
    Ui::MainWindow *ui;
    AudioMskDemodulator *audiomskdemodulator;
    AudioMskDemodulator::Settings audiomskdemodulatorsettings;
    QLabel *ebnolabel;
    QLabel *freqlabel;
    QUdpSocket *udpsocket;

    //OQPSK add
    AudioOqpskDemodulator *audiooqpskdemodulator;
    AudioOqpskDemodulator::Settings audiooqpskdemodulatorsettings;
    //

    //Burst OQPSK add
    AudioBurstOqpskDemodulator *audioburstoqpskdemodulator;
    AudioBurstOqpskDemodulator::Settings audioburstoqpskdemodulatorsettings;
    //

    //bottom textedit output
    QUdpSocket *udpsocket_bottom_textedit;
    //

    AeroL *aerol;
    AeroL *aerol2;

    SettingsDialog *settingsdialog;

    SBS1 *sbs1;

    void acceptsettings();

    PlaneLog *planelog;

    void log(QString &text);
    QFile filelog;
    QTextStream outlogstream;

    DataBaseTextUser *dbtu;

    DemodType typeofdemodtouse;

    void selectdemodulatorconnections(DemodType demodtype);

    ArincParse arincparser;

    QSound *beep;
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

   // void result(bool ok, int ref, const QStringList &result);
};

#endif // MAINWINDOW_H
