//    Copyright (C) 2015  Jonti Olds


#ifndef DSPH
#define DSPH
//---------------------------------------------------------------------------
#include <math.h>
#include <vector>
#include <complex>
//---------------------------------------------------------------------------

#define WTSIZE 19999

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
        double   WTSinValue();
        double   WTSinValue(double PlusFractOfSample);
        double  WTCosValue();
        double   WTCosValue(double PlusFractOfSample);
        void  WTsetFreq(int freq,int samplerate);
        void  SetFreq(double freq, int samplerate);
        double GetFreqTest();
        void  Advance(double FractionOfSample){WTptr+=FractionOfSample*WTstep;if(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;if(((int)WTptr)<0) WTptr+=WTSIZE;}
        void  AdvanceFractionOfWave(double FractionOfWave){WTptr+=FractionOfWave*WTSIZE;if(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;if(((int)WTptr)<0) WTptr+=WTSIZE;}
        void  Retard(double FractionOfSample){WTptr-=FractionOfSample*WTstep;if(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;if(((int)WTptr)<0) WTptr+=WTSIZE;}
        bool   IfPassesPointNextTime_frames(double NumberOfFrames);
        bool   IfPassesPointNextTime(double FractionOfWave);
        bool   IfPassesPointNextTime();
        double FractionOfSampleItPassesBy;
        void  SetWTptr(double FractionOfWave,double PlusNumberOfFrames);
        double WTptr;
        double  DistancebetweenWT(double WTptr1, double WTptr2);
        WaveTable();
        void  SetFreq(double freq);
        void  SetPhaseDeg(double phase_deg);
        double GetPhaseDeg();
        double GetFreqHz();
        void  IncresePhaseDeg(double phase_deg);
        void  IncreseFreqHz(double freq_hz);
private:
        double WTstep;
        double freq;
        double samplerate;
        int tint;

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
    double Val;
private:
    int MASz;
    double MASum;
    double *MABuffer;
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
private:
    bool laststate;
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


#endif
