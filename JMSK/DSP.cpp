//---------------------------------------------------------------------------


#include "DSP.h"
#include <assert.h>
#include <QDebug>

//---------------------------------------------------------------------------

TrigLookUp tringlookup;

TrigLookUp::TrigLookUp()
{
    //--load wavetables (trig)
    SinWT.resize(WTSIZE);
    CosWT.resize(WTSIZE);
    CosINV.resize(WTSIZE);
    CISWT.resize(WTSIZE);
    int cosine,i;
    for(i=0;i<WTSIZE;i++){SinWT[i]=(sin(2*M_PI*((double)i)/WTSIZE));}
    for(i=0;i<WTSIZE;i++){CosWT[i]=(sin(M_PI_2+2*M_PI*((double)i)/WTSIZE));}
    for(i=0;i<WTSIZE;i++)
    {
        CISWT[i]=cpx_type(CosWT[i],SinWT[i]);
        cosine=(2.0*((double)i)/(WTSIZE-1.0))-1.0;
        if (cosine>1)cosine=1;
        else if(cosine<-1)cosine=-1;
        CosINV[i]=acos(cosine);
    }
    //--
}

WaveTable::WaveTable()
{
    samplerate=48000;
    freq=1000;
    WTstep=(1000.0)*WTSIZE/(48000);//default
    WTptr=0;
    FractionOfSampleItPassesBy=0.0;
}

WaveTable::WaveTable(int _freq,int _samplerate)
{
    freq=_freq;
    samplerate=_samplerate;
    if(freq<0)freq=0;
    WTstep=((double)freq)*WTSIZE/((float)samplerate);
    WTptr=0;
    FractionOfSampleItPassesBy=0.0;
}

WaveTable::~WaveTable()
{

}

double  WaveTable::DistancebetweenWT(double WTptr1, double WTptr2)
{
    double dif1=WTptr1-WTptr2;
    double dif2=WTptr2-WTptr1;
    if(dif1<0)dif1+=WTSIZE;
    if(dif2<0)dif2+=WTSIZE;
    double dif=dif1;
    if(dif2<dif1)dif=-dif2;
    dif=dif/WTSIZE;
    return dif;
}

void  WaveTable::WTnextFrame()
{
    assert(WTptr>=0.0);
    if(WTstep<0)WTstep=0;
    WTptr+=WTstep;
    while(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;
}

cpx_type  WaveTable::WTCISValue()
{
    tint=(int)WTptr;
    if(tint>=WTSIZE)tint=0;
    if(tint<0)tint=WTSIZE-1;
    return tringlookup.CISWT[tint];
}

double   WaveTable::WTSinValue()
{
    tint=(int)WTptr;
    if(tint>=WTSIZE)tint=0;
    if(tint<0)tint=WTSIZE-1;
    return tringlookup.SinWT[tint];
}

double   WaveTable::WTSinValue(double PlusFractOfSample)
{
    double ts=WTptr;
    ts+=PlusFractOfSample*WTstep;
    if(((int)ts)>=WTSIZE) ts-=WTSIZE;
    tint=(int)ts;
    if(tint>=WTSIZE)tint=0;
    if(tint<0)tint=WTSIZE-1;
    return tringlookup.SinWT[tint];
}

double  WaveTable::WTCosValue()
{
    tint=(int)WTptr;
    if(tint>=WTSIZE)tint=0;
    if(tint<0)tint=WTSIZE-1;
    return tringlookup.CosWT[tint];
}

double   WaveTable::WTCosValue(double PlusFractOfSample)
{
    double ts=WTptr;
    ts+=PlusFractOfSample*WTstep;
    if(((int)ts)>=WTSIZE) ts-=WTSIZE;
    tint=(int)ts;
    if(tint>=WTSIZE)tint=0;
    if(tint<0)tint=WTSIZE-1;
    return tringlookup.CosWT[tint];
}

void  WaveTable::WTsetFreq(int _freq,int _samplerate)
{
    samplerate=_samplerate;
    freq=_freq;
    if(freq<0)freq=0;
    WTstep=((double)freq)*WTSIZE/((float)samplerate);
    while(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;
}

void  WaveTable::SetFreq(double _freq,int _samplerate)
{
    freq=_freq;
    samplerate=_samplerate;
    if(freq<0)freq=0;
    WTstep=(freq)*((double)WTSIZE)/((float)samplerate);
    while(((int)WTptr)>=WTSIZE) WTptr-=WTSIZE;
}

void  WaveTable::SetFreq(double _freq)
{
    freq=_freq;
    if(freq<0)freq=0;
    WTstep=(freq)*((double)WTSIZE)/samplerate;
}

double WaveTable::GetFreqHz()
{
    return freq;
}

void  WaveTable::IncreseFreqHz(double freq_hz)
{
    freq_hz+=freq;
    SetFreq(freq_hz);
}

void  WaveTable::IncresePhaseDeg(double phase_deg)
{
    phase_deg+=(360.0*WTptr/((double)WTSIZE));
    SetPhaseDeg(phase_deg);
}

void  WaveTable::SetPhaseDeg(double phase_deg)
{
    phase_deg=std::fmod(phase_deg,360.0);
    if(phase_deg<0)phase_deg+=360.0;
    WTptr=(phase_deg/360.0)*((double)WTSIZE);
}

double WaveTable::GetPhaseDeg()
{
    return (360.0*WTptr/((double)WTSIZE));
}

double WaveTable::GetFreqTest()
{
    return WTstep;
}

bool  WaveTable::IfPassesPointNextTime()
{
    FractionOfSampleItPassesBy=WTptr+(WTstep-WTSIZE);
    if(FractionOfSampleItPassesBy<0)return false;
    FractionOfSampleItPassesBy/=WTstep;
    return true;
}

bool  WaveTable::IfPassesPointNextTime(double FractionOfWave)
{
    FractionOfSampleItPassesBy=(FractionOfWave*WTSIZE)-WTptr;
    if(FractionOfSampleItPassesBy<0)FractionOfSampleItPassesBy+=WTSIZE;
    if(FractionOfSampleItPassesBy<WTstep)
    {
        FractionOfSampleItPassesBy=(WTstep-FractionOfSampleItPassesBy)/WTstep;
        return true;
    }
    return false;
}

bool  WaveTable::IfPassesPointNextTime_frames(double NumberOfFrames)
{
    FractionOfSampleItPassesBy=NumberOfFrames-WTptr;
    if(FractionOfSampleItPassesBy<0)FractionOfSampleItPassesBy+=WTSIZE;
    if(FractionOfSampleItPassesBy<WTstep)
    {
        FractionOfSampleItPassesBy=(WTstep-FractionOfSampleItPassesBy)/WTstep;
        return true;
    }
    return false;
}

void  WaveTable::SetWTptr(double FractionOfWave,double PlusNumberOfFrames)
{
    while(FractionOfWave>=1)FractionOfWave-=1;
    while(FractionOfWave<0)FractionOfWave+=1;
    WTptr=(FractionOfWave*WTSIZE);
    WTptr+=PlusNumberOfFrames*WTstep;
    while(((int)WTptr)>=WTSIZE)WTptr-=WTSIZE;
    while(((int)WTptr)<0)WTptr+=WTSIZE;
}

//---------------------------------------------------------------------------



FIR::FIR(int _NumberOfPoints)
{
        int i;
        points=0;buff=0;

        NumberOfPoints=_NumberOfPoints;
        buffsize=NumberOfPoints+1;
        points=new double[NumberOfPoints];
        for(i=0;i<NumberOfPoints;i++)points[i]=0;
        buff=new double[buffsize];
        for(i=0;i<buffsize;i++)buff[i]=0;
        ptr=0;
        outsum=0;
}

FIR::~FIR()
{
        if(points)delete [] points;
        if(buff)delete [] buff;
}

double  FIR::FIRUpdateAndProcess(double sig)
{
        buff[ptr]=sig;
        ptr++;ptr%=buffsize;
        int tptr=ptr;
        outsum=0;
        for(int i=0;i<NumberOfPoints;i++)
        {
                outsum+=points[i]*buff[tptr];
                tptr++;tptr%=buffsize;
        }
        return outsum;
}

void  FIR::FIRUpdate(double sig)
{
        buff[ptr]=sig;
        ptr++;ptr%=buffsize;
}

double  FIR::FIRProcess(double FractionOfSampleOffset)
{
        double nextp=FractionOfSampleOffset;
        double thisp=1-nextp;
        int tptr=ptr;
        int nptr=ptr;
        outsum=0;
        nptr++;nptr%=buffsize;
        for(int i=0;i<NumberOfPoints;i++)
        {
                outsum+=points[i]*(buff[tptr]*thisp+buff[nptr]*nextp);
                tptr=nptr;
                nptr++;nptr%=buffsize;
        }
        return outsum;
}

double  FIR::FIRUpdateAndProcess(double sig, double FractionOfSampleOffset)
{
        buff[ptr]=sig;
        ptr++;ptr%=buffsize;
        double nextp=FractionOfSampleOffset;
        double thisp=1-nextp;
        int tptr=ptr;
        int nptr=ptr;
        outsum=0;
        nptr++;nptr%=buffsize;
        for(int i=0;i<NumberOfPoints;i++)
        {
                outsum+=points[i]*(buff[tptr]*thisp+buff[nptr]*nextp);
                tptr=nptr;
                nptr++;nptr%=buffsize;
        }
        return outsum;
}

void  FIR::FIRSetPoint(int point, double value)
{
    assert(point>=0);
    assert(point<NumberOfPoints);
    if((point<0)||(point>=NumberOfPoints))return;
    points[point]=value;
}

//-----------------
AGC::AGC(double _SecondsToAveOver,double _Fs)
{
    AGCMASz=round(_SecondsToAveOver*_Fs);
    assert(AGCMASz>0);
    AGCMASum=0;
    AGCMABuffer=new double[AGCMASz];
    for(int i=0;i<AGCMASz;i++)AGCMABuffer[i]=0;
    AGCMAPtr=0;
    AGCVal=0;
}

double AGC::Update(double sig)
{
    AGCMASum=AGCMASum-AGCMABuffer[AGCMAPtr];
    AGCMASum=AGCMASum+fabs(sig);
    AGCMABuffer[AGCMAPtr]=fabs(sig);
    AGCMAPtr++;AGCMAPtr%=AGCMASz;
    AGCVal=1.414213562/fmax(AGCMASum/((double)AGCMASz),0.000001);
    AGCVal=fmax(AGCVal,0.000001);
    return AGCVal;
}

AGC::~AGC()
{
    if(AGCMASz)delete [] AGCMABuffer;
}

//---------------------

MovingAverage::MovingAverage(int number)
{
    MASz=round(number);
    assert(MASz>0);
    MASum=0;
    MABuffer=new double[MASz];
    for(int i=0;i<MASz;i++)MABuffer[i]=0;
    MAPtr=0;
    Val=0;
}

double MovingAverage::Update(double sig)
{
    MASum=MASum-MABuffer[MAPtr];
    MASum=MASum+fabs(sig);
    MABuffer[MAPtr]=fabs(sig);
    MAPtr++;MAPtr%=MASz;
    Val=MASum/((double)MASz);
    return Val;
}

MovingAverage::~MovingAverage()
{
    if(MASz)delete [] MABuffer;
}
//---------------------


MovingVar::MovingVar(int number)
{
    E = new MovingAverage(number);
    E2 = new MovingAverage(number);
}

double MovingVar::Update(double sig)
{
    E2->Update(sig*sig);
    Mean=E->Update(sig);
    Var=(E2->Val)-(E->Val*E->Val);
    return Var;
}

MovingVar::~MovingVar()
{
    delete E;
    delete E2;
}
//---------------------

MSKEbNoMeasure::MSKEbNoMeasure(int number)
{
    E = new MovingAverage(number);
    E2 = new MovingAverage(number);
}

double MSKEbNoMeasure::Update(double sig)
{
    E2->Update(sig*sig);
    Mean=E->Update(sig);
    Var=(E2->Val)-(E->Val*E->Val);
    double alpha=sqrt(2)/Mean;
    //double tebno=10.0*(log10(2.0)-log10(((Var*alpha*alpha)- 0.0085 ))+log10(1.5));//0.0085 for the non constant modulus after the matched filter
    double tebno=10.0*(log10(2.0)-log10(((Var*alpha*alpha)- 0.0085 )))-5.0;// this one matches matlab better
    if(std::isnan(tebno))tebno=50;
    if(tebno>50.0)tebno=50;
    EbNo=EbNo*0.8+0.2*tebno;
    return EbNo;
}

MSKEbNoMeasure::~MSKEbNoMeasure()
{
    delete E;
    delete E2;
}


//----------------------


DiffDecode::DiffDecode()
{
    laststate=false;
}

bool DiffDecode::Update(bool state)
{
    bool res=(state+laststate)%2;
    laststate=state;
    return res;
}

//-----------------------

bool  BaceConverter::LoadSymbol(int Value)
{
        if(Value<0)
        {
                Value=0;
                ErasurePtr=BuffPosPtr+NumberOfBitsForInSize;
        }

        Buff|=(Value<<BuffPosPtr);
        BuffPosPtr+=NumberOfBitsForInSize;
        if(BuffPosPtr>=NumberOfBitsForOutSize)DataAvailable=true;
         else DataAvailable=false;
        return DataAvailable;
}

bool  BaceConverter::GetNextSymbol()
{
         if(BuffPosPtr>=NumberOfBitsForOutSize)
         {
                if(ErasurePtr>0)
                {
                        Result=-1;
                        ErasurePtr-=NumberOfBitsForOutSize;
                }
                 else Result=Buff&OutMaxVal;
                Buff=(Buff>>NumberOfBitsForOutSize);
                BuffPosPtr-=NumberOfBitsForOutSize;
         }
         if(BuffPosPtr>=NumberOfBitsForOutSize)DataAvailable=true;
          else DataAvailable=false;
         return DataAvailable;
}

 BaceConverter::BaceConverter()
{
        SetInNumberOfBits(4);
        SetOutNumberOfBits(8);
}


void  BaceConverter::SetInNumberOfBits(int NumberOfBits)
{
        InMaxVal=((int)pow(2,NumberOfBits))-1;
        NumberOfBitsForInSize=NumberOfBits;
        Reset();
}

void  BaceConverter::SetOutNumberOfBits(int NumberOfBits)
{
        OutMaxVal=((int)pow(2,NumberOfBits))-1;
        NumberOfBitsForOutSize=NumberOfBits;
        Reset();
}


void  BaceConverter::Reset()
{
        Result=-1;
        DataAvailable=false;
        Buff=0;
        BuffPosPtr=0;
        Result=0;
        ErasurePtr=0;
}

