#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#ifdef __linux__
#include <unistd.h>
#define Sleep(x) usleep(x * 1000);
#endif
#ifdef _WIN32
#include <windows.h>
#endif


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    //create a udp socket and a varicode decoder pipe
    udpsocket = new QUdpSocket(this);
    varicodepipedecoder = new VariCodePipeDecoder(this);

    //default sink is the aerol device
    audiomskdemodulator->ConnectSinkDevice(aerol);

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

    //connections
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
    connect(ui->action_About,    SIGNAL(triggered()),                                   this, SLOT(AboutSlot()));
    connect(audiomskdemodulator, SIGNAL(BitRateChanged(double)),                        aerol,SLOT(setBitRate(double)));

//aeroL connections
    connect(aerol,SIGNAL(DataCarrierDetect(bool)),this,SLOT(DataCarrierDetectStatusSlot(bool)));
    connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),planelog,SLOT(ACARSslot(ACARSItem&)));
    connect(aerol,SIGNAL(ACARSsignal(ACARSItem&)),this,SLOT(ACARSslot(ACARSItem&)));
    connect(audiomskdemodulator, SIGNAL(SignalStatus(bool)),aerol,SLOT(SignalStatusSlot(bool)));


    //load settings
    QSettings settings("Jontisoft", "JAEROL");
    ui->comboBoxafc->setCurrentIndex(settings.value("comboBoxafc",0).toInt());
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

    audiomskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
    audiomskdemodulatorsettings.freq_center=tmpfreq;
    audiomskdemodulator->setSQL(false);
    audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
    audiomskdemodulator->start();

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
    ui->inputwidget->appendHtml("<b>"+QDateTime::currentDateTime().toString("h:mmap ddd d-MMM-yyyy")+" JAERO started</b>");
    QTimer::singleShot(100,ui->inputwidget,SLOT(scrolltoend()));

    ui->actionTXRX->setVisible(false);//there is a hidden audio modulator

    acceptsettings();

}

void MainWindow::closeEvent(QCloseEvent *event)
{

    //save settings
    QSettings settings("Jontisoft", "JAEROL");
    settings.setValue("comboBoxafc", ui->comboBoxafc->currentIndex());
    settings.setValue("comboBoxbps", ui->comboBoxbps->currentIndex());
    settings.setValue("comboBoxlbw", ui->comboBoxlbw->currentIndex());
    settings.setValue("comboBoxdisplay", ui->comboBoxdisplay->currentIndex());
    settings.setValue("actionConnectToUDPPort", ui->actionConnectToUDPPort->isChecked());
    settings.setValue("actionRawOutput", ui->actionRawOutput->isChecked());
    settings.setValue("freq_center", audiomskdemodulator->getCurrentFreq());
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
    if(Volume>0.9)ui->ledvolume->setLED(QIcon::On,QIcon::Disabled);
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
                                     "<H3>v1.0.0</H3>"
                                     "<p>This is a program to demodulate and decode Aero signals. These signals contain SatCom ACARS (<em>Satelitle Comunication Aircraft Communications Addressing and Reporting System</em>) messages as used by planes beyond VHF ACARS range. This protocol is used by Inmarsat's \"Classic Aero\" system and can be received using low gain L band antennas.</p>"
                                     "<p>For more information about this application see <a href=\"http://jontio.zapto.org/hda1/jaero.html\">http://jontio.zapto.org/hda1/jaero.html</a>.</p>"
                                     "<p>Jonti 2015</p>" );
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
    audiomskdemodulatorsettings.fb=arg1.split(" ")[0].toDouble();
    if(audiomskdemodulatorsettings.fb==600){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=12000;}
    if(audiomskdemodulatorsettings.fb==1200){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=24000;}

   // if(audiomskdemodulatorsettings.fb==600){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=48000;}
   // if(audiomskdemodulatorsettings.fb==1200){audiomskdemodulatorsettings.symbolspercycle=12;audiomskdemodulatorsettings.Fs=48000;}

    int idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(audiomskdemodulatorsettings.fb*1.5));
    if(idx>=0)ui->comboBoxlbw->setCurrentIndex(idx);
    audiomskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
    audiomskdemodulator->setSettings(audiomskdemodulatorsettings);

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
}

void MainWindow::on_comboBoxafc_currentIndexChanged(const QString &arg1)
{
    if(arg1=="AFC on")audiomskdemodulator->setAFC(true);
     else audiomskdemodulator->setAFC(false);
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
    if(arg1=="Constellation")audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_constellation);
    if(arg1=="Symbol phase"){audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_phaseoffsetest);ui->scatterplot->setDisksize(6);}
    if(arg1=="None")audiomskdemodulator->setScatterPointType(MskDemodulator::SPT_None);
}

void MainWindow::on_actionConnectToUDPPort_toggled(bool arg1)
{
    audiomskdemodulator->DisconnectSinkDevice();
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
            ui->console->setEnableUpdates(false,"Console disabled while raw demodulated data is routed to UDP port 8765 at LocalHost.");
        }
         else
         {
            audiomskdemodulator->ConnectSinkDevice(aerol);
            aerol->ConnectSinkDevice(udpsocket);
            ui->console->setEnableUpdates(false,"Console disabled while decoded and demodulated data is routed to UDP port 8765 at LocalHost.");
         }

    }
     else
     {
         audiomskdemodulator->ConnectSinkDevice(aerol);
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

    if(audiomskdemodulatorsettings.audio_device_in!=settingsdialog->audioinputdevice)
    {
        audiomskdemodulator->stop();
        audiomskdemodulatorsettings.audio_device_in=settingsdialog->audioinputdevice;
        qDebug()<<audiomskdemodulatorsettings.audio_device_in.deviceName();
        audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
        audiomskdemodulator->start();
    }

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

    //this is how you can change the display format in the lowwer window
    if(settingsdialog->msgdisplayformat=="1")
    {
        if(acarsitem.TAK==0x15)TAKstr=((QString)"<NAK>").toLatin1();
        if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X MODE = %c REG = %s TAK = %s LABEL = %02X%02X BI = %c",acarsitem.isuitem->AESID,acarsitem.isuitem->GESID,acarsitem.isuitem->QNO,acarsitem.isuitem->REFNO,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],(uchar)acarsitem.LABEL[1],acarsitem.BI);
        else humantext+=(((QString)"").sprintf("ISU: AESID = %06X GESID = %02X QNO = %02X REFNO = %02X MODE = %c REG = %s TAK = %s LABEL = %02X%02X BI = %c TEXT = \"",acarsitem.isuitem->AESID,acarsitem.isuitem->GESID,acarsitem.isuitem->QNO,acarsitem.isuitem->REFNO,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],(uchar)acarsitem.LABEL[1],acarsitem.BI)+acarsitem.message+"\"");
        if(acarsitem.moretocome)humantext+=" ...more to come... ";
        humantext+="\t( ";
        for(int k=0;k<(acarsitem.isuitem->userdata.size());k++)
        {
            uchar byte=((uchar)acarsitem.isuitem->userdata[k]);
            //byte&=0x7F;
            humantext+=((QString)"").sprintf("%02X ",byte);
        }
        humantext+=" )";
        if((!settingsdialog->dropnontextmsgs)||(!acarsitem.message.isEmpty()))ui->inputwidget->appendPlainText(humantext);
    }

    if(settingsdialog->msgdisplayformat=="2")
    {
        humantext+=QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ");
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';
        if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem->AESID,acarsitem.isuitem->GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI);
        else humantext+=(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c ",acarsitem.isuitem->AESID,acarsitem.isuitem->GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI))+acarsitem.message;
        if(acarsitem.moretocome)humantext+=" ...more to come... ";
        if((!settingsdialog->dropnontextmsgs)||(!acarsitem.message.isEmpty()))ui->inputwidget->appendPlainText(humantext);
    }

    if(settingsdialog->msgdisplayformat=="3")
    {
        QString message=acarsitem.message;
        if(message.right(1)=="●")message.remove(acarsitem.message.size()-1,1);
        if(message.right(1)!="●")message.append("\n");
        if(message.left(1)!="●")message.prepend("\n\t");
        message.replace("●","\n\t");
        humantext+=QDateTime::currentDateTime().toString("hh:mm:ss dd-MM-yy ");
        if(acarsitem.TAK==0x15)TAKstr=((QString)"!").toLatin1();
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';
        if(acarsitem.message.isEmpty())humantext+=((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c",acarsitem.isuitem->AESID,acarsitem.isuitem->GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI);
        else humantext+=(((QString)"").sprintf("AES:%06X GES:%02X %c %s %s %c%c %c\n\t",acarsitem.isuitem->AESID,acarsitem.isuitem->GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI))+message;
        if(acarsitem.moretocome)humantext+=" ...more to come...\n";
        if((!settingsdialog->dropnontextmsgs)||(!acarsitem.message.isEmpty()))ui->inputwidget->appendPlainText(humantext);
    }

}

void MainWindow::ERRorslot(QString &error)
{
    ui->inputwidget->appendHtml("<font color=\"red\">"+error+"</font>");
}
