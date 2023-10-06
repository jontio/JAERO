#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QStandardPaths>
#include <QLibrary>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHostInfo>

//create a publisher based of the GUI MQTT settings
//used for testing
//#define MQTT_PUBLISH_TEST

#ifdef __linux__
#include <unistd.h>
#define Sleep(x) usleep(x * 1000);
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include "databasetext.h"


template <typename T>
QString upperHex(T a, int fieldWidth, int base, QChar fillChar)
{
    return QString("%1").arg(a, fieldWidth, base, fillChar).toUpper();
}

//helper function for connecting UDP sockets, abstracted for potential usefulness in the future
void udpSocketConnect(QUdpSocket &sock, const QString &host, quint16 port)
{
    sock.connectToHost(host, port);
}

void MainWindow::setLedState(QLed *led, LedState state)
{
    QLabel *label=nullptr;
    if(led==ui->ledmqtt)label=ui->labelmqtt;
    else if(led==ui->leddata)label=ui->labeldata;
    else if(led==ui->ledsignal)label=ui->labelsignal;
    else if(led==ui->ledvolume)label=ui->labelvolume;
    switch(state)
    {
    case LedState::Disable:
        led->setVisible(false);
        if(label)label->setVisible(false);
        break;
    default:
        led->setVisible(true);
        if(label)label->setVisible(true);
        break;
    }
    switch(state)
    {
    case LedState::Off:
        led->setLED(QIcon::Off);
        break;
    case LedState::On:
        led->setLED(QIcon::On);
        break;
    case LedState::Overload:
        led->setLED(QIcon::On,QIcon::Active);
        break;
    default:
        break;
    }
}

void MainWindow::onMqttConnectionStateChange(MqttSubscriber::ConnectionState state)
{
    qDebug()<<"MainWindow::onMqttConnectionStateChange"<<state;
    if(!settingsdialog->mqtt_enable)
    {
        setLedState(ui->ledmqtt,LedState::Disable);
        return;
    }
    if(state==MqttSubscriber::STATE_CONNECTED)
    {
        setLedState(ui->ledmqtt,LedState::Off);
        return;
    }
    if(state==MqttSubscriber::STATE_CONNECTED_SUBSCRIBED)
    {
        setLedState(ui->ledmqtt,LedState::On);
        return;
    }
    setLedState(ui->ledmqtt,LedState::Overload);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    last_dcd=false;
    last_frequency=0;
    last_EbNo=0;

    beep=nullptr;

    //plane logging window
    planelog = new PlaneLog;

    //mqtt pub & sub
    mqttsubscriber=new MqttSubscriber(this);

    //create zmq audio receiver
    zmq_audio_receiver = new ZMQAudioReceiver(this);
    //create zmq audio sender
    zmq_audio_sender = new ZMQAudioSender(this);

    //create areo decoder
    aerol = new AeroL(this); //Create Aero sink
    aerol2 = new AeroL(this); //Create Aero sink

    //ambe decompressor
    QString aeroambe_object_error_str;
    QLibrary library("aeroambe.dll");
    if(!library.load())library.setFileName("aeroambe.so");
    if(!library.load())library.setFileName(QDir::currentPath()+"/aeroambe.dll");
    if(!library.load())library.setFileName(QDir::currentPath()+"/aeroambe.so");
    if(!library.load())library.setFileName(QApplication::applicationDirPath()+"/aeroambe.dll");
    if(!library.load())library.setFileName(QApplication::applicationDirPath()+"/aeroambe.so");
    if(!library.load())
    {
        aeroambe_object_error_str="Can't find or load all the libraries necessary for aeroambe. You will not get audio.";//library.errorString() is a usless description and can be missleading, not using
        ambe=new QObject;
        ambe->setObjectName("NULL");
    }
    if(library.load())
    {
        qDebug() << "aeroambe library loaded";
        typedef QObject *(*CreateQObjectFunction)(QObject*);
        CreateQObjectFunction cof = (CreateQObjectFunction)library.resolve("createAeroAMBE");
        if (cof)
        {
            ambe = cof(0);//Cannot create children for a parent that is in a different thread. so have to use 0 and manually delete or use an autoptr
            ambe->setObjectName("ambe");
        }
        else
        {
            ambe=new QObject;
            ambe->setObjectName("NULL");
            aeroambe_object_error_str="Could not resolve createAeroAMBE in aeroambe. You will not get audio.";
        }
    }

    //create settings dialog.
    settingsdialog = new SettingsDialog(this);

    //create the demodulator
    audiomskdemodulator = new AudioMskDemodulator(this);

    //create the oqpsk demodulator
    audiooqpskdemodulator = new AudioOqpskDemodulator(this);

    //create the burst oqpsk demodulator
    audioburstoqpskdemodulator = new AudioBurstOqpskDemodulator(this);

    //create the burst msk demodulator
    audioburstmskdemodulator = new AudioBurstMskDemodulator(this);

    //add a audio output device
    audioout = new AudioOutDevice(this);
    if(ambe->objectName()!="NULL")audioout->start();

    //add a thing for saving audio to disk with
    compresseddiskwriter = new CompressedAudioDiskWriter(this);

    //create udp socket for raw data and hex (not bottom textbox)
    udpsocket = new QUdpSocket(this);

    //create sbs server and connect the ADS parser to it
    sbs1 = new SBS1(this);
    connect(&arincparser,SIGNAL(DownlinkGroupsSignal(DownlinkGroups&)),sbs1,SLOT(DownlinkGroupsSlot(DownlinkGroups&)));
    //connect(&arincparser,SIGNAL(DownlinkBasicReportGroupSignal(DownlinkBasicReportGroup&)),sbs1,SLOT(DownlinkBasicReportGroupSlot(DownlinkBasicReportGroup&)));
    //connect(&arincparser,SIGNAL(DownlinkEarthReferenceGroupSignal(DownlinkEarthReferenceGroup&)),sbs1,SLOT(DownlinkEarthReferenceGroupSlot(DownlinkEarthReferenceGroup&)));

    //default sink is the aerol device
    audiomskdemodulator->ConnectSinkDevice(aerol);
    audiooqpskdemodulator->ConnectSinkDevice(aerol);
    audioburstoqpskdemodulator->ConnectSinkDevice(aerol);
    audioburstoqpskdemodulator->demod2->ConnectSinkDevice(aerol2);
    audioburstmskdemodulator->ConnectSinkDevice(aerol);

    //console setup
    ui->console->setEnabled(true);
    ui->console->setLocalEchoEnabled(true);

    //aerol setup
    aerol->ConnectSinkDevice(ui->console->consoledevice);
    aerol2->ConnectSinkDevice(ui->console->consoledevice);

    connect(audiomskdemodulator,SIGNAL(processDemodulatedSoftBits(QVector<short>)),aerol,SLOT(processDemodulatedSoftBits(QVector<short>)));
    connect(audiooqpskdemodulator,SIGNAL(processDemodulatedSoftBits(QVector<short>)),aerol,SLOT(processDemodulatedSoftBits(QVector<short>)));
    connect(audioburstmskdemodulator,SIGNAL(processDemodulatedSoftBits(QVector<short>)),aerol,SLOT(processDemodulatedSoftBits(QVector<short>)));
    connect(audioburstoqpskdemodulator,SIGNAL(processDemodulatedSoftBits(QVector<short>)),aerol,SLOT(processDemodulatedSoftBits(QVector<short>)));
    connect(audioburstoqpskdemodulator->demod2,SIGNAL(processDemodulatedSoftBits(QVector<short>)),aerol2,SLOT(processDemodulatedSoftBits(QVector<short>)));

    //send compressed audio through decompressor
    //connect(ambe,SIGNAL(decoded_signal(QByteArray)),this,SLOT(Voiceslot(QByteArray))); // an example
    connect(ambe,SIGNAL(decoded_signal(QByteArray)),compresseddiskwriter,SLOT(audioin(QByteArray)));
    connect(ambe,SIGNAL(decoded_signal(QByteArray)),audioout,SLOT(audioin(QByteArray)));

    //statusbar setup
    sourcelabel = new QLabel();
    freqlabel = new QLabel();
    ebnolabel = new QLabel();
    ui->statusBar->addPermanentWidget(new QLabel());
    ui->statusBar->addPermanentWidget(sourcelabel);
    ui->statusBar->addPermanentWidget(freqlabel);
    ui->statusBar->addPermanentWidget(ebnolabel);

    //led setup
    setLedState(ui->ledvolume,LedState::Off);
    setLedState(ui->ledsignal,LedState::Off);
    setLedState(ui->leddata,LedState::Off);
    setLedState(ui->ledmqtt,LedState::Disable);

    //misc connections
    connect(ui->action_About,    SIGNAL(triggered()),                                   this, SLOT(AboutSlot()));

    //select msk or oqpsk
    selectdemodulatorconnections(MSK);
    typeofdemodtouse=NoDemodType;

    //aeroL connections
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),this,SLOT(DataCarrierDetectStatusSlot(bool)));
    connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),this,SLOT(ACARSslot(ACARSItem&)));
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),audiomskdemodulator,SLOT(DCDstatSlot(bool)));
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),audioburstmskdemodulator,SLOT(DCDstatSlot(bool)));
    connect(aerol,SIGNAL(CChannelAssignmentSignal(CChannelAssignmentItem&)),this,SLOT(CChannelAssignmentSlot(CChannelAssignmentItem&)));
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),audiooqpskdemodulator,SLOT(DCDstatSlot(bool)));
    connect(aerol,SIGNAL(Call_progress_Signal(QByteArray)),compresseddiskwriter,SLOT(Call_progress_Slot(QByteArray)));

    //aeroL2 connections
    connect(aerol2,SIGNAL(DataCarrierDetect(bool)),this,SLOT(DataCarrierDetectStatusSlot(bool)));
    connect(aerol2,SIGNAL(ACARSsignal(ACARSItem&)),this,SLOT(ACARSslot(ACARSItem&)));

    //load settings
    QSettings settings("Jontisoft", settings_name);
    ui->comboBoxafc->setCurrentIndex(settings.value("comboBoxafc",1).toInt());
    ui->comboBoxbps->setCurrentIndex(settings.value("comboBoxbps",0).toInt());
    ui->comboBoxlbw->setCurrentIndex(settings.value("comboBoxlbw",0).toInt());
    ui->comboBoxdisplay->setCurrentIndex(settings.value("comboBoxdisplay",0).toInt());
    ui->actionConnectToUDPPort->setChecked(settings.value("actionConnectToUDPPort",false).toBool());
    ui->actionRawOutput->setChecked(settings.value("actionRawOutput",false).toBool());
    ui->actionSound_Out->setChecked(settings.value("actionSound_Out",false).toBool());
    ui->actionReduce_CPU->setChecked(settings.value("actionCpuReduce",false).toBool());

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    double tmpfreq=settings.value("freq_center",1000).toDouble();
    ui->inputwidget->setPlainText(settings.value("inputwidget","").toString());
    ui->tabWidget->setCurrentIndex(settings.value("tabindex",0).toInt());

    //set audio msk demodulator settings and start
    on_comboBoxafc_currentIndexChanged(ui->comboBoxafc->currentText());
    on_comboBoxbps_currentIndexChanged(ui->comboBoxbps->currentText());
    on_comboBoxlbw_currentIndexChanged(ui->comboBoxlbw->currentText());
    on_comboBoxdisplay_currentIndexChanged(ui->comboBoxdisplay->currentText());
    on_actionConnectToUDPPort_toggled(ui->actionConnectToUDPPort->isChecked());

    //msk
    audiomskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audiomskdemodulatorsettings.freq_center=tmpfreq;
    audiomskdemodulator->setSQL(false);
    audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
    audiomskdemodulator->setCPUReduce(ui->actionReduce_CPU->isChecked());
    audiomskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
    if(typeofdemodtouse==MSK)audiomskdemodulator->start();

    //oqpsk
    audiooqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audiooqpskdemodulatorsettings.freq_center=tmpfreq;
    audiooqpskdemodulator->setSQL(false);
    audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
    audiooqpskdemodulator->setCPUReduce(ui->actionReduce_CPU->isChecked());
    audiooqpskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
    if(typeofdemodtouse==OQPSK)audiooqpskdemodulator->start();

    //burstoqpsk
    audioburstoqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audioburstoqpskdemodulatorsettings.freq_center=tmpfreq;
    audioburstoqpskdemodulator->setSQL(false);
    audioburstoqpskdemodulator->setSettings(audioburstoqpskdemodulatorsettings);
    audioburstoqpskdemodulator->setCPUReduce(ui->actionReduce_CPU->isChecked());
    audioburstoqpskdemodulatorsettings.zmqAudio =settingsdialog->zmqAudioInputEnabled;
    if(typeofdemodtouse==BURSTOQPSK)audioburstoqpskdemodulator->start();

    //burst msk
    audioburstmskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audioburstmskdemodulatorsettings.freq_center=tmpfreq;
    audioburstmskdemodulator->setSQL(false);
    audioburstmskdemodulator->setSettings(audioburstmskdemodulatorsettings);
    audioburstmskdemodulator->setCPUReduce(ui->actionReduce_CPU->isChecked());
    audioburstmskdemodulatorsettings.zmqAudio =settingsdialog->zmqAudioInputEnabled;
    if(typeofdemodtouse==BURSTMSK)audioburstmskdemodulator->start();

    //add todays date
    ui->inputwidget->appendPlainText(QDateTime::currentDateTime().toUTC().toString("h:mmap ddd d-MMM-yyyy")+" UTC JAERO started\n");
    QTimer::singleShot(100,ui->inputwidget,SLOT(scrolltoend()));

    //say if aeroabme was not found or loaded correctly
    if(!aeroambe_object_error_str.isEmpty())ui->inputwidget->appendPlainText(aeroambe_object_error_str+"\n");

    ui->actionTXRX->setVisible(false);//there is a hidden audio modulator icon.

    //publish test
#ifdef MQTT_PUBLISH_TEST
    MqttSubscriber *mqttpublisher=new MqttSubscriber(this);
    MqttSubscriber_Settings_Object mqtt_settings=settingsdialog->mqtt_settings_object;
    mqtt_settings.clientId="pub_41349f7ug134bhof";
    mqtt_settings.publish=true;
    mqtt_settings.subscribe=false;
    connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),mqttpublisher,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
    connect(aerol2,SIGNAL(ACARSsignal(ACARSItem&)),mqttpublisher,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
    mqttpublisher->connectToHost(mqtt_settings);
#endif

    //MQTT connections
    connect(mqttsubscriber,SIGNAL(connectionStateChange(MqttSubscriber::ConnectionState)),this,SLOT(onMqttConnectionStateChange(MqttSubscriber::ConnectionState)));
    connect(mqttsubscriber,SIGNAL(ACARSsignal(ACARSItem&)),this,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
    connect(mqttsubscriber,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
    connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),mqttsubscriber,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
    connect(aerol2,SIGNAL(ACARSsignal(ACARSItem&)),mqttsubscriber,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);

    //set pop and accept settings
    settingsdialog->populatesettings();
    acceptsettings();

    //periodic timer for sending status info if using UDP and JSON
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(statusToUDPifJSONset()));
    timer->start(30000);

}

void MainWindow::selectdemodulatorconnections(DemodType demodtype)
{
    static DemodType lastdemodtype=NoDemodType;
    if(demodtype!=lastdemodtype)
    {
        //invalidate demods
        audiomskdemodulator->invalidatesettings();
        audiooqpskdemodulator->invalidatesettings();
        audioburstoqpskdemodulator->invalidatesettings();
        audioburstmskdemodulator->invalidatesettings();
    }
    lastdemodtype=demodtype;
    switch(demodtype)
    {
    case MSK: //msk
    default:

        //some oqpsk connections
        disconnect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audiooqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audiooqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audiooqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                     audiooqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audiooqpskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some burstoqpsk connections
        disconnect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,        SIGNAL(CenterFreqChanged(double)),                     audioburstoqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audioburstoqpskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //burstdemod demod2
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                          audioburstoqpskdemodulator->demod2,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(BitRateChanged(double,bool)),                   aerol2,SLOT(setSettings(double,bool)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),aerol2,SLOT(SignalStatusSlot(bool)));

        //some burst msk connections
        disconnect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstmskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstmskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstmskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audioburstmskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audioburstmskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some msk connections
        connect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        connect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        connect(audiomskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        connect(audiomskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        connect(audiomskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        connect(audiomskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        connect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        connect(audiomskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        connect(audiomskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        connect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audiomskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        connect(audiomskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        connect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        connect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)), audiomskdemodulator, SLOT(dataReceived(QByteArray,quint32)), Qt::UniqueConnection);

        break;
    case OQPSK: //opqsk
        //some msk connections
        disconnect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audiomskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audiomskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audiomskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audiomskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audiomskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audiomskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some burstoqpsk connections
        disconnect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                          audioburstoqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audioburstoqpskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //burstdemod demod2
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                          audioburstoqpskdemodulator->demod2,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(BitRateChanged(double,bool)),                   aerol2,SLOT(setSettings(double,bool)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),aerol2,SLOT(SignalStatusSlot(bool)));

        //some burst msk connections
        disconnect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstmskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstmskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstmskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audioburstmskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audioburstmskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some oqpsk connections
        connect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        connect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        connect(audiooqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        connect(audiooqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        connect(audiooqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        connect(audiooqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        connect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        connect(audiooqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        connect(audiooqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        connect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                     audiooqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        connect(audiooqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        connect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        connect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)), audiooqpskdemodulator, SLOT(dataReceived(QByteArray,quint32)), Qt::UniqueConnection);

        break;
    case BURSTOQPSK: //burstopqsk

        //some msk connections
        disconnect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audiomskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audiomskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audiomskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audiomskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audiomskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audiomskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some oqpsk connections
        disconnect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audiooqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audiooqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audiooqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                     audiooqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audiooqpskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some burst msk connections
        disconnect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstmskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstmskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstmskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audioburstmskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstmskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audioburstmskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some burstoqpsk connections
        connect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        connect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        connect(audioburstoqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        connect(audioburstoqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        connect(audioburstoqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        connect(audioburstoqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        connect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        connect(audioburstoqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        connect(audioburstoqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        connect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                          audioburstoqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        connect(audioburstoqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        connect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        connect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)), audioburstoqpskdemodulator, SLOT(dataReceived(QByteArray,quint32)), Qt::UniqueConnection);

        //burstdemod demod2
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        connect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                          audioburstoqpskdemodulator->demod2,SLOT(CenterFreqChangedSlot(double)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(BitRateChanged(double,bool)),                   aerol2,SLOT(setSettings(double,bool)));
        connect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),aerol2,SLOT(SignalStatusSlot(bool)));

        break;

    case BURSTMSK: //bursmsk

        //some msk connections
        disconnect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audiomskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audiomskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audiomskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audiomskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audiomskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audiomskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audiomskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audiomskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some oqpsk connections
        disconnect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audiooqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audiooqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audiooqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audiooqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                     audiooqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audiooqpskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //some burstoqpsk connections
        disconnect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                          audioburstoqpskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        disconnect(audioburstoqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        disconnect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)),audioburstoqpskdemodulator,SLOT(dataReceived(QByteArray,quint32)));

        //burstdemod demod2
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        disconnect(ui->spectrumdisplay,   SIGNAL(CenterFreqChanged(double)),                          audioburstoqpskdemodulator->demod2,SLOT(CenterFreqChangedSlot(double)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(BitRateChanged(double,bool)),                   aerol2,SLOT(setSettings(double,bool)));
        disconnect(audioburstoqpskdemodulator->demod2, SIGNAL(SignalStatus(bool)),aerol2,SLOT(SignalStatusSlot(bool)));

        //some burst msk connections
        connect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              this,SLOT(PlottablesSlot(double,double,double)));
        connect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),                            this,SLOT(SignalStatusSlot(bool)));
        connect(audioburstmskdemodulator, SIGNAL(WarningTextSignal(QString)),                    this,SLOT(WarningTextSlot(QString)));
        connect(audioburstmskdemodulator, SIGNAL(EbNoMeasurmentSignal(double)),                  this,SLOT(EbNoSlot(double)));
        connect(audioburstmskdemodulator, SIGNAL(PeakVolume(double)),                            this, SLOT(PeakVolumeSlot(double)));
        connect(audioburstmskdemodulator, SIGNAL(OrgOverlapedBuffer(QVector<double>)),           ui->spectrumdisplay,SLOT(setFFTData(QVector<double>)));
        connect(audioburstmskdemodulator, SIGNAL(Plottables(double,double,double)),              ui->spectrumdisplay,SLOT(setPlottables(double,double,double)));
        connect(audioburstmskdemodulator, SIGNAL(SampleRateChanged(double)),                     ui->spectrumdisplay,SLOT(setSampleRate(double)));
        connect(audioburstmskdemodulator, SIGNAL(ScatterPoints(QVector<cpx_type>)),              ui->scatterplot,SLOT(setData(QVector<cpx_type>)));
        connect(ui->spectrumdisplay, SIGNAL(CenterFreqChanged(double)),                     audioburstmskdemodulator,SLOT(CenterFreqChangedSlot(double)));
        connect(audioburstmskdemodulator, SIGNAL(BitRateChanged(double,bool)),                   aerol,SLOT(setSettings(double,bool)));
        connect(audioburstmskdemodulator, SIGNAL(SignalStatus(bool)),                           aerol,SLOT(SignalStatusSlot(bool)));
        connect(zmq_audio_receiver, SIGNAL(recAudio(const QByteArray&,quint32)), audioburstmskdemodulator, SLOT(dataReceived(QByteArray,quint32)), Qt::UniqueConnection);

        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    //save settings
    QSettings settings("Jontisoft", settings_name);
    settings.setValue("comboBoxafc", ui->comboBoxafc->currentIndex());
    settings.setValue("comboBoxbps", ui->comboBoxbps->currentIndex());
    settings.setValue("comboBoxlbw", ui->comboBoxlbw->currentIndex());
    settings.setValue("comboBoxdisplay", ui->comboBoxdisplay->currentIndex());
    settings.setValue("actionConnectToUDPPort", ui->actionConnectToUDPPort->isChecked());
    settings.setValue("actionRawOutput", ui->actionRawOutput->isChecked());
    settings.setValue("actionSound_Out", ui->actionSound_Out->isChecked());
    settings.setValue("actionCpuReduce", ui->actionReduce_CPU->isChecked());
    settings.setValue("tabindex", ui->tabWidget->currentIndex());
    if(typeofdemodtouse==MSK)settings.setValue("freq_center", audiomskdemodulator->getCurrentFreq());
    if(typeofdemodtouse==OQPSK)settings.setValue("freq_center", audiooqpskdemodulator->getCurrentFreq());
    if(typeofdemodtouse==BURSTOQPSK)settings.setValue("freq_center", audioburstoqpskdemodulator->getCurrentFreq());
    if(typeofdemodtouse==BURSTMSK)settings.setValue("freq_center", audioburstmskdemodulator->getCurrentFreq());

    settings.setValue("inputwidget", ui->inputwidget->toPlainText());

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    planelog->close();
    event->accept();
}

MainWindow::~MainWindow()
{
    delete ambe;
    delete planelog;
    delete ui;
}

void MainWindow::SignalStatusSlot(bool signal)
{
    if(signal)ui->ledsignal->setLED(QIcon::On);
    else ui->ledsignal->setLED(QIcon::Off);
}

void MainWindow::DataCarrierDetectStatusSlot(bool dcd)
{
    last_dcd=dcd;
    if(dcd)ui->leddata->setLED(QIcon::On);
    else ui->leddata->setLED(QIcon::Off);
}

void MainWindow::EbNoSlot(double EbNo)
{
    last_EbNo=EbNo;
    ebnolabel->setText(((QString)" EbNo: %1dB ").arg((int)round(EbNo),2, 10, QChar('0')));
}

void MainWindow::WarningTextSlot(QString warning)
{
    ui->statusBar->showMessage("Warning: "+warning,5000);
}

void MainWindow::PeakVolumeSlot(double Volume)
{
    if(Volume>0.9)ui->ledvolume->setLED(QIcon::On,QIcon::Active);
    else if(Volume<0.1)ui->ledvolume->setLED(QIcon::Off);
    else ui->ledvolume->setLED(QIcon::On);
}

void MainWindow::PlottablesSlot(double freq_est,double freq_center,double bandwidth)
{
    Q_UNUSED(freq_center);
    Q_UNUSED(bandwidth);
    last_frequency=freq_est;
    QString str=(((QString)"%1Hz  ").arg(freq_est,0, 'f', 2)).rightJustified(11,' ');
    freqlabel->setText("  Freq: "+str);
}

void MainWindow::AboutSlot()
{
    QStringList date=QString(__DATE__).split(" ");
    QMessageBox::about(this,"JAERO",""
                                    "<H1>An Aero demodulator and decoder</H1>"
                                    "<H3>"+QString(JAERO_VERSION)+"</H3>"
                                                                  "<p>This is a program to demodulate and decode Aero signals. These signals contain SatCom ACARS (<em>Satellite Communication Aircraft Communications Addressing and Reporting System</em>) messages as used by planes beyond VHF ACARS range. This protocol is used by Inmarsat's \"Classic Aero\" system and can be received using low or medium gain L band or high gain C band antennas.</p>"
                                                                  "<p>For more information about this application see <a href=\"http://jontio.zapto.org/hda1/jaero.html\">http://jontio.zapto.org/hda1/jaero.html</a>.</p>"
                                                                  "<p>Jonti "+date[1]+" "+date[0]+" "+date[2]+"</p>" );
}

void MainWindow::on_comboBoxbps_currentIndexChanged(const QString &arg)
{
    QString arg1=arg;
    if(arg1.isEmpty())
    {
        ui->comboBoxbps->setCurrentIndex(0);
        arg1=ui->comboBoxbps->currentText();
    }

    //demodulator settings
    double bitrate_tmp=arg1.split(" ")[0].toDouble();

    DemodType lasttypeofdemodtouse=typeofdemodtouse;
    if(bitrate_tmp>1200)typeofdemodtouse=OQPSK;
    else typeofdemodtouse=MSK;
    if(arg.contains("burst")&&typeofdemodtouse==OQPSK)typeofdemodtouse=BURSTOQPSK;
    if(arg.contains("burst")&&typeofdemodtouse==MSK)typeofdemodtouse=BURSTMSK;

    if(lasttypeofdemodtouse==BURSTOQPSK)
    {
        audioburstoqpskdemodulatorsettings.freq_center=audioburstoqpskdemodulator->getCurrentFreq();
        audiooqpskdemodulatorsettings.freq_center=audioburstoqpskdemodulator->getCurrentFreq();
        audiomskdemodulatorsettings.freq_center=audioburstoqpskdemodulator->getCurrentFreq();
        audioburstmskdemodulatorsettings.freq_center=audioburstoqpskdemodulator->getCurrentFreq();
    }

    if(lasttypeofdemodtouse==OQPSK)
    {
        audioburstoqpskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
        audiooqpskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
        audiomskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
        audioburstmskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
    }

    if(lasttypeofdemodtouse==MSK)
    {
        audioburstoqpskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
        audiooqpskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
        audiomskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
        audioburstmskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
    }

    if(lasttypeofdemodtouse==BURSTMSK)
    {
        audioburstoqpskdemodulatorsettings.freq_center=audioburstmskdemodulator->getCurrentFreq();
        audiooqpskdemodulatorsettings.freq_center=audioburstmskdemodulator->getCurrentFreq();
        audiomskdemodulatorsettings.freq_center=audioburstmskdemodulator->getCurrentFreq();
        audioburstmskdemodulatorsettings.freq_center=audioburstmskdemodulator->getCurrentFreq();
    }

    audiomskdemodulatorsettings.fb=bitrate_tmp;
    audiooqpskdemodulatorsettings.fb=bitrate_tmp;
    audioburstoqpskdemodulatorsettings.fb=bitrate_tmp;
    audioburstmskdemodulatorsettings.fb=bitrate_tmp;

    audiooqpskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
    audiomskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
    audioburstoqpskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
    audioburstmskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;

    if(typeofdemodtouse==BURSTOQPSK)
    {
        audiomskdemodulator->stop();
        audiooqpskdemodulator->stop();
        audioburstmskdemodulator->stop();
        selectdemodulatorconnections(BURSTOQPSK);

        int idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(audioburstoqpskdemodulatorsettings.fb*1.0));
        if(idx>=0)audioburstoqpskdemodulatorsettings.lockingbw=ui->comboBoxlbw->itemText(idx).split(" ")[0].toDouble();
        audioburstoqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
        if(arg.contains("x2"))audioburstoqpskdemodulatorsettings.channel_stereo=true;
        else audioburstoqpskdemodulatorsettings.channel_stereo=false;
        audioburstoqpskdemodulator->setSettings(audioburstoqpskdemodulatorsettings);
        if(idx>=0)ui->comboBoxlbw->setCurrentIndex(idx);

        audioburstoqpskdemodulator->start();
    }

    if(typeofdemodtouse==OQPSK)
    {
        audiomskdemodulator->stop();
        audioburstoqpskdemodulator->stop();
        audioburstmskdemodulator->stop();
        selectdemodulatorconnections(OQPSK);
        audiooqpskdemodulatorsettings.Fs=48000;
        int idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(audiooqpskdemodulatorsettings.fb*1.0));
        if(idx>=0)audiooqpskdemodulatorsettings.lockingbw=ui->comboBoxlbw->itemText(idx).split(" ")[0].toDouble();

        //if ambe then use a smaller locking bw by default
        if(audiooqpskdemodulatorsettings.fb==8400)
        {
            idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(5000));
            if(idx>=0)audiooqpskdemodulatorsettings.lockingbw=ui->comboBoxlbw->itemText(idx).split(" ")[0].toDouble();
        }

        audiooqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
        audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
        if(idx>=0)ui->comboBoxlbw->setCurrentIndex(idx);

        audiooqpskdemodulator->start();
    }

    if(typeofdemodtouse==MSK)
    {
        audiooqpskdemodulator->stop();
        audioburstoqpskdemodulator->stop();
        audioburstmskdemodulator->stop();
        selectdemodulatorconnections(MSK);
        if(!settingsdialog->widebandwidthenable)
        {
            if(audiomskdemodulatorsettings.fb==600){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=12000;}
            if(audiomskdemodulatorsettings.fb==1200){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=24000;}
        }
        else
        {
            if(audiomskdemodulatorsettings.fb==600){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=48000;}
            if(audiomskdemodulatorsettings.fb==1200){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=48000;}
        }

        int idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(audiomskdemodulatorsettings.fb*1.5));
        if(idx>=0)audiomskdemodulatorsettings.lockingbw=ui->comboBoxlbw->itemText(idx).split(" ")[0].toDouble();
        audiomskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
        audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
        if(idx>=0)ui->comboBoxlbw->setCurrentIndex(idx);

        audiomskdemodulator->start();
    }

    if(typeofdemodtouse==BURSTMSK)
    {
        audiooqpskdemodulator->stop();
        audioburstoqpskdemodulator->stop();
        audiomskdemodulator->stop();

        selectdemodulatorconnections(BURSTMSK);
        audioburstmskdemodulatorsettings.Fs=48000;
        int idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(audioburstmskdemodulatorsettings.fb*1.5));
        if(idx>=0)audioburstmskdemodulatorsettings.lockingbw=ui->comboBoxlbw->itemText(idx).split(" ")[0].toDouble();
        audioburstmskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
        audioburstmskdemodulator->setSettings(audioburstmskdemodulatorsettings);
        if(idx>=0)ui->comboBoxlbw->setCurrentIndex(idx);

        audioburstmskdemodulator->start();
    }

}

void MainWindow::on_comboBoxlbw_currentIndexChanged(const QString &arg1)
{
    audiomskdemodulatorsettings.lockingbw=arg1.split(" ")[0].toDouble();
    audiomskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
    if(audiomskdemodulatorsettings.lockingbw==24000)audiomskdemodulatorsettings.freq_center=12000;
    audiomskdemodulator->setSettings(audiomskdemodulatorsettings);

    audiooqpskdemodulatorsettings.lockingbw=arg1.split(" ")[0].toDouble();
    audiooqpskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
    if(audiooqpskdemodulatorsettings.lockingbw==24000)audiooqpskdemodulatorsettings.freq_center=12000;
    audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);

    audioburstoqpskdemodulatorsettings.lockingbw=arg1.split(" ")[0].toDouble();
    audioburstoqpskdemodulatorsettings.freq_center=audioburstoqpskdemodulator->getCurrentFreq();
    if(audioburstoqpskdemodulatorsettings.lockingbw==24000)audioburstoqpskdemodulatorsettings.freq_center=12000;
    audioburstoqpskdemodulator->setSettings(audioburstoqpskdemodulatorsettings);

    audioburstmskdemodulatorsettings.lockingbw=arg1.split(" ")[0].toDouble();
    audioburstmskdemodulatorsettings.freq_center=audioburstmskdemodulator->getCurrentFreq();
    if(audioburstmskdemodulatorsettings.lockingbw==24000)audioburstmskdemodulatorsettings.freq_center=12000;
    audioburstmskdemodulator->setSettings(audioburstmskdemodulatorsettings);
}

void MainWindow::on_comboBoxafc_currentIndexChanged(const QString &arg1)
{
    if(arg1=="AFC on")
    {
        audiomskdemodulator->setAFC(true);
        audiooqpskdemodulator->setAFC(true);
        audioburstoqpskdemodulator->setAFC(true);
        audioburstmskdemodulator->setAFC(true);
    }
    else
    {
        audiomskdemodulator->setAFC(false);
        audiooqpskdemodulator->setAFC(false);
        audioburstoqpskdemodulator->setAFC(false);
        audioburstmskdemodulator->setAFC(false);
    }
}

void MainWindow::on_actionCleanConsole_triggered()
{

    switch(ui->tabWidget->currentIndex())
    {
    case 0:
        ui->console->clear();
        break;
    case 1:
        ui->inputwidget->clear();
        break;
    case 2:
        ui->plainTextEdit_cchan_assignment->clear();
        break;
    default:
        break;
    };
}

void MainWindow::on_comboBoxdisplay_currentIndexChanged(const QString &arg1)
{
#if QCUSTOMPLOT_VERSION >= 0x020000
    ui->scatterplot->graph(0)->data().clear();
#else
    ui->scatterplot->graph(0)->clearData();
#endif

    ui->scatterplot->replot();
    ui->scatterplot->setDisksize(3);
    if(arg1=="Constellation")
    {
        audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_constellation);
        audiooqpskdemodulator->setScatterPointType(OqpskDemodulator::SPT_constellation);
        audioburstoqpskdemodulator->setScatterPointType(BurstOqpskDemodulator::SPT_constellation);
        audioburstmskdemodulator->setScatterPointType(BurstMskDemodulator::SPT_constellation);
    }
    if(arg1=="Symbol phase")
    {
        audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_phaseoffsetest);
        audiooqpskdemodulator->setScatterPointType(OqpskDemodulator::SPT_phaseoffsetest);
        audioburstoqpskdemodulator->setScatterPointType(BurstOqpskDemodulator::SPT_phaseoffsetest);
        audioburstmskdemodulator->setScatterPointType(BurstMskDemodulator::SPT_phaseoffsetest);
        ui->scatterplot->setDisksize(6);
    }
    if(arg1=="None")
    {
        audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_None);
        audiooqpskdemodulator->setScatterPointType(OqpskDemodulator::SPT_None);
        audioburstoqpskdemodulator->setScatterPointType(BurstOqpskDemodulator::SPT_None);
        audioburstmskdemodulator->setScatterPointType(BurstMskDemodulator::SPT_None);
    }
}

void MainWindow::on_actionConnectToUDPPort_toggled(bool arg1)
{
    audiomskdemodulator->DisconnectSinkDevice();
    audiooqpskdemodulator->DisconnectSinkDevice();
    audioburstoqpskdemodulator->DisconnectSinkDevice();
    audioburstoqpskdemodulator->demod2->DisconnectSinkDevice();
    audioburstmskdemodulator->DisconnectSinkDevice();

    aerol->DisconnectSinkDevice();
    aerol2->DisconnectSinkDevice();
    udpsocket->close();
    if(arg1)
    {
        ui->actionRawOutput->setEnabled(true);
        ui->console->setEnableUpdates(false,"Console disabled while raw demodulated data is routed to UDP port 8765 at LocalHost.");
        udpsocket->connectToHost(QHostAddress::LocalHost, 8765);

        if(ui->actionRawOutput->isChecked())
        {
            audiomskdemodulator->ConnectSinkDevice(udpsocket);
            audiooqpskdemodulator->ConnectSinkDevice(udpsocket);
            audioburstoqpskdemodulator->ConnectSinkDevice(udpsocket);
            audioburstmskdemodulator->ConnectSinkDevice(udpsocket);

            if(audioburstoqpskdemodulatorsettings.channel_stereo)ui->console->setEnableUpdates(false,"Console disabled while raw demodulated data from the left channel is routed to UDP port 8765 at LocalHost.");
            else ui->console->setEnableUpdates(false,"Console disabled while raw demodulated data is routed to UDP port 8765 at LocalHost.");
        }
        else
        {
            audiomskdemodulator->ConnectSinkDevice(aerol);
            audiooqpskdemodulator->ConnectSinkDevice(aerol);
            audioburstoqpskdemodulator->ConnectSinkDevice(aerol);
            audioburstmskdemodulator->ConnectSinkDevice(aerol);

            aerol->ConnectSinkDevice(udpsocket);
            if(audioburstoqpskdemodulatorsettings.channel_stereo) ui->console->setEnableUpdates(false,"Console disabled while decoded and demodulated data from the left channel is routed to UDP port 8765 at LocalHost.");
            else ui->console->setEnableUpdates(false,"Console disabled while decoded and demodulated data is routed to UDP port 8765 at LocalHost.");
        }

    }
    else
    {
        audiomskdemodulator->ConnectSinkDevice(aerol);
        audiooqpskdemodulator->ConnectSinkDevice(aerol);
        audioburstoqpskdemodulator->ConnectSinkDevice(aerol);
        audioburstoqpskdemodulator->demod2->ConnectSinkDevice(aerol2);
        audioburstmskdemodulator->ConnectSinkDevice(aerol);
        aerol->ConnectSinkDevice(ui->console->consoledevice);
        aerol2->ConnectSinkDevice(ui->console->consoledevice);
        ui->console->setEnableUpdates(true);
    }
}

void MainWindow::on_actionRawOutput_triggered()
{
    on_actionConnectToUDPPort_toggled(ui->actionConnectToUDPPort->isChecked());
}

void MainWindow::on_action_Settings_triggered()
{
    settingsdialog->populatesettings();
    if(settingsdialog->exec()==QDialog::Accepted)acceptsettings();
}

void MainWindow::acceptsettings()
{

    ui->statusBar->clearMessage();

    if(settingsdialog->mqtt_enable)
    {
        setLedState(ui->ledmqtt,LedState::Off);
        mqttsubscriber->connectToHost(settingsdialog->mqtt_settings_object);
    }
    else
    {
        setLedState(ui->ledmqtt,LedState::Disable);
        mqttsubscriber->disconnectFromHost();
    }

    aerol->setDoNotDisplaySUs(settingsdialog->donotdisplaysus);
    aerol->setDataBaseDir(settingsdialog->planesfolder);

    aerol2->setDoNotDisplaySUs(settingsdialog->donotdisplaysus);
    aerol2->setDataBaseDir(settingsdialog->planesfolder);

    if(settingsdialog->disablePlaneLogWindow)
    {
        disconnect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)));
        disconnect(aerol2,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)));
        disconnect(mqttsubscriber,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)));
        ui->action_PlaneLog->setVisible(false);
    }
    else
    {
        connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
        connect(aerol2,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
        connect(mqttsubscriber,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)),Qt::UniqueConnection);
        ui->action_PlaneLog->setVisible(true);
    }

    if(settingsdialog->zmqAudioInputEnabled)sourcelabel->setText(" "+settingsdialog->zmqAudioInputTopic+" ");
    else sourcelabel->setText(" "+settingsdialog->audioinputdevice.deviceName()+" ");

    if(settingsdialog->localAudioOutEnabled&&(!ui->actionSound_Out->isVisible()))
    {
        on_actionSound_Out_toggled(false);
        ui->actionSound_Out->setVisible(true);
    }
    else if(!settingsdialog->localAudioOutEnabled)
    {
        on_actionSound_Out_toggled(true);
        ui->actionSound_Out->setVisible(false);
    }

    //start or stop tcp server/client
    if(settingsdialog->tcp_for_ads_messages_enabled)sbs1->starttcpconnection(settingsdialog->tcp_for_ads_messages_address,settingsdialog->tcp_for_ads_messages_port,settingsdialog->tcp_as_client_enabled);
    else sbs1->stoptcpconnection();

    //if soundcard rate changed
    if(typeofdemodtouse==MSK)
    {

        //if bandwidth setting changed then adjust
        if((settingsdialog->widebandwidthenable&&((audiomskdemodulatorsettings.fb==600&&audiomskdemodulatorsettings.Fs==12000)||(audiomskdemodulatorsettings.fb==1200&&audiomskdemodulatorsettings.Fs==24000)))||((!settingsdialog->widebandwidthenable)&&((audiomskdemodulatorsettings.fb==600&&audiomskdemodulatorsettings.Fs==48000)||(audiomskdemodulatorsettings.fb==1200&&audiomskdemodulatorsettings.Fs==48000))))
        {
            if(!settingsdialog->widebandwidthenable)
            {
                if(audiomskdemodulatorsettings.fb==600){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=12000;}
                if(audiomskdemodulatorsettings.fb==1200){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=24000;}
            }
            else
            {
                if(audiomskdemodulatorsettings.fb==600){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=48000;}
                if(audiomskdemodulatorsettings.fb==1200){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=48000;}
            }
            audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
        }

    }
    if(typeofdemodtouse==BURSTMSK)// we only use 48000
    {
        // audioburstmskdemodulatorsettings.Fs=48000;
        // audioburstmskdemodulator->setSettings(audioburstmskdemodulatorsettings);
    }

    if(typeofdemodtouse==OQPSK)//OQPSK uses 48000 all the time so not needed
    {
        //audiooqpskdemodulatorsettings.Fs=48000;
        //audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
    }
    if(typeofdemodtouse==BURSTOQPSK)//BURSTOQPSK uses 48000 all the time so not needed
    {
        //audioburstoqpskdemodulatorsettings.Fs=48000;
        //audioburstoqpskdemodulator->setSettings(audioburstoqpskdemodulatorsettings);
    }

    //if soundcard device changed
    if(typeofdemodtouse==MSK)
    {
        if(audiomskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice || audiomskdemodulatorsettings.zmqAudio != settingsdialog->zmqAudioInputEnabled )
        {
            audiomskdemodulator->stop();
            audiomskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audiomskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
            audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
            audiomskdemodulator->start();
        }
    }
    if(typeofdemodtouse==OQPSK)
    {
        if(audiooqpskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice || audiooqpskdemodulatorsettings.zmqAudio != settingsdialog->zmqAudioInputEnabled)
        {
            audiooqpskdemodulator->stop();
            audiooqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audiooqpskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
            audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
            audiooqpskdemodulator->start();
        }
    }
    if(typeofdemodtouse==BURSTOQPSK)
    {
        if(audioburstoqpskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice || audioburstoqpskdemodulatorsettings.zmqAudio != settingsdialog->zmqAudioInputEnabled)
        {
            audioburstoqpskdemodulator->stop();
            audioburstoqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audioburstoqpskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
            audioburstoqpskdemodulator->setSettings(audioburstoqpskdemodulatorsettings);
            audioburstoqpskdemodulator->start();
        }
    }

    if(typeofdemodtouse==BURSTMSK)
    {
        if(audioburstmskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice || audioburstmskdemodulatorsettings.zmqAudio != settingsdialog->zmqAudioInputEnabled)
        {
            audioburstmskdemodulator->stop();
            audioburstmskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audioburstmskdemodulatorsettings.zmqAudio = settingsdialog->zmqAudioInputEnabled;
            audioburstmskdemodulator->setSettings(audioburstmskdemodulatorsettings);
            audioburstmskdemodulator->start();
        }
    }

    if(settingsdialog->msgdisplayformat=="3")ui->inputwidget->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else ui->inputwidget->setLineWrapMode(QPlainTextEdit::NoWrap);

    planelog->planesfolder=settingsdialog->planesfolder;
    planelog->planelookup=settingsdialog->planelookup;

    //disconnect ports
    for (int i=0;i<feeder_udp_socks.size();i++)
    {
        feeder_udp_socks[i].data()->close();
    }
    feeder_formats.clear();

    //resize number of ports
    int number_of_ports_wanted=settingsdialog->udp_feeders.count();
    while(feeder_udp_socks.size()<number_of_ports_wanted)
    {
        feeder_udp_socks.push_back(new QUdpSocket(this));
    }
    for (int i=number_of_ports_wanted;i<feeder_udp_socks.size();)
    {
        feeder_udp_socks[i].data()->deleteLater();
        feeder_udp_socks.removeAt(i);
    }

    //connect ports
    if(settingsdialog->udp_for_decoded_messages_enabled)
    {
        for (int i=0;i<feeder_udp_socks.size();i++)
        {
            QJsonObject info = settingsdialog->udp_feeders[i].toObject();
            udpSocketConnect(*feeder_udp_socks[i].data(), info["host"].toString(), info["port"].toInt());
            feeder_formats.push_back(info["format"].toString());
        }
    }

    if(settingsdialog->loggingenable)compresseddiskwriter->setLogDir(settingsdialog->loggingdirectory);
    else compresseddiskwriter->setLogDir("");

    //zmq audio in from remote sdr
    if(settingsdialog->zmqAudioInputEnabled)zmq_audio_receiver->Start(settingsdialog->zmqAudioInputAddress, settingsdialog->zmqAudioInputTopic);
    else zmq_audio_receiver->Stop();

    //local audio decode
    if(settingsdialog->localAudioOutEnabled)connect(aerol,SIGNAL(Voicesignal(QByteArray)),ambe,SLOT(to_decode_slot(QByteArray)),Qt::UniqueConnection);
    else disconnect(aerol,SIGNAL(Voicesignal(QByteArray)),ambe,SLOT(to_decode_slot(QByteArray)));

    //zmq compressed audio output
    if(settingsdialog->zmqAudioOutEnabled)
    {
        zmq_audio_sender->Start(settingsdialog->zmqAudioOutBind,settingsdialog->zmqAudioOutTopic);
        connect(aerol,SIGNAL(Voicesignal(QByteArray&, QString&)),zmq_audio_sender,SLOT(Voiceslot(QByteArray&, QString&)),Qt::UniqueConnection);
    }
    else
    {
        zmq_audio_sender->Stop();
        disconnect(aerol,SIGNAL(Voicesignal(QByteArray&, QString&)),zmq_audio_sender,SLOT(Voiceslot(QByteArray&, QString&)));
    }
   if(settingsdialog->beepontextmessage)
   {
        if(!beep)
            beep=new QSound(":/sounds/beep.wav", this);
   }else{
        if(beep)
        {
            beep->deleteLater();
            beep=nullptr;
        }
   }
}

void MainWindow::on_action_PlaneLog_triggered()
{
    planelog->show();
}

void MainWindow::CChannelAssignmentSlot(CChannelAssignmentItem &item)
{
    QString message=QDateTime::currentDateTime().toUTC().toString("hh:mm:ss dd-MM-yy ")+QString("UTC AES:%1 GES:%2 ")
                                                                                            .arg(upperHex(item.AESID, 6, 16, QChar('0')))
                                                                                            .arg(upperHex(item.GESID, 2, 16, QChar('0')));
    QString rx_beam = " Global Beam ";
    if(item.receive_spotbeam)rx_beam=" Spot Beam ";
    message += "Receive Freq: " + QString::number(item.receive_freq, 'f', 4) + rx_beam + "Transmit " + QString::number(item.transmit_freq, 'f', 4);

    switch(item.type)
    {
    case AEROTypeP::MessageType::C_channel_assignment_distress:
        message+=" distress";
        break;
    case AEROTypeP::MessageType::C_channel_assignment_flight_safety:
        message+=" flight safety";
        break;
    case AEROTypeP::MessageType::C_channel_assignment_other_safety:
        message+=" other safety";
        break;
    case AEROTypeP::MessageType::C_channel_assignment_non_safety:
        message+=" non safety";
        break;
    default:
        message+=" unknown type";
    }

    ui->plainTextEdit_cchan_assignment->appendPlainText(message);
}

bool MainWindow::formatACARSItem(const ACARSItem &acarsitem, const QString &msgfmt, QString &humantext, bool &hasMessage)
{
    QByteArray TAKstr;
    TAKstr+=acarsitem.TAK;

    humantext.clear();

    if(msgfmt=="1")
    {
        if(acarsitem.TAK==0x15)TAKstr=((QString)"<NAK>").toLatin1();

        if(acarsitem.nonacars)
        {
            if(acarsitem.message.isEmpty())
            {
                humantext+=QString("ISU: AESID = %1 GESID = %2 QNO = %3 REFNO = %4 REG = %5")
                                .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                                .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                                .arg(upperHex(acarsitem.isuitem.QNO, 2, 16, QChar('0')))
                                .arg(upperHex(acarsitem.isuitem.REFNO, 2, 16, QChar('0')))
                                .arg(acarsitem.PLANEREG.data());
            }
            else
            {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=QString("ISU: AESID = %1 GESID = %2 QNO = %3 REFNO = %4 REG = %5 TEXT = \"%6\"")
                               .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.QNO, 2, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.REFNO, 2, 16, QChar('0')))
                               .arg(acarsitem.PLANEREG.data())
                               .arg(message);
            }
        }
        else
        {
            if(acarsitem.message.isEmpty())
            {
                humantext+=QString("ISU: AESID = %1 GESID = %2 QNO = %3 REFNO = %4 MODE = %5 REG = %6 TAK = %7 LABEL = %8%9 BI = %10")
                               .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.QNO, 2, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.REFNO, 2, 16, QChar('0')))
                               .arg(QChar(acarsitem.MODE))
                               .arg(acarsitem.PLANEREG.data())
                               .arg(TAKstr.data())
                               .arg(upperHex(acarsitem.LABEL[0], 2, 16, QChar('0')))
                               .arg(upperHex(acarsitem.LABEL[1], 2, 16, QChar('0')))
                               .arg(QChar(acarsitem.BI));
            }
            else
            {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=QString("ISU: AESID = %1 GESID = %2 QNO = %3 REFNO = %4 MODE = %5 REG = %6 TAK = %7 LABEL = %8%9 BI = %10 TEXT = \"%11\"")
                        .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                        .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                        .arg(upperHex(acarsitem.isuitem.QNO, 2, 16, QChar('0')))
                        .arg(upperHex(acarsitem.isuitem.REFNO, 2, 16, QChar('0')))
                        .arg(QChar(acarsitem.MODE))
                        .arg(acarsitem.PLANEREG.data())
                        .arg(TAKstr.data())
                        .arg(upperHex(acarsitem.LABEL[0], 2, 16, QChar('0')))
                        .arg(upperHex(acarsitem.LABEL[1], 2, 16, QChar('0')))
                        .arg(QChar(acarsitem.BI))
                        .arg(message);
            }
        }
        if(acarsitem.moretocome)humantext+=" ...more to come... ";
        humantext+="\t( ";
        for(int k=0;k<(acarsitem.isuitem.userdata.size());k++)
        {
            uchar byte=((uchar)acarsitem.isuitem.userdata[k]);
            //byte&=0x7F;
            humantext+=QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();
        }
        humantext+=" )";
        hasMessage=!acarsitem.message.isEmpty();
        return true;
    }

    if(msgfmt=="2")
    {
        humantext+=QDateTime::currentDateTime().toUTC().toString("hh:mm:ss dd-MM-yy ")+"UTC ";
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';

        if(acarsitem.nonacars)
        {
            if(acarsitem.message.isEmpty())
            {
                humantext+=QString("AES:%1 GES:%2 REG:%3")
                               .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                               .arg(acarsitem.PLANEREG.data());
            }
            else
            {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=QString("AES:%1 GES:%2 REG:%3 %4")
                               .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                               .arg(acarsitem.PLANEREG.data())
                               .arg(message);
            }
        }
        else
        {
            if(acarsitem.message.isEmpty())
            {
                humantext+=QString("AES:%1 GES:%2 %3 %4 %5 %6%7 %8")
                               .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                               .arg(QChar(acarsitem.MODE))
                               .arg(acarsitem.PLANEREG.data())
                               .arg(TAKstr.data())
                               .arg(QChar(acarsitem.LABEL[0]))
                               .arg(QChar(label1))
                               .arg(QChar(acarsitem.BI));
            }
            else
            {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=QString("AES:%1 GES:%2 %3 %4 %5 %6%7 %8 %9")
                               .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                               .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                               .arg(QChar(acarsitem.MODE))
                               .arg(acarsitem.PLANEREG.data())
                               .arg(TAKstr.data())
                               .arg(QChar(acarsitem.LABEL[0]))
                               .arg(QChar(label1))
                               .arg(QChar(acarsitem.BI))
                               .arg(message);
            }
        }

        if(acarsitem.moretocome)humantext+=" ...more to come... ";
        hasMessage=!acarsitem.message.isEmpty();
        return true;
    }

    if(msgfmt=="3")
    {
        QString message=acarsitem.message;
        message.replace('\r','\n');
        message.replace("\n\n","\n");
        if(message.right(1)=="\n")message.chop(1);
        if(message.left(1)=="\n")message.remove(0,1);
        message.replace("\n","\n\t");

        humantext+=QDateTime::currentDateTime().toUTC().toString("hh:mm:ss dd-MM-yy ")+"UTC ";
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';

        QByteArray PLANEREG;
        PLANEREG=(((QString)".").repeated(7-acarsitem.PLANEREG.size())).toLatin1()+acarsitem.PLANEREG;
        if(acarsitem.nonacars)
        {
            humantext+=QString("AES:%1 GES:%2   %3       ")
                           .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                           .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                           .arg(PLANEREG.data());
        }
        else
        {
            humantext+=QString("AES:%1 GES:%2 %3 %4 %5 %6%7 %8")
                           .arg(upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0')))
                           .arg(upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0')))
                           .arg(acarsitem.MODE)
                           .arg(PLANEREG.data())
                           .arg(TAKstr.data())
                           .arg(QChar(acarsitem.LABEL[0]))
                           .arg(QChar(label1))
                           .arg(QChar(acarsitem.BI));
        }

        //if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI)+"\n";
        //else humantext+=(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI))+"\n\n\t"+message+"\n";

        if(acarsitem.dblookupresult.size()==QMetaEnum::fromType<DataBaseTextUser::DataBaseSchema>().keyCount())
        {
            QString manufacturer_and_type=acarsitem.dblookupresult[DataBaseTextUser::DataBaseSchema::Manufacturer]+" "+acarsitem.dblookupresult[DataBaseTextUser::DataBaseSchema::Type];
            manufacturer_and_type=manufacturer_and_type.trimmed();
            humantext+=" "+manufacturer_and_type+" "+acarsitem.dblookupresult[DataBaseTextUser::DataBaseSchema::RegisteredOwners];
        }

        if(!acarsitem.message.isEmpty())
        {
            if(!arincparser.downlinkheader.flightid.isEmpty())humantext+=" Flight "+arincparser.downlinkheader.flightid;
            if(arincparser.arincmessage.info.size()>2)
            {
                QString info = arincparser.arincmessage.info;
                info.replace("\n","\n\t");
                humantext+="\n\n\t"+message+"\n\n\t"+info;
            }
            else humantext+="\n\n\t"+message+"\n";
        }
        hasMessage=!acarsitem.message.isEmpty();
        return true;
    }

    if(msgfmt.startsWith("JSON"))
    {
        QString message=acarsitem.message;
        message.replace('\r','\n');
        message.replace("\n\n","\n");
        if(message.right(1)=="\n")message.chop(1);
        if(message.left(1)=="\n")message.remove(0,1);
        message.replace("\n","\n\t");


        QDateTime time=QDateTime::currentDateTimeUtc();
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';

        //json object creation
        QJsonObject json;

        if(msgfmt=="JSON")
        {
            //add database lookup info if available
            if(acarsitem.dblookupresult.size()==QMetaEnum::fromType<DataBaseTextUser::DataBaseSchema>().keyCount())
            {
                json["DB_MANUFACTURER"]=acarsitem.dblookupresult[DataBaseTextUser::DataBaseSchema::Manufacturer].trimmed();
                json["DB_TYPE"]=acarsitem.dblookupresult[DataBaseTextUser::DataBaseSchema::Type].trimmed();
                json["DB_OWNERS"]=acarsitem.dblookupresult[DataBaseTextUser::DataBaseSchema::RegisteredOwners].trimmed();
            }
            //add common things
            json["TIME"]=time.toSecsSinceEpoch();
            json["TIME_UTC"]=time.toUTC().toString("yyyy-MM-dd hh:mm:ss");
            json["NAME"]=QApplication::applicationDisplayName();
            json["NONACARS"]=acarsitem.nonacars;
            json["AESID"]=upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0'));
            json["GESID"]=upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0'));
            json["QNO"]=upperHex(acarsitem.isuitem.QNO, 2, 16, QChar('0'));
            json["REFNO"]=upperHex(acarsitem.isuitem.REFNO, 2, 16, QChar('0'));
            json["REG"]=(QString)acarsitem.PLANEREG;
            //add acars message things
            if(!acarsitem.nonacars)
            {
                json["MODE"]=(QString)acarsitem.MODE;
                json["TAK"]=(QString)TAKstr;
                json["LABEL"]=QString("%1%2").arg(QChar(acarsitem.LABEL[0])).arg(QChar(label1));
                json["BI"]=(QString)acarsitem.BI;
            }
            //if there is a message then add it and any parsing using arincparser
            if(!message.isEmpty())
            {
                json["MESSAGE"]=message;
                if(!arincparser.downlinkheader.flightid.isEmpty())json["FLIGHT"]=arincparser.downlinkheader.flightid;
                if(arincparser.arincmessage.info.size()>2)json["ARINCPARSER_MESSAGE_INFO"]=arincparser.arincmessage.info;
            }
        }
        else if(msgfmt=="JSONdump")
        {
            QJsonObject app;
            app["name"]="JAERO";
            app["ver"]=QApplication::applicationDisplayName();
            json["app"]=QJsonValue(app);

            QJsonObject isu;

            QJsonObject aes;
            aes["type"]="Aircraft Earth Station";
            aes["addr"]=upperHex(acarsitem.isuitem.AESID, 6, 16, QChar('0'));

            QJsonObject ges;
            ges["type"]="Ground Earth Station";
            ges["addr"]=upperHex(acarsitem.isuitem.GESID, 2, 16, QChar('0'));

            if(!acarsitem.nonacars)
            {
                QJsonObject acars;
                acars["mode"]=(QString)acarsitem.MODE;
                acars["ack"]=(QString)TAKstr;
                acars["blk_id"]=(QString)acarsitem.BI;
                acars["label"]=QString("%1%2").arg(QChar(acarsitem.LABEL[0])).arg(QChar(label1));
                acars["reg"]=(QString)acarsitem.PLANEREG;

                if(!arincparser.downlinkheader.flightid.isEmpty())acars["flight"]=arincparser.downlinkheader.flightid;

                if(!message.isEmpty())
                {
                    acars["msg_text"]=message;

                    if (arincparser.arincmessage.info_json.size()>0)
                    {
                        for(auto it=arincparser.arincmessage.info_json.constBegin();it!=arincparser.arincmessage.info_json.constEnd();it++)
                        {
                            acars.insert(it.key(),it.value());
                        }
                    }
                }

                isu["acars"]=QJsonValue(acars);
            }

            isu["refno"]=upperHex(acarsitem.isuitem.REFNO, 2, 16, QChar('0'));
            isu["qno"]=upperHex(acarsitem.isuitem.QNO, 2, 16, QChar('0'));
            isu["src"]=QJsonValue(acarsitem.downlink?aes:ges);
            isu["dst"]=QJsonValue(acarsitem.downlink?ges:aes);

            QJsonObject t;
            QDateTime ts=time.toUTC();
            t["sec"]=ts.toSecsSinceEpoch();
            t["usec"]=(ts.toMSecsSinceEpoch() % 1000) * 1000;
            json["t"]=QJsonValue(t);

            json["isu"]=QJsonValue(isu);

            if(settingsdialog->set_station_id_enabled&&settingsdialog->station_id.size()>0)json["station"]=settingsdialog->station_id;
        }

        //convert json object to string
        humantext=QJsonDocument(json).toJson(QJsonDocument::Compact);
        hasMessage=!message.isEmpty();
        return true;
    }

    return false;
}

//--new method of mainwindow getting second channel from aerol

void MainWindow::ACARSslot(ACARSItem &acarsitem)
{
    if(!acarsitem.valid)return;
    QString humantext;
    QByteArray TAKstr;
    TAKstr+=acarsitem.TAK;

    while(acarsitem.LABEL.size()<2)acarsitem.LABEL.append((char)0x00);

    arincparser.parseDownlinkmessage(acarsitem,settingsdialog->onlyuselibacars);//parse ARINC 745-2 and header
    arincparser.parseUplinkmessage(acarsitem);

    if(acarsitem.hastext&&settingsdialog->beepontextmessage)
    {
        int linecnt=acarsitem.message.count("\n");
        if(acarsitem.message[acarsitem.message.size()-1]=='\n'||acarsitem.message[acarsitem.message.size()-1]=='\r')linecnt--;
        if(linecnt>0)beep->play();//QApplication::beep();
    }

    bool hasMessage=false;

    if(!formatACARSItem(acarsitem,settingsdialog->msgdisplayformat,humantext,hasMessage))
    {
        QString msg=QString(
            "[ERROR] Invalid UDP feeder format settings: '%1' is not a valid message format. "
            "Please go to Decoding tab in Settings and re-select an entry in the Output Format dropdown."
        ).arg(settingsdialog->msgdisplayformat);
        ui->inputwidget->appendPlainText(msg);

        return;
    }

    if((settingsdialog->msgdisplayformat=="1")||(settingsdialog->msgdisplayformat=="2"))
    {
        ui->inputwidget->setLineWrapMode(QPlainTextEdit::NoWrap);
        if((!settingsdialog->dropnontextmsgs)||hasMessage)
        {
            ui->inputwidget->appendPlainText(humantext);
            log(humantext);
        }
    }

    if(settingsdialog->msgdisplayformat=="3")
    {
        ui->inputwidget->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        if((!settingsdialog->dropnontextmsgs)||(hasMessage&&(!acarsitem.nonacars)))
        {
            ui->inputwidget->appendPlainText(humantext);
            log(humantext);
        }
    }

    if(settingsdialog->msgdisplayformat.startsWith("JSON"))
    {
        ui->inputwidget->setLineWrapMode(QPlainTextEdit::NoWrap);

        if((!settingsdialog->dropnontextmsgs)||(hasMessage&&(!acarsitem.nonacars)))
        {
            ui->inputwidget->appendPlainText(humantext);
            log(humantext);
        }
    }

    if(settingsdialog->udp_for_decoded_messages_enabled)
    {
        if((feeder_formats.size()!=feeder_udp_socks.size())||(feeder_udp_socks.size()!=settingsdialog->udp_feeders.size()))
        {
            QString msg="[ERROR] UDP feeder entries mismatch: entry count mismatch. Please delete "
                        "and re-enter all the entries in the Feeder tab in Settings if possible.";
            ui->inputwidget->appendPlainText(msg);

            return;
        }

        for (int i=0;i<feeder_udp_socks.size();i++)
        {
            QString msgfmt=feeder_formats[i];
            QString msgtext;

            bool hasMessage=false;
            bool sendPkt=false;

            if (!formatACARSItem(acarsitem,msgfmt,msgtext,hasMessage))
            {
                QString msg=QString(
                    "[ERROR] Invalid UDP feeder format settings: '%1' is not a valid message format. "
                    "Please delete this entry in the Feeder tab in Settings and add a new entry with a valid display format."
                ).arg(msgfmt);
                ui->inputwidget->appendPlainText(msg);

                continue;
            }

            if ((msgfmt=="1")||(msgfmt=="2")||(msgfmt=="3")) msgtext+="\n";
            if (((msgfmt=="1")||(msgfmt=="2"))&&((!settingsdialog->dropnontextmsgs)||hasMessage)) sendPkt=true;
            if (((msgfmt=="3")||msgfmt.startsWith("JSON"))&&((!settingsdialog->dropnontextmsgs)||(hasMessage&&(!acarsitem.nonacars)))) sendPkt=true;

            if (sendPkt)
            {
                QUdpSocket *sock=feeder_udp_socks[i].data();
                if((!sock->isOpen())||(!sock->isWritable()))
                {
                    sock->close();

                    QJsonObject info=settingsdialog->udp_feeders[i].toObject();
                    udpSocketConnect(*sock, info["host"].toString(), info["port"].toInt());
                }
                if((sock->isOpen())&&(sock->isWritable()))sock->write((msgtext).toLatin1().data());
            }
        }
    }
}

void MainWindow::log(QString &text)
{
    if(!settingsdialog->loggingenable)return;
    if(text.isEmpty())return;
    QDate now=QDateTime::currentDateTimeUtc().date();
    QString nowfilename=settingsdialog->loggingdirectory+"/acars-log-"+now.toString("yy-MM-dd")+".txt";
    if(!filelog.isOpen()||filelog.fileName()!=nowfilename)
    {
        QDir dir;
        dir.mkpath(settingsdialog->loggingdirectory);
        filelog.setFileName(nowfilename);
        if(!filelog.open(QIODevice::Append))return;
        outlogstream.setDevice(&filelog);
    }
    outlogstream << text << '\n';
    outlogstream.flush();
}

void MainWindow::ERRorslot(QString &error)
{
    ui->inputwidget->appendHtml("<font color=\"red\">"+error+"</font>");
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index);
    ui->console->verticalScrollBar()->setValue(ui->console->verticalScrollBar()->maximum());
    ui->inputwidget->verticalScrollBar()->setValue(ui->inputwidget->verticalScrollBar()->maximum());
    ui->plainTextEdit_cchan_assignment->verticalScrollBar()->setValue(ui->plainTextEdit_cchan_assignment->verticalScrollBar()->maximum());

    //finally found a hack that makes the scroll to end not overshoot
    int orgheight=ui->centralWidget->size().height();
    int orgwidth=ui->centralWidget->size().width();
    ui->centralWidget->resize(orgwidth,orgheight+1);
    ui->centralWidget->resize(orgwidth,orgheight);
}

void MainWindow::on_actionSound_Out_toggled(bool mute)
{
    if(ambe->objectName()=="NULL")return;
    if(mute)
    {
        disconnect(ambe,SIGNAL(decoded_signal(QByteArray)),audioout,SLOT(audioin(QByteArray)));
        audioout->stop();
    }
    else
    {
        connect(ambe,SIGNAL(decoded_signal(QByteArray)),audioout,SLOT(audioin(QByteArray)));
        audioout->start();
    }
}

void MainWindow::on_actionReduce_CPU_triggered(bool checked)
{
    audiomskdemodulator->setCPUReduce(checked);
    audiooqpskdemodulator->setCPUReduce(checked);
    audioburstoqpskdemodulator->setCPUReduce(checked);
    audioburstmskdemodulator->setCPUReduce(checked);
}

//periodic info to be sent via UDP if JSON format used
void MainWindow::statusToUDPifJSONset()
{
    if(settingsdialog->udp_for_decoded_messages_enabled)
    {
        //json object creation
        QJsonObject json;
        json["DCD"]=last_dcd;
        json["FREQUENCY"]=last_frequency;
        json["NAME"]=QApplication::applicationDisplayName();
        json["SNR"]=last_EbNo;
        json["TIME"]=QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

        //convert json object to string
        QString humantext=QJsonDocument(json).toJson(QJsonDocument::Compact);

        if((feeder_formats.size()!=feeder_udp_socks.size())||(feeder_formats.size()!=settingsdialog->udp_feeders.size()))
        {
            QString msg="[ERROR] UDP feeder entries mismatch: entry count mismatch. Please delete "
                        "and re-enter all the entries in the Feeder tab in Settings if possible.";
            ui->inputwidget->appendPlainText(msg);
            return;
        }

        for(int i=0;i<feeder_udp_socks.size();i++)
        {
            if(feeder_formats[i]=="JSON")
            {
                QUdpSocket *sock=feeder_udp_socks[i].data();
                if((!sock->isOpen())||(!sock->isWritable()))
                {
                    sock->close();

                    QJsonObject info=settingsdialog->udp_feeders[i].toObject();
                    udpSocketConnect(*sock, info["host"].toString(), info["port"].toInt());
                }
                if((sock->isOpen())&&(sock->isWritable()))sock->write((humantext).toLatin1().data());
            }
        }
    }
}
