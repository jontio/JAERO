#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#include <QVector>
#include "../DSP.h"

namespace File_Utils
{

namespace Matlab
{
void print(const QString &filename, const char *name, const QVector<cpx_type> &result);
void print(const QString &filename, const char *name, const QVector<double> &result);
}

namespace CPP
{
void print(const QString &filename, const char *name, const QVector<cpx_type> &result);
void print(const QString &filename, const char *name, const QVector<double> &result);
}

}

#endif // FILE_UTILS_H
 
