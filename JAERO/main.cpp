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
    settings_name=cmdparser.value(settingsnameoption).trimmed();
    if(settings_name.isEmpty())
    {
        QApplication::setApplicationDisplayName(QString(JAERO_VERSION));
        settings_name="JAERO";
    }
    else
    {
        QApplication::setApplicationDisplayName(settings_name);
        settings_name="JAERO ["+settings_name+"]";
    }

    MainWindow w;
    w.show();

    return a.exec();
}



