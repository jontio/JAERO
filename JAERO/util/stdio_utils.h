#ifndef STDIO_UTILS_H
#define STDIO_UTILS_H
#include <QVector>
#include "../DSP.h"

//cpputest doesn't like qDebug() so have to use printf

namespace Stdio_Utils
{

namespace Matlab
{
void print(const char *name,const QVector<cpx_type> &result);
void print(const char *name,const QVector<double> &result);
}

namespace CPP
{
void print(const char *name,const QVector<cpx_type> &result);
void print(const char *name,const QVector<double> &result);
}

}

#endif // STDIO_UTILS_H
