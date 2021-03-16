#include "file_utils.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include "RuntimeError.h"

namespace File_Utils
{

static void create_path_if_needed_for(const QString &filename)
{
    QFileInfo fileinfo(filename);
    QDir dir;
    if(!dir.mkpath(fileinfo.absoluteDir().absolutePath()))
    {
        RUNTIME_ERROR("Failed to file path",1);
        return;
    }
}

namespace Matlab
{

void print(const QString &filename,const char *name,const QVector<cpx_type> &result)
{
    create_path_if_needed_for(filename);
    QFile data(filename);
    if(!data.open(QFile::WriteOnly | QFile::Truncate))
    {
        RUNTIME_ERROR("Failed to open file for writing",1);
        return;
    }
    QTextStream out(&data);

    for(int k=0;k<result.size();k++)
    {
        if(k==0)out<<name<<"=[";
        if(result[k].imag()>=0)out<<result[k].real()<<"+"<<result[k].imag()<<"i";
        else out<<result[k].real()<<result[k].imag()<<"i";
        if((k+1)<result.size())out<<",";
        else out<<"];\n";
    }
}

void print(const QString &filename,const char *name,const QVector<double> &result)
{
    create_path_if_needed_for(filename);
    QFile data(filename);
    if(!data.open(QFile::WriteOnly | QFile::Truncate))
    {
        RUNTIME_ERROR("Failed to open file for writing",1);
        return;
    }
    QTextStream out(&data);

    for(int k=0;k<result.size();k++)
    {
        if(k==0)out<<name<<"=[";
        out<<result[k];
        if((k+1)<result.size())out<<",";
        else out<<"];\n";
    }
}

}

namespace CPP
{

void print(const QString &filename,const char *name,const QVector<cpx_type> &result)
{
    create_path_if_needed_for(filename);
    QFile data(filename);
    if(!data.open(QFile::WriteOnly | QFile::Truncate))
    {
        RUNTIME_ERROR("Failed to open file for writing",1);
        return;
    }
    QTextStream out(&data);

    for(int k=0;k<result.size();k++)
    {
        if(k==0)out<<"QVector<cpx_type> "<<name<<"={";
        out<<"cpx_type("<<result[k].real()<<","<<result[k].imag()<<")";
        if((k+1)<result.size())out<<",";
        else out<<"};\n";
    }
}

void print(const QString &filename,const char *name,const QVector<double> &result)
{
    create_path_if_needed_for(filename);
    QFile data(filename);
    if(!data.open(QFile::WriteOnly | QFile::Truncate))
    {
        RUNTIME_ERROR("Failed to open file for writing",1);
        return;
    }
    QTextStream out(&data);

    for(int k=0;k<result.size();k++)
    {
        if(k==0)out<<"QVector<double> "<<name<<"={";
        out<<result[k];
        if((k+1)<result.size())out<<",";
        else out<<"};\n";
    }
}

}

}
 
