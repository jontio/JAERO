//    Copyright (C) 2015  Jonti Olds

#ifndef DSPH
#define DSPH
//---------------------------------------------------------------------------
#include <math.h>
#include <vector>
#include <complex>
#include <QObject>
#include <assert.h>
#include <QVector>
#include "kiss_fft.h"
//---------------------------------------------------------------------------

//#define ASSERTCH(obj,idx) assert(idx>=0);assert(idx<obj.size())
//#define JASSERT(x) assert(x)

#define ASSERTCH(obj,idx)
#define JASSERT(x)

#define WTSIZE 19999
#define WTSIZE_3_4 3.0*WTSIZE/4.0
#define WTSIZE_1_4 1.0*WTSIZE/4.0

typedef std::complex<double> cpx_type;

class TrigLookUp
{
    public:
    TrigLookUp();
    std::vector<double> SinWT;
    std::vector<double> CosWT;
    std::vector<double> CosINV;
    std::vector<cpx_type> CISWT;

};

extern TrigLookUp tringlookup;

class WaveTable
{
public:
        WaveTable(int freq, int samplerate);
        ~WaveTable();
        void   WTnextFrame();
        cpx_type  WTCISValue();
        cpx_type  WTCISValue_conj();
        double   WTSinValue();
        double   WTSinValue(double PlusFractOfSample);
        double  WTCosValue();
        double   WTCosValue(double PlusFractOfSample);
        void  WTsetFreq(int freq,int samplerate);
        void  SetFreq(double freq, int samplerate);
        double GetFreqTest();
        void  Advance(double FractionOfSample){WTptr+=FractionOfSample*WTstep;if(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;if(((int)WTptr)<0) WTptr+=WTSIZE;}
        void  AdvanceFractionOfWave(double FractionOfWave){WTptr+=FractionOfWave*WTSIZE;while(WTptr>=WTSIZE) WTptr-=WTSIZE;while(WTptr<0) WTptr+=WTSIZE;}
        void  Retard(double FractionOfSample){WTptr-=FractionOfSample*WTstep;if(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;if(((int)WTptr)<0) WTptr+=WTSIZE;}
        bool   IfPassesPointNextTime_frames(double NumberOfFrames);
        bool   IfPassesPointNextTime(double FractionOfWave);
        bool   IfPassesPointNextTime();
        bool   IfHavePassedPoint(double FractionOfWave);
        double FractionOfSampleItPassesBy;
        void  SetWTptr(double FractionOfWave,double PlusNumberOfFrames);
        double WTptr;
        double  DistancebetweenWT(double WTptr1, double WTptr2);
        WaveTable();
        void  SetFreq(double freq);
        void  SetPhaseDeg(double phase_deg);
        void  SetPhaseCycles(double phase_cycles);
        double GetPhaseDeg();
        double GetFreqHz();
        void  IncresePhaseDeg(double phase_deg);
        void  IncreseFreqHz(double freq_hz);
private:
        double WTstep;
        double freq;
        double samplerate;
        int tint;
        double last_WTptr;

};

class FIR
{
public:
        FIR(int _NumberOfPoints);
        ~FIR();
        double  FIRUpdateAndProcess(double sig);
        void  FIRUpdate(double sig);
        double  FIRProcess(double FractionOfSampleOffset);
        double  FIRUpdateAndProcess(double sig, double FractionOfSampleOffset);
        void  FIRSetPoint(int point, double value);
        double *points;
        double *buff;
        int NumberOfPoints;
        int buffsize;
        int ptr;
        double outsum;
};

//--C-band


class QJFastFIRFilter : public QObject
{
    Q_OBJECT
public:
    QJFastFIRFilter(QObject *parent = 0);
    int setKernel(QVector<kffsamp_t> imp_responce,int nfft);
    int setKernel(QVector<kffsamp_t> imp_responce);
    void Update(kffsamp_t *data,int Size);
    void Update(QVector<kffsamp_t> &data);
    void reset();
    ~QJFastFIRFilter();
private:
    size_t nfft;
    kiss_fastfir_cfg cfg;
    size_t idx_inbuf;
    QVector<kffsamp_t> inbuf;
    QVector<kffsamp_t> outbuf;
    QVector<kffsamp_t> remainder;
    int remainder_ptr;
};

class QJHilbertFilter : public QJFastFIRFilter
{
    Q_OBJECT
public:
    QJHilbertFilter(QObject *parent = 0);
    void setSize(int N);
    QVector<kffsamp_t> getKernel();
private:
    QVector<kffsamp_t> kernel;
};

//--

class AGC
{
public:
    AGC(double _SecondsToAveOver,double _Fs);
    ~AGC();
    double Update(double sig);
    double AGCVal;
private:
    int AGCMASz;
    double AGCMASum;
    double *AGCMABuffer;
    int AGCMAPtr;
};

class MovingAverage
{
public:
    MovingAverage(int number);
    ~MovingAverage();
    double Update(double sig);
    double UpdateSigned(double sig);
    void Zero();
    double Val;
private:
    int MASz;
    double MASum;
    double *MABuffer;
    int MAPtr;
};

template <class T>
class TMovingAverage
{
public:
    TMovingAverage(int number)
    {
        MASz=round(number);
        JASSERT(MASz>0);
        MASum=0;
        MABuffer=new T[MASz];
        for(int i=0;i<MASz;i++)MABuffer[i]=0;
        MAPtr=0;
        Val=0;
    }
    TMovingAverage()
    {
        MASz=10;
        JASSERT(MASz>0);
        MASum=0;
        MABuffer=new T[MASz];
        for(int i=0;i<MASz;i++)MABuffer[i]=0;
        MAPtr=0;
        Val=0;
    }
    void setLength(int number)
    {
        if(MASz)delete [] MABuffer;
        MASz=round(number);
        JASSERT(MASz>0);
        MASum=0;
        MABuffer=new T[MASz];
        for(int i=0;i<MASz;i++)MABuffer[i]=0;
        MAPtr=0;
        Val=0;
    }
    ~TMovingAverage()
    {
        if(MASz)delete [] MABuffer;
    }
    T UpdateSigned(T sig)
    {
        MASum=MASum-MABuffer[MAPtr];
        MASum=MASum+(sig);
        MABuffer[MAPtr]=(sig);
        MAPtr++;MAPtr%=MASz;
        Val=MASum/((double)MASz);
        return Val;
    }
    T Val;
private:
    int MASz;
    T MASum;
    T *MABuffer;
    int MAPtr;
};

class MovingVar
{
public:
    MovingVar(int number);
    ~MovingVar();
    double Update(double sig);
    double Var;
    double Mean;
private:
    MovingAverage *E;
    MovingAverage *E2;
};

class MSEcalc
{
public:
    MSEcalc(int number);
    ~MSEcalc();
    double Update(cpx_type pt_qpsk);
    double mse;
    void Zero();
private:
    MovingAverage *pointmean;
    MovingAverage *msema;
};

//approx ebno measurement for msk signal with a mathed filter
class MSKEbNoMeasure
{
public:
    MSKEbNoMeasure(int number);
    ~MSKEbNoMeasure();
    double Update(double sig);//requires a matched filter first
    double EbNo;
    double Var;
    double Mean;
private:
    MovingAverage *E;
    MovingAverage *E2;
};

//approx ebno measurement for oqpsk signal with a mathed filter
class OQPSKEbNoMeasure
{
public:
    OQPSKEbNoMeasure(int number,double Fs,double fb);
    ~OQPSKEbNoMeasure();
    void setup_update(double Fs,double fb);
    double Update(double sig);//requires a matched filter first
    double EbNo;
    double Var;
    double Mean;
    double Fs;
    double fb;
private:
    MovingAverage *E;
    MovingAverage *E2;
};

//for MSK
class SymTracker
{
public:
    SymTracker()
    {
        Freq=0;
        Phase=0;
    }
    void Reset()
    {
        Freq=0;
        Phase=0;
    }
    double Freq;
    double Phase;
};

class DiffDecode
{
public:
    DiffDecode();
    bool Update(bool state);
    double UpdateSoft(double soft);
private:
    bool laststate;
    double lastsoftstate;
};


//---------------------------------------

class BaceConverter
{
public:
        BaceConverter();
        bool  LoadSymbol(int Val);
        bool  GetNextSymbol();
        void  Reset();
        int Result;
        bool DataAvailable;
        void  SetInNumberOfBits(int NumberOfBits);
        void  SetOutNumberOfBits(int NumberOfBits);
        int GetInNumberOfBits(){return NumberOfBitsForInSize;}
        int GetOutNumberOfBits(){return NumberOfBitsForOutSize;}
        int BuffPosPtr;
private:

        int InMaxVal,OutMaxVal;
        int NumberOfBitsForInSize,NumberOfBitsForOutSize;
        int Buff;
        int ErasurePtr;


};

class RootRaisedCosine
{
public:
    void design(double alpha,int firsize,double samplerate,double symbol_freq)
    {
        if((firsize%2)==0)firsize+=1;
        Points.resize(firsize);
        double T=(samplerate)/(symbol_freq);
        double fi;
        for(int i=0;i<firsize;i++)
        {
            ASSERTCH(Points,i);
            if(i==((firsize-1)/2)) Points[i]=(4.0*alpha+M_PI-M_PI*alpha)/(M_PI*sqrt(T));
             else
             {
                fi=(((double)i)-((double)(firsize-1))/2.0);
                if(fabs(1.0-pow(4.0*alpha*fi/T,2))<0.0000000001)Points[i]=(alpha*((M_PI-2.0)*cos(M_PI/(4.0*alpha))+(M_PI+2.0)*sin(M_PI/(4.0*alpha)))/(M_PI*sqrt(2.0*T)));
                else Points[i]=(4.0*alpha/(M_PI*sqrt(T))*(cos((1.0+alpha)*M_PI*fi/T)+T/(4.0*alpha*fi)*sin((1.0-alpha)*M_PI*fi/T))/(1.0-pow(4.0*alpha*fi/T,2)));
             }
        }
    }
    QVector<double> Points;
};


template <class T>
class Delay
{
public:
    Delay()
    {
        setdelay(1);
    }
    void setdelay(double fractdelay_)
    {
        fractdelay=fractdelay_;
        int buffsize=std::ceil(fractdelay)+1;
        buff.resize(buffsize);
        buff.fill(0);
        buffptr=0;
    }
    T update(T sig)
    {
        ASSERTCH(buff,buffptr);
        buff[buffptr]=sig;
        double dptr=((double)buffptr)-fractdelay;
        buffptr++;buffptr%=buff.size();

        while(std::floor(dptr)<0)dptr+=((double)buff.size());
        int iptr=std::floor(dptr);

        double weighting=dptr-((double)iptr);
        ASSERTCH(buff,iptr);
        T older=buff.at(iptr);
        iptr++;iptr%=buff.size();
        T newer=buff.at(iptr);

        return (weighting*newer+(1.0-weighting)*older);
    }
private:
    QVector<T> buff;
    int buffptr;
    double fractdelay;
};

class IIR
{
public:
    IIR();
    double  update(double sig);
    QVector<double> a;
    QVector<double> b;
    void init();
    double y;
private:
    QVector<double> buff_y;
    QVector<double> buff_x;
    int buff_y_ptr;
    int buff_x_ptr;
};

template <class T>
class Intergrator
{
public:
    Intergrator()
    {
        setlength(1);
    }
    void setlength(int length)
    {
        MASz=length;
        MABuffer.resize(MASz);
        for(int i=0;i<MABuffer.size();i++)MABuffer[i]=(T)0;
        MAPtr=0;
        Val=0;
    }
    void clear()
    {
        for(int i=0;i<MABuffer.size();i++)MABuffer[i]=(T)0;
        MAPtr=0;
        Val=0;
    }
    T Update(T sig)
    {
        ASSERTCH(MABuffer,MAPtr);
        MASum=MASum-MABuffer[MAPtr];
        MASum=MASum+sig;
        MABuffer[MAPtr]=sig;
        MAPtr++;MAPtr%=MASz;
        Val=MASum/((double)MASz);
        return Val;
    }
    T Val;
private:
    int MASz;
    T MASum;
    QVector<T> MABuffer;
    int MAPtr;
};

//-------------

template <class T>
class DelayThing
{
public:
    DelayThing()
    {
        setLength(12);
    }
    void setLength(int length)
    {
        length++;
        assert(length>0);
        buffer.resize(length);
        buffer_ptr=0;
        buffer_sz=buffer.size();
    }
    void update(T &data)
    {
        buffer[buffer_ptr]=data;
        buffer_ptr++;buffer_ptr%=buffer_sz;
        data=buffer[buffer_ptr];
    }
    T update_dont_touch(T data)
    {
        buffer[buffer_ptr]=data;
        buffer_ptr++;buffer_ptr%=buffer_sz;
        return buffer[buffer_ptr];
    }
    int findmaxpos(T &maxval)
    {
        int maxpos=0;
        maxval=buffer[buffer_ptr];
        for(int i=0;i<buffer_sz;i++)
        {
            if(buffer[buffer_ptr]>maxval)
            {
               maxval=buffer[buffer_ptr];
               maxpos=i;
            }
            buffer_ptr++;buffer_ptr%=buffer_sz;
        }
        return maxpos;
    }
private:
    QVector<T> buffer;
    int buffer_ptr;
    int buffer_sz;
};

//------
#include <QDebug>

class PeakDetector
{
public:
   PeakDetector()
   {
       setSettings((int)(9.14*128.0/2.0),0.25);//not sure what the best length should be

   }
   ~PeakDetector()
   {
   }
   void setSettings(int length,double _threshold)
   {

       d1.setLength(length*2);
       d2.setLength(length);
       lastdy=0;
       maxcntdown=2*length;
       cntdown=maxcntdown;
       threshold=_threshold;
       maxposcntdown=-1;
       d3.setLength(2*length);
   }
   void setSettings(int length,double _threshold,int _maxcntdown)
   {

       d1.setLength(length*2);
       d2.setLength(length);
       lastdy=0;
       maxcntdown=_maxcntdown;
       cntdown=maxcntdown;
       threshold=_threshold;
       maxposcntdown=-1;
       d3.setLength(2*length);
   }
   bool update(double &val)
   {
       double val2=d3.update_dont_touch(val);//actual value to return;

       double dy=val-d1.update_dont_touch(val);//diff over 2*length
       d2.update(val);//val we work with due to delay caused by dy
       if((!cntdown)&&(val>threshold)&&((lastdy>=0&&dy<0)))//clear, going up and above threshold
       {

           //stop calling again to fast
           cntdown=maxcntdown;

           //abs maximum in return vals
           maxval=0;
           maxpos=d3.findmaxpos(maxval);
           maxposcntdown=maxpos;

       }
       if(cntdown>0)cntdown--;
       lastdy=dy;


       val=val2;//set return val

      if(!maxposcntdown)//if time to return peak then return so
       {
           maxposcntdown--;



           return true;
       }
       if(maxposcntdown>0)maxposcntdown--;

       return false;

   }

   DelayThing<double> d1;
   DelayThing<double> d2;
   DelayThing<double> d3;
   double lastdy;
   int cntdown;
   int maxcntdown;
   double threshold;

   double maxval;
   int maxpos;
   int maxposcntdown;

};


#endif
