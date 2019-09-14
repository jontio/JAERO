#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QStandardPaths>
#include <QLibrary>

#ifdef __linux__
#include <unistd.h>
#define Sleep(x) usleep(x * 1000);
#endif
#ifdef _WIN32
#include <windows.h>
#endif


#include "databasetext.h"


/*void MainWindow::result(bool ok, int ref, const QStringList &result)
{
    ACARSItem *pai=((ACARSItem*)dbtu->getuserdata(ref));
    if(!ok)
    {
        if(result.size())
        {
             qDebug()<<"Error: "<<result[0];
        } else qDebug()<<"Error: Unknowen";
    }
    else
    {
        qDebug()<<"ok"<<result;
        qDebug()<<pai->message;
    }
    delete pai;
}*/



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    beep=new QSound(":/sounds/beep.wav",this);

    //example of using db
    /*dbtu=new DataBaseTextUser(this);
    connect(dbtu,SIGNAL(result(bool,int,QStringList)),this,SLOT(result(bool,int,QStringList)));
    ACARSItem *pai=new ACARSItem;
    QString dirname,aes;
    dirname="../";
    aes="7582F8";
    pai->message="jhbuybuy000";
    dbtu->request(dirname,aes,pai);*/

    //plane logging window
    planelog = new PlaneLog;

    //create areo decoder
    aerol = new AeroL(this); //Create Aero sink
    aerol2 = new AeroL(this); //Create Aero sink

    //ambe decompressor
    QString aeroambe_object_error_str;
    QLibrary library("aeroambe.dll");
    if(!library.load())library.setFileName("aeroambe.so");
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

    //create udp sockets
    udpsocket = new QUdpSocket(this);
    udpsocket_bottom_textedit = new QUdpSocket(this);

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
    connect(aerol,SIGNAL(Voicesignal(QByteArray)),ambe,SLOT(to_decode_slot(QByteArray)));

   // compresseddiskwriter->openFileForOutput("e:/delme.ogg");

    //statusbar setup
    freqlabel = new QLabel();
    ebnolabel = new QLabel();
    ui->statusBar->addPermanentWidget(new QLabel());
    ui->statusBar->addPermanentWidget(freqlabel);
    ui->statusBar->addPermanentWidget(ebnolabel);

    //led setup
    ui->ledvolume->setLED(QIcon::Off);
    ui->ledsignal->setLED(QIcon::Off);

    //misc connections
    connect(ui->action_About,    SIGNAL(triggered()),                                   this, SLOT(AboutSlot()));

    //select msk or oqpsk
    selectdemodulatorconnections(MSK);
    typeofdemodtouse=NoDemodType;

    //aeroL connections
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),this,SLOT(DataCarrierDetectStatusSlot(bool)));
    connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)));
    connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),this,SLOT(ACARSslot(ACARSItem&)));
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),audiomskdemodulator,SLOT(DCDstatSlot(bool)));
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),audioburstmskdemodulator,SLOT(DCDstatSlot(bool)));
    connect(aerol,SIGNAL(CChannelAssignmentSignal(CChannelAssignmentItem&)),this,SLOT(CChannelAssignmentSlot(CChannelAssignmentItem&)));
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),audiooqpskdemodulator,SLOT(DCDstatSlot(bool)));
    connect(aerol,SIGNAL(Call_progress_Signal(QByteArray)),compresseddiskwriter,SLOT(Call_progress_Slot(QByteArray)));

    //aeroL2 connections
    connect(aerol2,SIGNAL(DataCarrierDetect(bool)),this,SLOT(DataCarrierDetectStatusSlot(bool)));
    connect(aerol2,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)));
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
    if(typeofdemodtouse==MSK)audiomskdemodulator->start();

    //oqpsk
    audiooqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audiooqpskdemodulatorsettings.freq_center=tmpfreq;
    audiooqpskdemodulator->setSQL(false);
    audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
    audiooqpskdemodulator->setCPUReduce(ui->actionReduce_CPU->isChecked());
    if(typeofdemodtouse==OQPSK)audiooqpskdemodulator->start();

    //burstoqpsk
    audioburstoqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audioburstoqpskdemodulatorsettings.freq_center=tmpfreq;
    audioburstoqpskdemodulator->setSQL(false);
    audioburstoqpskdemodulator->setSettings(audioburstoqpskdemodulatorsettings);
    if(typeofdemodtouse==BURSTOQPSK)audioburstoqpskdemodulator->start();

    //burst msk
    audioburstmskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audioburstmskdemodulatorsettings.freq_center=tmpfreq;
    audioburstmskdemodulator->setSQL(false);
    audioburstmskdemodulator->setSettings(audioburstmskdemodulatorsettings);
    if(typeofdemodtouse==BURSTMSK)audioburstmskdemodulator->start();

    //add todays date
    ui->inputwidget->appendPlainText(QDateTime::currentDateTime().toString("h:mmap ddd d-MMM-yyyy")+" JAERO started\n");
    QTimer::singleShot(100,ui->inputwidget,SLOT(scrolltoend()));

    //say if aeroabme was not found or loaded correctly
    if(!aeroambe_object_error_str.isEmpty())ui->inputwidget->appendPlainText(aeroambe_object_error_str+"\n");

    ui->actionTXRX->setVisible(false);//there is a hidden audio modulator icon.

    //set pop and accept settings
    settingsdialog->populatesettings();
    acceptsettings();

    /*QString teststr="F58ADL0040/AKLCDYA.ADS.N705DN07EEE19454DAC7D010D21D0DEEEC44556208024029F0588C71D7884D000E13B90F00000F12C1A280001029305F1019F4";
    // //teststr="F61AQF0027#M1B/B6 AKLCDYA.ADS.VH-OEI14DC715BE394C80ED02D1D0DDBBBBBEEEEC80E8258D82D843333080E800F85";
    teststr="A92AXA42FD#M1B/B6 OAKODYA.ADS..N42FD0724FE94A9BECAFC4DF01F0D24FA4CAAAACAFC802D238E3CE38E4AFC800E24B0F540040F25899FC004101420DEB82858";
    teststr="F21AUA082/FUKJJYA.ADS.N772UA070DD5F32FBD894736B79D1602BC7B5928E04EA01600C97B8938806560171165E328E289C408AA0D0EEF9B2D8D897302AD11C88B282289C4000E7B50F780000F79F9A30000BB12";
    //teststr="F42AUA0828/FUKJJYA.ADS.N772UA07150C231FE549470E3D1D1600DF765128E01B401601F479E128E055C01603557D7128E0B5801601DF042928E0E6C0160073043908F8F2E01600B47DC8CC69078017188A7B1E12C57D48AB0D1556AB1E84094700DA160C0B1C7A8947000E76";//5";

    teststr="J84ACI0017/OAKODYA.ADSB-183560301D095";
    ACARSItem tmpitem;
    tmpitem.message=teststr;
    tmpitem.downlink=true;
    tmpitem.nonacars=false;
    if(arincparser.parseDownlinkmessage(tmpitem))
    {
       qDebug()<<arincparser.arincmessage.info.toLatin1().data();
    }*/

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
    if(dcd)ui->leddata->setLED(QIcon::On);
     else ui->leddata->setLED(QIcon::Off);
}

void MainWindow::EbNoSlot(double EbNo)
{
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
    QString str=(((QString)"%1Hz  ").arg(freq_est,0, 'f', 2)).rightJustified(11,' ');
    freqlabel->setText("  Freq: "+str);
}

void MainWindow::AboutSlot()
{
    QMessageBox::about(this,"JAERO",""
                                     "<H1>An Aero demodulator and decoder</H1>"
                                     "<H3>v1.0.4.10</H3>"
                                     "<p>This is a program to demodulate and decode Aero signals. These signals contain SatCom ACARS (<em>Satelitle Comunication Aircraft Communications Addressing and Reporting System</em>) messages as used by planes beyond VHF ACARS range. This protocol is used by Inmarsat's \"Classic Aero\" system and can be received using low or medium gain L band or high gain C band antennas.</p>"
                                     "<p>For more information about this application see <a href=\"http://jontio.zapto.org/hda1/jaero.html\">http://jontio.zapto.org/hda1/jaero.html</a>.</p>"
                                     "<p>Jonti 2019</p>" );
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
    if(bitrate_tmp>1200){typeofdemodtouse=OQPSK;}
     else {typeofdemodtouse=MSK;
    }
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


    if(typeofdemodtouse==BURSTOQPSK)
    {
        audiomskdemodulator->stop();
        audiooqpskdemodulator->stop();
        audioburstmskdemodulator->stop();
        selectdemodulatorconnections(BURSTOQPSK);
        audioburstoqpskdemodulatorsettings.Fs=48000;
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
    ui->scatterplot->graph(0)->clearData();
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
    udpsocket_bottom_textedit->close();
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

    aerol->setDoNotDisplaySUs(settingsdialog->donotdisplaysus);
    aerol->setDataBaseDir(settingsdialog->planesfolder);

    aerol2->setDoNotDisplaySUs(settingsdialog->donotdisplaysus);
    aerol2->setDataBaseDir(settingsdialog->planesfolder);

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
        if(audiomskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice)
        {
            audiomskdemodulator->stop();
            audiomskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
            audiomskdemodulator->start();
        }
    }
    if(typeofdemodtouse==OQPSK)
    {
        if(audiooqpskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice)
        {
            audiooqpskdemodulator->stop();
            audiooqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
            audiooqpskdemodulator->start();
        }
    }
    if(typeofdemodtouse==BURSTOQPSK)
    {
        if(audioburstoqpskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice)
        {
            audioburstoqpskdemodulator->stop();
            audioburstoqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audioburstoqpskdemodulator->setSettings(audioburstoqpskdemodulatorsettings);
            audioburstoqpskdemodulator->start();
        }
    }

    if(typeofdemodtouse==BURSTMSK)
    {
        if(audioburstmskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice)
        {
            audioburstmskdemodulator->stop();
            audioburstmskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
            audioburstmskdemodulator->setSettings(audioburstmskdemodulatorsettings);
            audioburstmskdemodulator->start();
        }
    }

    if(settingsdialog->msgdisplayformat=="3")ui->inputwidget->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else ui->inputwidget->setLineWrapMode(QPlainTextEdit::NoWrap);

    planelog->planesfolder=settingsdialog->planesfolder;
    planelog->planelookup=settingsdialog->planelookup;
    udpsocket_bottom_textedit->close();
    if(settingsdialog->udp_for_decoded_messages_enabled)
    {
        udpsocket_bottom_textedit->connectToHost(settingsdialog->udp_for_decoded_messages_address, settingsdialog->udp_for_decoded_messages_port);
    }

    if(settingsdialog->loggingenable)compresseddiskwriter->setLogDir(settingsdialog->loggingdirectory);
     else compresseddiskwriter->setLogDir("");

}

void MainWindow::on_action_PlaneLog_triggered()
{
    planelog->show();
}

void MainWindow::CChannelAssignmentSlot(CChannelAssignmentItem &item)
{
    QString message=QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ")+((QString)"").sprintf("AES:%06X GES:%02X ",item.AESID,item.GESID);
    QString rx_beam = " Global Beam ";
    if(item.receive_spotbeam)rx_beam=" Spot Beam ";
    message += "Receive Freq: " + QString::number(item.receive_freq) + rx_beam + "Transmit " + QString::number(item.transmit_freq);


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

//--new method of mainwindow getting second channel from aerol

void MainWindow::ACARSslot(ACARSItem &acarsitem)
{
    if(!acarsitem.valid)return;
    QString humantext;
    QByteArray TAKstr;
    TAKstr+=acarsitem.TAK;

    arincparser.parseDownlinkmessage(acarsitem);//parse ARINC 745-2 and header
    arincparser.parseUplinkmessage(acarsitem);

    if(acarsitem.hastext&&settingsdialog->beepontextmessage)
    {
        int linecnt=acarsitem.message.count("\n");
        if(acarsitem.message[acarsitem.message.size()-1]=='\n'||acarsitem.message[acarsitem.message.size()-1]=='\r')linecnt--;
        if(linecnt>0)beep->play();//QApplication::beep();
    }

    //this is how you can change the display format in the lowwer window
    if(settingsdialog->msgdisplayformat=="1")
    {
        ui->inputwidget->setLineWrapMode(QPlainTextEdit::NoWrap);
        if(acarsitem.TAK==0x15)TAKstr=((QString)"<NAK>").toLatin1();

        if(acarsitem.nonacars)
        {
            if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X REG = %s",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.isuitem.QNO,acarsitem.isuitem.REFNO,acarsitem.PLANEREG.data());
             else
             {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=(((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X REG = %s TEXT = \"",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.isuitem.QNO,acarsitem.isuitem.REFNO,acarsitem.PLANEREG.data())+message+"\"");
             }
        }
         else
         {
            if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X MODE = %c REG = %s TAK = %s LABEL = %02X%02X BI = %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.isuitem.QNO,acarsitem.isuitem.REFNO,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],(uchar)acarsitem.LABEL[1],acarsitem.BI);
             else
             {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=(((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X MODE = %c REG = %s TAK = %s LABEL = %02X%02X BI = %c TEXT = \"",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.isuitem.QNO,acarsitem.isuitem.REFNO,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],(uchar)acarsitem.LABEL[1],acarsitem.BI)+message+"\"");
             }
         }
        if(acarsitem.moretocome)humantext+=" ...more to come... ";
        humantext+="\t( ";
        for(int k=0;k<(acarsitem.isuitem.userdata.size());k++)
        {
            uchar byte=((uchar)acarsitem.isuitem.userdata[k]);
            //byte&=0x7F;
            humantext+=((QString)"").sprintf("%02X ",byte);
        }
        humantext+=" )";
        if((!settingsdialog->dropnontextmsgs)||(!acarsitem.message.isEmpty()))
        {
            ui->inputwidget->appendPlainText(humantext);
            log(humantext);
        }
    }

    if(settingsdialog->msgdisplayformat=="2")
    {
        ui->inputwidget->setLineWrapMode(QPlainTextEdit::NoWrap);
        humantext+=QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ");
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';

        if(acarsitem.nonacars)
        {
            if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X REG:%s",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.PLANEREG.data());
             else
             {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=(((QString)"").sprintf("AES:%06X GES:%02X REG:%s ",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.PLANEREG.data()))+message;
             }
        }
         else
         {
            if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI);
             else
             {
                QString message=acarsitem.message;
                message.replace('\r','\n');
                message.replace("\n\n","\n");
                message.replace('\n',"●");
                humantext+=(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c ",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI))+message;
             }
         }

        if(acarsitem.moretocome)humantext+=" ...more to come... ";
        if((!settingsdialog->dropnontextmsgs)||(!acarsitem.message.isEmpty()))
        {
            ui->inputwidget->appendPlainText(humantext);
            log(humantext);
        }
    }

    if(settingsdialog->msgdisplayformat=="3")
    {
        ui->inputwidget->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        QString message=acarsitem.message;
        message.replace('\r','\n');
        message.replace("\n\n","\n");
        if(message.right(1)=="\n")message.chop(1);//.remove(acarsitem.message.size()-1,1);
        if(message.left(1)=="\n")message.remove(0,1);
        message.replace("\n","\n\t");

        humantext+=QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ");
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';

        QByteArray PLANEREG;
        PLANEREG=(((QString)".").repeated(7-acarsitem.PLANEREG.size())).toLatin1()+acarsitem.PLANEREG;
        if(acarsitem.nonacars) humantext+=((QString)"").sprintf("AES:%06X GES:%02X   %s       ",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,PLANEREG.data());
         else humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI);

        //if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI)+"\n";
        //else humantext+=(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI))+"\n\n\t"+message+"\n";

        if(acarsitem.dblookupresult.size()==5)
        {
            humantext+=" "+acarsitem.dblookupresult[3]+" "+acarsitem.dblookupresult[4];
        }

        if(!acarsitem.message.isEmpty())
        {
            if(!arincparser.downlinkheader.flightid.isEmpty())humantext+=" Flight "+arincparser.downlinkheader.flightid;
            if(arincparser.arincmessage.info.size()>2)
            {
                arincparser.arincmessage.info.replace("\n","\n\t");
                humantext+="\n\n\t"+message+"\n\n\t"+arincparser.arincmessage.info;
            }
            else humantext+="\n\n\t"+message+"\n";
        }

        if((!settingsdialog->dropnontextmsgs)||(!acarsitem.message.isEmpty()&&(!acarsitem.nonacars)))
        {
            if(settingsdialog->udp_for_decoded_messages_enabled)//((settingsdialog->udp_for_decoded_messages_enabled)&&(arincparser.adownlinkbasicreportgroup.valid))
            {
                if((!udpsocket_bottom_textedit->isOpen())||(!udpsocket_bottom_textedit->isWritable()))
                {
                    udpsocket_bottom_textedit->close();
                    udpsocket_bottom_textedit->connectToHost(settingsdialog->udp_for_decoded_messages_address, settingsdialog->udp_for_decoded_messages_port);
                }
                if((udpsocket_bottom_textedit->isOpen())&&(udpsocket_bottom_textedit->isWritable()))udpsocket_bottom_textedit->write((humantext+"\n").toLatin1().data());
            }
            ui->inputwidget->appendPlainText(humantext);
            log(humantext);
        }
    }

}

void MainWindow::log(QString &text)
{
    if(!settingsdialog->loggingenable)return;
    if(text.isEmpty())return;
    QDate now=QDate::currentDate();
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

    audiooqpskdemodulator->setCPUReduce(checked);
    audiomskdemodulator->setCPUReduce(checked);

}
