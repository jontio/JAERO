#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include "gui_classes/settingsdialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("JAERO");
    QApplication::setApplicationVersion("1.0.4.4");

    QCommandLineParser cmdparser;
    cmdparser.setApplicationDescription("Demodulatoe and decode Satcom ACARS");
    cmdparser.addHelpOption();
    cmdparser.addVersionOption();

    QCommandLineOption settingsnameoption(QStringList() << "s" << "settings-name",QApplication::translate("main", "Run with setting name <name>."),QApplication::translate("main", "name"));
    settingsnameoption.setDefaultValue("");
    cmdparser.addOption(settingsnameoption);

    cmdparser.process(a);
    settings_name=cmdparser.value(settingsnameoption);
    if(settings_name.isEmpty())settings_name="JAERO";
     else settings_name="JAERO-"+settings_name;

    MainWindow w;
    w.show();

    return a.exec();
}



