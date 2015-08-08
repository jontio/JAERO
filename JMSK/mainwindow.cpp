#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    audiomskdemodulator = new AudioMskDemodulator(this);

    //a udp socket and a varicode decoder as sinks if wanted
    udpsocket = new QUdpSocket(this);
    varicodepipedecoder = new VariCodePipeDecoder(this);

    //default sink is the varicode input of the console
    audiomskdemodulator->ConnectSinkDevice(ui->console->varicodeconsoledevice);

    //console setup
    ui->console->setEnabled(true);
    ui->console->setLocalEchoEnabled(true);

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

    //load settings
    QSettings settings("Jontisoft", "JMSK");
    ui->comboBoxafc->setCurrentIndex(settings.value("comboBoxafc",0).toInt());
    ui->comboBoxsql->setCurrentIndex(settings.value("comboBoxsql",0).toInt());
    ui->comboBoxbps->setCurrentIndex(settings.value("comboBoxbps",0).toInt());
    ui->comboBoxlbw->setCurrentIndex(settings.value("comboBoxlbw",0).toInt());
    ui->comboBoxdisplay->setCurrentIndex(settings.value("comboBoxdisplay",0).toInt());
    ui->actionConnectToUDPPort->setChecked(settings.value("actionConnectToUDPPort",false).toBool());
    ui->actionRawOutput->setChecked(settings.value("actionRawOutput",false).toBool());
    double tmpfreq=settings.value("freq_center",1000).toDouble();


    //set audio msk demodulator settings and start
    on_comboBoxafc_currentIndexChanged(ui->comboBoxafc->currentText());
    on_comboBoxsql_currentIndexChanged(ui->comboBoxsql->currentText());
    on_comboBoxbps_currentIndexChanged(ui->comboBoxbps->currentText());
    on_comboBoxlbw_currentIndexChanged(ui->comboBoxlbw->currentText());
    on_comboBoxdisplay_currentIndexChanged(ui->comboBoxdisplay->currentText());
    on_actionConnectToUDPPort_toggled(ui->actionConnectToUDPPort->isChecked());
    audiomskdemodulatorsettings.freq_center=tmpfreq;
    audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
    audiomskdemodulator->start();

}

MainWindow::~MainWindow()
{

    //save settings
    QSettings settings("Jontisoft", "JMSK");
    settings.setValue("comboBoxafc", ui->comboBoxafc->currentIndex());
    settings.setValue("comboBoxsql", ui->comboBoxsql->currentIndex());
    settings.setValue("comboBoxbps", ui->comboBoxbps->currentIndex());
    settings.setValue("comboBoxlbw", ui->comboBoxlbw->currentIndex());
    settings.setValue("comboBoxdisplay", ui->comboBoxdisplay->currentIndex());
    settings.setValue("actionConnectToUDPPort", ui->actionConnectToUDPPort->isChecked());
    settings.setValue("actionRawOutput", ui->actionRawOutput->isChecked());
    settings.setValue("freq_center", audiomskdemodulator->getCurrentFreq());


    delete ui;
}

void MainWindow::SignalStatusSlot(bool signal)
{
    if(signal)ui->ledsignal->setLED(QIcon::On);
     else ui->ledsignal->setLED(QIcon::Off);
}

void MainWindow::EbNoSlot(double EbNo)
{
    ebnolabel->setText(((QString)" EbNo: %1dB ").arg((int)round(EbNo),2, 10, QChar('0')));
}

void MainWindow::WarningTextSlot(QString warning)
{
    ui->statusBar->showMessage("Warning: "+warning,1000);
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
    QMessageBox::about(this,"JMSK",""
                                     "<H1>An MSK/GMSK demodulator</H1>"
                                     "<p>This is a program to demodulate varicode encoded differentially encoded MSK or GMSK.</p>"
                                     "<p>For more information about this application see <a href=\"http://jontio.zapto.org/hda1/jmsk.html\">http://jontio.zapto.org/hda1/jmsk.html</a>.</p>"
                                     "<p>Jonti 2015</p>" );
}

void MainWindow::on_comboBoxbps_currentIndexChanged(const QString &arg1)
{
    audiomskdemodulatorsettings.fb=arg1.split(" ")[0].toDouble();
    audiomskdemodulatorsettings.Fs=8000;
    if(audiomskdemodulatorsettings.fb==50)audiomskdemodulatorsettings.symbolspercycle=8;
    if(audiomskdemodulatorsettings.fb==125)audiomskdemodulatorsettings.symbolspercycle=16;
    if(audiomskdemodulatorsettings.fb==250)audiomskdemodulatorsettings.symbolspercycle=16;
    if(audiomskdemodulatorsettings.fb==500)audiomskdemodulatorsettings.symbolspercycle=24;
    if(audiomskdemodulatorsettings.fb==1000)audiomskdemodulatorsettings.symbolspercycle=24;
    if(audiomskdemodulatorsettings.fb==1225){audiomskdemodulatorsettings.symbolspercycle=24;audiomskdemodulatorsettings.Fs=22050;}
    int idx=ui->comboBoxlbw->findText(((QString)"%1 Hz").arg(audiomskdemodulatorsettings.fb*1.5));
    if(idx>=0)ui->comboBoxlbw->setCurrentIndex(idx);
    audiomskdemodulatorsettings.freq_center=audiomskdemodulator->getCurrentFreq();
    audiomskdemodulator->setSettings(audiomskdemodulatorsettings);
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

void MainWindow::on_comboBoxsql_currentIndexChanged(const QString &arg1)
{
    if(arg1=="SQL on")audiomskdemodulator->setSQL(true);
     else audiomskdemodulator->setSQL(false);
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
            audiomskdemodulator->ConnectSinkDevice(varicodepipedecoder);
            varicodepipedecoder->ConnectSinkDevice(udpsocket);
            ui->console->setEnableUpdates(false,"Console disabled while varicode decoded demodulated data is routed to UDP port 8765 at LocalHost.");
         }

    }
     else
     {
        ui->actionRawOutput->setEnabled(false);
        ui->console->setEnableUpdates(true);
        audiomskdemodulator->ConnectSinkDevice(ui->console->varicodeconsoledevice);
     }
}

void MainWindow::on_actionRawOutput_triggered()
{
    on_actionConnectToUDPPort_toggled(ui->actionConnectToUDPPort->isChecked());
}
