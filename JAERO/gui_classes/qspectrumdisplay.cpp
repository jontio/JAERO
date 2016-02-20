#include "qspectrumdisplay.h"
#include <assert.h>
#include <QDebug>

QSpectrumDisplay::QSpectrumDisplay(QWidget *parent)
    : QCustomPlot(parent)
{

    fftr = new FFTr(pow(2,SPECTRUM_FFT_POWER),false);
    addGraph();

    timer.start();

    Fs=8000;

    xAxis->setRange(0, Fs/2);
    yAxis->setRange(20, 30);

    freq_center=1000;
    fb=125;

    yAxis->setVisible(false);
    axisRect()->setAutoMargins(QCP::msBottom);
    axisRect()->setMargins(QMargins(0,0,1,0));

    lockingbwbar = new QCPBars(xAxis, yAxis);
    addPlottable(lockingbwbar);
    freqmarker = new QCPBars(xAxis, yAxis);
    addPlottable(freqmarker);
    freqmarker->setWidth(10);
    lockingbwbar->setPen(Qt::NoPen);
    lockingbwbar->setBrush(QColor(10, 140, 70, 80));
    freqmarker->setPen(Qt::NoPen);
    freqmarker->setBrush(QColor(Qt::yellow));


    nfft=pow(2,SPECTRUM_FFT_POWER);
    hzperbin=Fs/nfft;

    out.resize(pow(2,SPECTRUM_FFT_POWER));
    in.resize(pow(2,SPECTRUM_FFT_POWER));

    spec_log_abs_vals.resize(pow(2,SPECTRUM_FFT_POWER)/2);
    spec_freq_vals.resize(pow(2,SPECTRUM_FFT_POWER)/2);
    for(int i=0;i<spec_freq_vals.size();i++)
    {
        spec_freq_vals[i]=((double)i)*hzperbin;
    }

    hann_window.resize(pow(2,SPECTRUM_FFT_POWER));
    for(int i=0;i<hann_window.size();i++)
    {
        hann_window[i]=0.5*(1.0-cos(2*M_PI*((double)i)/(nfft-1.0)));
    }

    lockingbwbar_x.push_back(1000);
    lockingbwbar_y.push_back(0);
    freqmarker_x.push_back(1000);
    freqmarker_y.push_back(0);

    lockingbwbar->setWidth(500);

}

void QSpectrumDisplay::setSampleRate(double samplerate)
{
    Fs=samplerate;
    xAxis->setRange(0, Fs/2);
    hzperbin=Fs/nfft;
    for(int i=0;i<spec_freq_vals.size();i++)
    {
        spec_freq_vals[i]=((double)i)*hzperbin;
    }    
}

QSpectrumDisplay::~QSpectrumDisplay()
{
    delete fftr;
}

void QSpectrumDisplay::setPlottables(double freq_est,double _freq_center,double bandwidth)
{
    freq_center=_freq_center;
    lockingbwbar_x[0]=freq_center;
    lockingbwbar_y[0]=yAxis->range().upper;
    lockingbwbar->setWidth(bandwidth);
    lockingbwbar->setData(lockingbwbar_x, lockingbwbar_y);

    freqmarker_x[0]=freq_est;
    freqmarker_y[0]=0.75*(yAxis->range().upper-yAxis->range().lower)+yAxis->range().lower;
    freqmarker->setData(freqmarker_x, freqmarker_y);

    if(timer.elapsed()>100)
    {
        timer.start();
        replot();
    }
}

void QSpectrumDisplay::setFFTData(const QVector<double> &data)
{
    assert(hann_window.size()==data.size());
    for(int i=0;i<data.size();i++)
    {
        in[i]=hann_window[i]*data[i];
    }
    fftr->transform(in,out);
    double maxval=0;
    double aveval=0;
    for(int i=0;i<spec_log_abs_vals.size();i++)
    {
        spec_log_abs_vals[i]=spec_log_abs_vals[i]*0.7+0.3*10*log10(fmax(100000.0*abs((2./nfft)*out[i]),1));
        if(spec_log_abs_vals[i]>maxval)
        {
            maxval=spec_log_abs_vals[i];
        }
        aveval+=spec_log_abs_vals[i];
    }
    aveval/=spec_log_abs_vals.size();
    if((maxval-aveval)<10)
    {
        maxval=aveval+10.0;
    }

 //   spec_log_abs_vals=data;

    graph(0)->setData(spec_freq_vals,spec_log_abs_vals);
    //yAxis->setRange(yAxis->range().lower*0.9+0.1*(aveval*0.5), yAxis->range().upper*0.9+0.1*(maxval*1.1));
    yAxis->setRange(aveval-2, yAxis->range().upper*0.5+0.5*(maxval+1));

    lockingbwbar_y[0]=yAxis->range().upper;
    lockingbwbar->setData(lockingbwbar_x, lockingbwbar_y);
    freqmarker_y[0]=0.75*(yAxis->range().upper-yAxis->range().lower)+yAxis->range().lower;
    freqmarker->setData(freqmarker_x, freqmarker_y);

    if(timer.elapsed()>100)
    {
        timer.start();
        replot();
    }

}

void QSpectrumDisplay::mousePressEvent(QMouseEvent *event)
{
    if(event->buttons()&Qt::LeftButton)
    {
        QPoint point=event->pos();
        double cursorX=xAxis->pixelToCoord(point.x());
        if(cursorX>(Fs/2-lockingbwbar->width()/2))
        {
            cursorX=(Fs/2-lockingbwbar->width()/2)-1.0;
        }
        if(cursorX<(lockingbwbar->width()/2))
        {
            cursorX=(lockingbwbar->width()/2)+1.0;
        }
        lockingbwbar_x[0]=cursorX;
        lockingbwbar->setData(lockingbwbar_x, lockingbwbar_y);
        CenterFreqChanged(cursorX);
        replot();
    }
}

void QSpectrumDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons()&Qt::LeftButton)
    {
        QPoint point=event->pos();
        double cursorX=xAxis->pixelToCoord(point.x());
        if(cursorX>(Fs/2-lockingbwbar->width()/2))
        {
            cursorX=(Fs/2-lockingbwbar->width()/2);
        }
        if(cursorX<(lockingbwbar->width()/2))
        {
            cursorX=(lockingbwbar->width()/2)+1.0-1.0;
        }
        lockingbwbar_x[0]=cursorX;
        lockingbwbar->setData(lockingbwbar_x, lockingbwbar_y);
        CenterFreqChanged(cursorX);
        replot();
    }

}



