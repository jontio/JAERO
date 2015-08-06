#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QLabel>
#include "audiomskdemodulator.h"
#include "varicodepipedecoder.h"

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
};

#endif // MAINWINDOW_H
