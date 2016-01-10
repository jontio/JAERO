#include "qled.h"

QLed::QLed(QWidget *parent)
    : QLabel(parent)
{
    setScaledContents(true);
    icon.addFile(":/images/green_led.svg",size(),QIcon::Normal,QIcon::On);
    icon.addFile(":/images/off_led.svg",size(),QIcon::Normal,QIcon::Off);
    icon.addFile(":/images/red_led.svg",size(),QIcon::Active,QIcon::On);
    setLED(QIcon::Off);
}

void QLed::setLED( QIcon::State state, QIcon::Mode mode)
{
    setPixmap(icon.pixmap(size(),mode,state));
}

QLed::~QLed()
{

}

