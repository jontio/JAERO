#include "stdio_utils.h"

namespace Stdio_Utils
{
namespace Matlab
{

void print(const char *name,const QVector<cpx_type> &result)
{
    for(int k=0;k<result.size();k++)
    {
        if(k==0)printf("%s=[",name);
        if(result[k].imag()>=0)printf("%f+%fi",result[k].real(),result[k].imag());
        else printf("%f%fi",result[k].real(),result[k].imag());
        if((k+1)<result.size())printf(",");
        else printf("];\n");
    }
}

void print(const char *name,const QVector<double> &result)
{
    for(int k=0;k<result.size();k++)
    {
        if(k==0)printf("%s=[",name);
        printf("%f",result[k]);
        if((k+1)<result.size())printf(",");
        else printf("];\n");
    }
}

}

namespace CPP
{

void print(const char *name,const QVector<cpx_type> &result)
{
    for(int k=0;k<result.size();k++)
    {
        if(k==0)printf("QVector<cpx_type> %s={",name);
        printf("cpx_type(%f,%f)",result[k].real(),result[k].imag());
        if((k+1)<result.size())printf(",");
        else printf("};\n");
    }
}

void print(const char *name,const QVector<double> &result)
{
    for(int k=0;k<result.size();k++)
    {
        if(k==0)printf("QVector<double> %s={",name);
        printf("%f",result[k]);
        if((k+1)<result.size())printf(",");
        else printf("};\n");
    }
}

}

}
