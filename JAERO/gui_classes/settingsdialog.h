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
#include <QJsonArray>
#include "mqttsubscriber.h"

extern QString settings_name;

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
    bool onlyuselibacars;

    bool set_station_id_enabled;
    QString station_id;

    QJsonArray udp_feeders;
    bool udp_for_decoded_messages_enabled;


    QHostAddress tcp_for_ads_messages_address;
    quint16 tcp_for_ads_messages_port;
    bool tcp_for_ads_messages_enabled;
    bool tcp_as_client_enabled;

    bool cpuSaveMode;
    bool disableAcarsConsole;

    bool localAudioOutEnabled;
    bool zmqAudioOutEnabled;
    QString zmqAudioOutBind;
    QString zmqAudioOutTopic;

    bool zmqAudioInputEnabled;
    QString zmqAudioInputAddress;
    QString zmqAudioInputTopic;

    bool disablePlaneLogWindow;

    MqttSubscriber_Settings_Object mqtt_settings_object;
    bool mqtt_enable;

private:
    Ui::SettingsDialog *ui;
    QStringList outputformats = {"1", "2", "3", "JSON", "JSONdump"};
    void populatepublicvars();
    bool isUniqueFeederHostPort(const QString &host, const QString &port);
    int validateOutputFormatIdx(int idx);

protected:
    void accept();

private slots:

    void on_lineEditlogdir_editingFinished();
    void on_lineEditplanesfolder_editingFinished();
    void on_checkOutputADSMessageToTCP_stateChanged(int arg1);
    void on_newEntryButton_clicked();
    void on_editEntryButton_clicked();
    void on_removeEntryButton_clicked();
    void on_outputListTable_itemSelectionChanged();
};

#endif // SETTINGSDIALOG_H
