#include "qscatterplot.h"
#include <QDebug>

QScatterPlot::QScatterPlot(QWidget *parent)
    : QCustomPlot(parent)
{

    timer.start();

    addGraph();
    //xAxis->setVisible(false);
    //yAxis->setVisible(false);
    axisRect()->setAutoMargins(QCP::msNone);
    axisRect()->setMargins(QMargins(0,0,0,0));

    graph(0)->setPen(QPen(Qt::yellow, 0));
    graph(0)->setLineStyle(QCPGraph::lsNone);
    graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));

    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(80, 80, 80));
    plotGradient.setColorAt(1, QColor(50, 50, 50));

    setBackground(QBrush(QColor(Qt::black)));

    xAxis->setRange(-2, 2);
    yAxis->setRange(-2, 2);
}

void QScatterPlot::setDisksize(double disksize)
{
    graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, disksize));
}

void QScatterPlot::setData(const QVector<cpx_type> &points)
{    
    if(timer.elapsed()<50)return;
    timer.start();
    x.resize(points.size());
    y.resize(points.size());
    for(int i=0;i<points.size();i++)
    {
        x[i]=points[i].real();
        y[i]=points[i].imag();
    }
    graph(0)->setData(x,y);
    replot();
}

QScatterPlot::~QScatterPlot()
{

}

