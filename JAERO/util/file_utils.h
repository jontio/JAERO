#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#include <QVector>

namespace File_Utils
{
	
typedef std::complex<double> cpx_type;

namespace Matlab
{
void print(const char *filename,const char *name,const QVector<cpx_type> &result);
void print(const char *filename,const char *name,const QVector<double> &result);
}

namespace CPP
{
void print(const char *filename,const char *name,const QVector<cpx_type> &result);
void print(const char *filename,const char *name,const QVector<double> &result);
}

}

#endif // FILE_UTILS_H
 
