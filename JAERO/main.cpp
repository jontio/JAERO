#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include "gui_classes/settingsdialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("JAERO");
    QApplication::setApplicationVersion(QString(JAERO_VERSION));

    QCommandLineParser cmdparser;
    cmdparser.setApplicationDescription("Demodulate and decode Satcom ACARS");
    cmdparser.addHelpOption();
    cmdparser.addVersionOption();

    QCommandLineOption settingsnameoption(QStringList() << "s" << "settings-name",QApplication::translate("main", "Run with setting name <name>."),QApplication::translate("main", "name"));
    settingsnameoption.setDefaultValue("");
    cmdparser.addOption(settingsnameoption);

    cmdparser.process(a);
    settings_name=cmdparser.value(settingsnameoption);
    if(settings_name.isEmpty())settings_name="JAERO "+QString(JAERO_VERSION);
     else settings_name="JAERO ["+settings_name+"]";

    MainWindow w;
    w.setWindowTitle(settings_name);
    w.show();

    return a.exec();
}



