#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QStandardPaths>

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

    //create the modulators, varicode encoder pipe, and serial ppt
    varicodepipeencoder = new VariCodePipeEncoder(this);
    audiomskmodulator = new AudioMskModulator(this);  
    aerol = new AeroL(this); //Create Aero L test sink

    //create settings dialog. only some modulator settings are held atm.
    settingsdialog = new SettingsDialog(this);

    //create the demodulator
    audiomskdemodulator = new AudioMskDemodulator(this);

    //create the oqpsk demodulator
    audiooqpskdemodulator = new AudioOqpskDemodulator(this);

    //create a udp socket and a varicode decoder pipe
    udpsocket = new QUdpSocket(this);
    varicodepipedecoder = new VariCodePipeDecoder(this);

    //default sink is the aerol device
    audiomskdemodulator->ConnectSinkDevice(aerol);
    audiooqpskdemodulator->ConnectSinkDevice(aerol);

    //console setup
    ui->console->setEnabled(true);
    ui->console->setLocalEchoEnabled(true);

    //aerol setup
    aerol->ConnectSinkDevice(ui->console->consoledevice);

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

    //load settings
    QSettings settings("Jontisoft", "JAERO");
    ui->comboBoxafc->setCurrentIndex(settings.value("comboBoxafc",1).toInt());
    ui->comboBoxbps->setCurrentIndex(settings.value("comboBoxbps",0).toInt());
    ui->comboBoxlbw->setCurrentIndex(settings.value("comboBoxlbw",0).toInt());
    ui->comboBoxdisplay->setCurrentIndex(settings.value("comboBoxdisplay",0).toInt());
    ui->actionConnectToUDPPort->setChecked(settings.value("actionConnectToUDPPort",false).toBool());
    ui->actionRawOutput->setChecked(settings.value("actionRawOutput",false).toBool());
    double tmpfreq=settings.value("freq_center",1000).toDouble();
    ui->inputwidget->setPlainText(settings.value("inputwidget","").toString());

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
    if(typeofdemodtouse==MSK)audiomskdemodulator->start();

    //oqpsk
    audiooqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audiooqpskdemodulatorsettings.freq_center=tmpfreq;
    audiooqpskdemodulator->setSQL(false);
    audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
    if(typeofdemodtouse==OQPSK)audiooqpskdemodulator->start();

//start modulator setup. the modulator is new and is and add on.

    //connect the encoder to the text widget. the text encoder to the modulator gets done later
    varicodepipeencoder->ConnectSourceDevice(ui->inputwidget->textinputdevice);

    //always connected
    //connect(ui->actionTXRX,SIGNAL(triggered(bool)),ui->inputwidget,SLOT(reset()));//not needed
    connect(ui->actionClearTXWindow,SIGNAL(triggered(bool)),ui->inputwidget,SLOT(clear()));

    //set modulator settings and connections

    settingsdialog->populatesettings();


    //set audio msk modulator settings
    audiomskmodulatorsettings.freq_center=settingsdialog->audiomskmodulatorsettings.freq_center;//this is the only one that gets set
    audiomskmodulatorsettings.secondsbeforereadysignalemited=settingsdialog->audiomskmodulatorsettings.secondsbeforereadysignalemited;//well there is two now
    ui->inputwidget->textinputdevice->preamble1=settingsdialog->Preamble1;
    ui->inputwidget->textinputdevice->preamble2=settingsdialog->Preamble2;
    ui->inputwidget->textinputdevice->postamble=settingsdialog->Postamble;
    ui->inputwidget->reset();
    audiomskmodulator->setSettings(audiomskmodulatorsettings);

    //connections for audio out
    audiomskmodulator->ConnectSourceDevice(varicodepipeencoder);
    connect(ui->actionTXRX,SIGNAL(triggered(bool)),audiomskmodulator,SLOT(startstop(bool)));
    connect(ui->inputwidget->textinputdevice,SIGNAL(eof()),audiomskmodulator,SLOT(stopgraceful()));
    connect(audiomskmodulator,SIGNAL(statechanged(bool)),ui->actionTXRX,SLOT(setChecked(bool)));
    connect(audiomskmodulator,SIGNAL(statechanged(bool)),ui->inputwidget,SLOT(reset()));
    connect(audiomskmodulator,SIGNAL(ReadyState(bool)),ui->inputwidget->textinputdevice,SLOT(SinkReadySlot(bool)));//connection to signal textinputdevice to go from preamble to normal operation


//--end modulator setup

    //add todays date
    //ui->inputwidget->appendHtml("<b>"+QDateTime::currentDateTime().toString("h:mmap ddd d-MMM-yyyy")+" JAERO started</b>");
    //ui->inputwidget->appendPlainText("");
    ui->inputwidget->appendPlainText(QDateTime::currentDateTime().toString("h:mmap ddd d-MMM-yyyy")+" JAERO started\n");
    QTimer::singleShot(100,ui->inputwidget,SLOT(scrolltoend()));

    ui->actionTXRX->setVisible(false);//there is a hidden audio modulator

    acceptsettings();

}

void MainWindow::selectdemodulatorconnections(DemodType demodtype)
{
    static DemodType lastdemodtype=NoDemodType;
    if(demodtype!=lastdemodtype)
    {
        //invalidate demods
        audiomskdemodulator->invalidatesettings();
        audiooqpskdemodulator->invalidatesettings();
    }
    lastdemodtype=demodtype;
    switch(demodtype)
    {
    case MSK: //msk
    default:
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
        connect(audiomskdemodulator, SIGNAL(BitRateChanged(double)),                        aerol,SLOT(setBitRate(double)));
        connect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));

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
        disconnect(audiooqpskdemodulator, SIGNAL(BitRateChanged(double)),                        aerol,SLOT(setBitRate(double)));
        disconnect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
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
        disconnect(audiomskdemodulator, SIGNAL(BitRateChanged(double)),                        aerol,SLOT(setBitRate(double)));
        disconnect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));

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
        connect(audiooqpskdemodulator, SIGNAL(BitRateChanged(double)),                        aerol,SLOT(setBitRate(double)));
        connect(audiooqpskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    //save settings
    QSettings settings("Jontisoft", "JAERO");
    settings.setValue("comboBoxafc", ui->comboBoxafc->currentIndex());
    settings.setValue("comboBoxbps", ui->comboBoxbps->currentIndex());
    settings.setValue("comboBoxlbw", ui->comboBoxlbw->currentIndex());
    settings.setValue("comboBoxdisplay", ui->comboBoxdisplay->currentIndex());
    settings.setValue("actionConnectToUDPPort", ui->actionConnectToUDPPort->isChecked());
    settings.setValue("actionRawOutput", ui->actionRawOutput->isChecked());
    if(typeofdemodtouse==MSK)settings.setValue("freq_center", audiomskdemodulator->getCurrentFreq());
    if(typeofdemodtouse==OQPSK)settings.setValue("freq_center", audiooqpskdemodulator->getCurrentFreq());
    settings.setValue("inputwidget", ui->inputwidget->toPlainText());

    planelog->close();
    event->accept();
}

MainWindow::~MainWindow()
{
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
                                     "<H3>v1.0.3</H3>"
                                     "<p>This is a program to demodulate and decode Aero signals. These signals contain SatCom ACARS (<em>Satelitle Comunication Aircraft Communications Addressing and Reporting System</em>) messages as used by planes beyond VHF ACARS range. This protocol is used by Inmarsat's \"Classic Aero\" system and can be received using low gain L band antennas.</p>"
                                     "<p>For more information about this application see <a href=\"http://jontio.zapto.org/hda1/jaero.html\">http://jontio.zapto.org/hda1/jaero.html</a>.</p>"
                                     "<p>Jonti 2016</p>" );
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

    if(lasttypeofdemodtouse==OQPSK)
    {
        audiooqpskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
        audiomskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
    }

    if(lasttypeofdemodtouse==MSK)
    {
        audiooqpskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
        audiomskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();        
    }

    audiomskdemodulatorsettings.fb=bitrate_tmp;
    audiooqpskdemodulatorsettings.fb=bitrate_tmp;

    if(typeofdemodtouse==OQPSK)
    {
        audiomskdemodulator->stop();
        selectdemodulatorconnections(OQPSK);
        audiooqpskdemodulatorsettings.Fs=48000;
        int idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(audiooqpskdemodulatorsettings.fb*1.0));
        if(idx>=0)audiooqpskdemodulatorsettings.lockingbw=ui->comboBoxlbw->itemText(idx).split(" ")[0].toDouble();
        audiooqpskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
        audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
        if(idx>=0)ui->comboBoxlbw->setCurrentIndex(idx);
        audiooqpskdemodulator->start();
    }

    if(typeofdemodtouse==MSK)
    {
        audiooqpskdemodulator->stop();
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

    //audio modulator setting
    audiomskmodulatorsettings.fb=audiomskdemodulatorsettings.fb;
    audiomskmodulatorsettings.Fs=audiomskdemodulatorsettings.Fs;
    audiomskmodulator->setSettings(audiomskmodulatorsettings);


}

void MainWindow::on_comboBoxlbw_currentIndexChanged(const QString &arg1)
{
    audiomskdemodulatorsettings.lockingbw=arg1.split(" ")[0].toDouble();
    audiomskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
    audiomskdemodulator->setSettings(audiomskdemodulatorsettings);

    audiooqpskdemodulatorsettings.lockingbw=arg1.split(" ")[0].toDouble();
    audiooqpskdemodulatorsettings.freq_center=audiooqpskdemodulator->getCurrentFreq();
    audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
}

void MainWindow::on_comboBoxafc_currentIndexChanged(const QString &arg1)
{
    if(arg1=="AFC on")
    {
        audiomskdemodulator->setAFC(true);
        audiooqpskdemodulator->setAFC(true);
    }
     else
     {
        audiomskdemodulator->setAFC(false);
        audiooqpskdemodulator->setAFC(false);
     }
}

void MainWindow::on_actionCleanConsole_triggered()
{
    ui->console->clear();
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
    }
    if(arg1=="Symbol phase")
    {
        audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_phaseoffsetest);
        audiooqpskdemodulator->setScatterPointType(OqpskDemodulator::SPT_phaseoffsetest);
        ui->scatterplot->setDisksize(6);
    }
    if(arg1=="None")
    {
        audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_None);
        audiooqpskdemodulator->setScatterPointType(OqpskDemodulator::SPT_None);
    }
}

void MainWindow::on_actionConnectToUDPPort_toggled(bool arg1)
{
    audiomskdemodulator->DisconnectSinkDevice();
    audiooqpskdemodulator->DisconnectSinkDevice();
    aerol->DisconnectSinkDevice();
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
            ui->console->setEnableUpdates(false,"Console disabled while raw demodulated data is routed to UDP port 8765 at LocalHost.");
        }
         else
         {
            audiomskdemodulator->ConnectSinkDevice(aerol);
            audiooqpskdemodulator->ConnectSinkDevice(aerol);
            aerol->ConnectSinkDevice(udpsocket);
            ui->console->setEnableUpdates(false,"Console disabled while decoded and demodulated data is routed to UDP port 8765 at LocalHost.");
         }

    }
     else
     {
         audiomskdemodulator->ConnectSinkDevice(aerol);
         audiooqpskdemodulator->ConnectSinkDevice(aerol);
         aerol->ConnectSinkDevice(ui->console->consoledevice);
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

    audiomskmodulator->stop();

    ui->statusBar->clearMessage();
    ui->inputwidget->textinputdevice->preamble1=settingsdialog->Preamble1;
    ui->inputwidget->textinputdevice->preamble2=settingsdialog->Preamble2;
    ui->inputwidget->textinputdevice->postamble=settingsdialog->Postamble;

    audiomskmodulatorsettings.freq_center=settingsdialog->audiomskmodulatorsettings.freq_center;//this is the only one that gets set
    audiomskmodulatorsettings.secondsbeforereadysignalemited=settingsdialog->audiomskmodulatorsettings.secondsbeforereadysignalemited;//well there is two now
    audiomskmodulator->setSettings(audiomskmodulatorsettings);

    aerol->setDoNotDisplaySUs(settingsdialog->donotdisplaysus);
    aerol->setDataBaseDir(settingsdialog->planesfolder);

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
    if(typeofdemodtouse==OQPSK)//OQPSK uses 48000 all the time so not needed
    {
        //audiooqpskdemodulatorsettings.Fs=48000;
        //audiooqpskdemodulator->setSettings(audiooqpskdemodulatorsettings);
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

    if(settingsdialog->msgdisplayformat=="3")ui->inputwidget->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else ui->inputwidget->setLineWrapMode(QPlainTextEdit::NoWrap);

    planelog->planesfolder=settingsdialog->planesfolder;
    planelog->planelookup=settingsdialog->planelookup;


}

void MainWindow::on_action_PlaneLog_triggered()
{
    planelog->show();
}

//--new method of mainwindow getting second channel from aerol

void MainWindow::ACARSslot(ACARSItem &acarsitem)
{
    if(!acarsitem.valid)return;
    QString humantext;
    QByteArray TAKstr;
    TAKstr+=acarsitem.TAK;

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
        if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X MODE = %c REG = %s TAK = %s LABEL = %02X%02X BI = %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.isuitem.QNO,acarsitem.isuitem.REFNO,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],(uchar)acarsitem.LABEL[1],acarsitem.BI);
         else
         {
            QString message=acarsitem.message;
            message.replace('\r','\n');
            message.replace("\n\n","\n");
            message.replace('\n',"●");
            humantext+=(((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X MODE = %c REG = %s TAK = %s LABEL = %02X%02X BI = %c TEXT = \"",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.isuitem.QNO,acarsitem.isuitem.REFNO,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],(uchar)acarsitem.LABEL[1],acarsitem.BI)+message+"\"");
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
        if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI);
         else
         {
            QString message=acarsitem.message;
            message.replace('\r','\n');
            message.replace("\n\n","\n");
            message.replace('\n',"●");
            humantext+=(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c ",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI))+message;
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
        if(message.right(1)=="\n")message.remove(acarsitem.message.size()-1,1);
        if(message.left(1)=="\n")message.remove(0,1);
        message.replace("\n","\n\t");



        humantext+=QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ");
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';


        humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI);

        //if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI)+"\n";
        //else humantext+=(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI))+"\n\n\t"+message+"\n";

        if(acarsitem.dblookupresult.size()==9)
        {
            humantext+=" "+acarsitem.dblookupresult[6]+" "+acarsitem.dblookupresult[7];
        }

        if(!acarsitem.message.isEmpty())humantext+="\n\n\t"+message+"\n";
        //humantext+="\n";

        if(acarsitem.moretocome)humantext+=" ...more to come...\n";



        if((!settingsdialog->dropnontextmsgs)||(!acarsitem.message.isEmpty()))
        {
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
}

void MainWindow::ERRorslot(QString &error)
{
    ui->inputwidget->appendHtml("<font color=\"red\">"+error+"</font>");
}
