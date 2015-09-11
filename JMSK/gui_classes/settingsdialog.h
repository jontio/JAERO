#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSerialPortInfo>
#include "../audiomskmodulator.h"
#include "webscraper.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    enum Device{AUDIO,JDDS};
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    void populatesettings();

    int beaconminidle;
    int beaconmaxidle;
    Device  modulatordevicetype;
    QString Serialportname;
    QString Preamble;
    QString Preamble1;
    QString Preamble2;
    QString Postamble;
    AudioMskModulator::Settings audiomskmodulatorsettings;
    ScrapeMapContainer scrapemapcontainer;

private:
    Ui::SettingsDialog *ui;    
    void poulatepublicvars();
    enum{ SCRAPE_ID,SCRAPE_URL,SCRAPE_REGEXPR,SCRAPE_MINIMAL,SCRAPE_REFRESHVALUE,SCRAPE_DROP_HTMLTAGS,SCRAPE_DEFAULTVALUE};
protected:
    void accept();

private slots:
    void on_spinBoxbeaconmaxidle_editingFinished();
    void on_spinBoxbeaconminidle_editingFinished();
    void on_pushButtonadd_clicked();
    void on_pushButtondel_clicked();
};

#endif // SETTINGSDIALOG_H
