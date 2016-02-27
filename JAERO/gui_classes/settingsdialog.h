#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtGlobal>
#include <QHostAddress>

#if defined(Q_OS_UNIX) || defined(Q_OS_LUNX)
#define APPDATALOCATIONS QStandardPaths::AppDataLocation
#else
#define APPDATALOCATIONS QStandardPaths::AppLocalDataLocation
#endif

#include <QDialog>
#include <QVector>
#include <QAudioDeviceInfo>
#include "../downloadmanager.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    void populatesettings();

    QAudioDeviceInfo audioinputdevice;
    QVector<int> donotdisplaysus;
    bool dropnontextmsgs;
    QString msgdisplayformat;
    bool loggingenable;
    QString loggingdirectory;
    bool widebandwidthenable;

    QString planesfolder;
    QString planelookup;
    bool beepontextmessage;
    void DataBaseUpdateSugestion();

    QHostAddress udp_for_decoded_messages_address;
    quint16 udp_for_decoded_messages_port;
    bool udp_for_decoded_messages_enabled;

private:
    Ui::SettingsDialog *ui;    
    void poulatepublicvars();
    QDate lastdbupdate;

protected:
    void accept();

private slots:

    void on_lineEditlogdir_editingFinished();
    void on_lineEditplanesfolder_editingFinished();
    void on_pushButtonDownloadDB_clicked();
    void DownloadDBResult(const QUrl &url, bool result);
};

#endif // SETTINGSDIALOG_H
