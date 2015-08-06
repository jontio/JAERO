#ifndef QSCATTERPLOT_H
#define QSCATTERPLOT_H

#include <QObject>
#include <complex>
#include <QElapsedTimer>

#include "../../qcustomplot/qcustomplot.h"

typedef std::complex<double> cpx_type;

class QScatterPlot : public QCustomPlot
{
    Q_OBJECT
public:
    explicit QScatterPlot(QWidget *parent = 0);
    ~QScatterPlot();
    void setDisksize(double disksize);
public slots:
    void setData(const QVector<cpx_type> &points);
private:
    QVector<double> x;
    QVector<double> y;
    QElapsedTimer timer;
};

#endif // QSCATTERPLOT_H
