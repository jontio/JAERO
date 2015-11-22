#include "aerol.h"


AeroLInterleaver::AeroLInterleaver()
{
    M=64;

    interleaverowpermute.resize(M);
    interleaverowdepermute.resize(M);

    for(int i=0;i<M;i++)
    {
        interleaverowpermute[(i*27)%M]=i;
        interleaverowdepermute[i]=(i*27)%M;
//        interleaverowdepermute[(i*19)%M]=i;
    }
    setSize(6);
}
void AeroLInterleaver::setSize(int _N)
{
    if(_N<1)return;
    N=_N;
    matrix.resize(M*N);
}
QVector<int> &AeroLInterleaver::interleave(QVector<int> &block)
{
    if(block.size()!=(M*N))
    {
        matrix.fill(0);
        assert("AeroLInterleaver: block.size()!=(M*N)");
    }

    int k=0;
    for(int i=0;i<M;i++)
    {
        for(int j=0;j<N;j++)
        {
            int entry=interleaverowpermute[i]+M*j;
            assert(entry<block.size());
            assert(k<matrix.size());
            matrix[k]=block[entry];
            k++;
        }
    }

    return matrix;

}
QVector<int> &AeroLInterleaver::deinterleave(QVector<int> &block)
{
    if(block.size()!=(M*N))
    {
        matrix.fill(0);
        assert("AeroLInterleaver: block.size()!=(M*N)");
    }

    int k=0;
    for(int j=0;j<N;j++)
    {
        for(int i=0;i<M;i++)
        {
            int entry=interleaverowdepermute[i]*N+j;
            assert(entry<block.size());
            assert(k<matrix.size());
            matrix[k]=block[entry];
            k++;
        }
    }

    return matrix;
}

PreambleDetector::PreambleDetector()
{
    preamble.resize(1);
    buffer.resize(1);
    buffer_ptr=0;
}
void PreambleDetector::setPreamble(QVector<int> _preamble)
{
    preamble=_preamble;
    if(preamble.size()<1)preamble.resize(1);
    buffer.fill(0,preamble.size());
    buffer_ptr=0;
}
bool PreambleDetector::setPreamble(quint64 bitpreamble,int len)
{
    if(len<1||len>64)return false;
    preamble.clear();
    for(int i=len-1;i>=0;i--)
    {
        if((bitpreamble>>i)&1)preamble.push_back(1);
         else preamble.push_back(0);
    }
    if(preamble.size()<1)preamble.resize(1);
    buffer.fill(0,preamble.size());
    buffer_ptr=0;
    return true;
}
bool PreambleDetector::Update(int val)
{ 
    for(int i=0;i<(buffer.size()-1);i++)buffer[i]=buffer[i+1];
    buffer[buffer.size()-1]=val;
    if(buffer==preamble){buffer.fill(0);return true;}
    return false;
}

AeroL::AeroL(QObject *parent) : QIODevice(parent)
{
    sbits.reserve(1000);
    decodedbytes.reserve(1000);

    //ViterbiCodec is not Qt so needs deleting when finished
    std::vector<int> polynomials;
    polynomials.push_back(109);
    polynomials.push_back(79);
    convolcodec=new ViterbiCodec(7, polynomials);
    convolcodec->setPaddingLength(24);

    dl1.setLength(12);
    dl2.setLength(576-6);//?? correct ??? hope it delays data to next frame

    preambledetector.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011

    //TODO: set size automatically
    leaver.setSize(9);//9 for 1200 baud, 6 for 600 baud
    block.resize(9*64);

}


AeroL::~AeroL()
{
    delete convolcodec;
}

bool AeroL::Start()
{
    if(psinkdevice.isNull())return false;
    QIODevice *out=psinkdevice.data();
    out->open(QIODevice::WriteOnly);
    return true;
}

void AeroL::Stop()
{
    if(!psinkdevice.isNull())
    {
        QIODevice *out=psinkdevice.data();
        out->close();
    }
}

void AeroL::ConnectSinkDevice(QIODevice *device)
{
    if(!device)return;
    psinkdevice=device;
    Start();
}

void AeroL::DisconnectSinkDevice()
{
    Stop();
    if(!psinkdevice.isNull())psinkdevice.clear();
}

QByteArray &AeroL::Decode(QVector<short> &bits)//0 --> oldest
{
    decodedbytes.clear();

    //for(int i=(bits.size()-1);i>=0;i--)
    //for(int i=0;i<bits.size();i++)decodedbytes.push_back((bits[i])+48);
    //return decodedbytes;

    static int cntr=1000000000;
    static int formatid=0;
    static int supfrmaker=0;
    static int framecounter1=0;
    static int framecounter2=0;
    static int nibble=0;
    static int nibblecntr=0;

    for(int i=0;i<bits.size();i++)
    {
        if(cntr<1000000000)cntr++;
        if(cntr<16)
        {
            nibblecntr++;nibblecntr%=4;
            if(cntr==0)nibblecntr=0;
            if(nibblecntr==0)nibble=0;
            if(bits[i])nibble|=(1<<(3-nibblecntr));

            if(cntr==3)formatid=nibble;
            if(cntr==7)supfrmaker=nibble;
            if(cntr==11)framecounter1=nibble;
            if(cntr==15)framecounter2=nibble;

            decodedbytes.push_back((bits[i])+48);
        }
        if(cntr==15)
        {
            decodedbytes.push_back('\n');
            if(framecounter1!=framecounter2)decodedbytes.push_back("Error: Frame Counter mismatch\n");
             else decodedbytes.push_back((((QString)"Format ID = %1\nSuper Frame Marker = %2\nFrame Counter = %3\n").arg(formatid).arg(supfrmaker).arg(framecounter1)).toLatin1());
        }
        if(cntr>=16)
        {

            //deinterleave
            int idx=(cntr-16)%block.size();
            block[idx]=bits[i];
            if(idx==(block.size()-1))
            {

                //deinterleaver
                QVector<int> deleaveredblock=leaver.deinterleave(block);
                //for(int j=0;j<deleaveredblock.size();j++)decodedbytes+=('0'+deleaveredblock[j])+QChar(',');

                //viterbi decoder
                QVector<int> deconvol=convolcodec->Decode_Continuous(deleaveredblock);

                //
                //Checking deinterleaving and viterbi decoder are getting valid data
                //
                //convol encode
                QVector<int> convol=convolcodec->Encode_Continuous(deconvol);
                //delay line for unencoded BER estimate
                dl1.update(deleaveredblock);
                //count differences
                assert(deleaveredblock.size()==convol.size());
                int diffsum=0;
                for(int k=0;k<deleaveredblock.size();k++)
                {
                    if(deleaveredblock[k]!=convol[k])diffsum++;
                }
                float unencoded_BER_estimate=((float)diffsum)/((float)deleaveredblock.size());
                decodedbytes+=((QString)"unencoded BER estimate=%1%\n").arg(QString::number( 100.0*unencoded_BER_estimate, 'f', 1));

                //delay line for super frame alignment
                dl2.update(deconvol);

                //scrambler
                scrambler.reset();
                scrambler.update(deconvol);

           //     for(int j=0;j<deconvol.size();j++)decodedbytes+=('0'+deconvol[j])+QChar(',');



            }

            //raw
            //if((cntr-16)<1152)decodedbytes+=('0'+bits[i])+QChar(',');
        }

       // if(cntr+1==1200)cntr=-1;
       // if((framecounter1!=framecounter2||cntr>1300)&&preambledetector.Update(bits[i]))
        if(preambledetector.Update(bits[i]))
        {
            decodedbytes+=((QString)"Bits for superframe = %1\n").arg(cntr+1);cntr=-1;
            decodedbytes+="\nGot sync\n";
        }

        if(cntr+1==1200)cntr=-1;


    }

    return decodedbytes;
}

qint64 AeroL::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}


qint64 AeroL::writeData(const char *data, qint64 len)
{
    sbits.clear();
    uchar *udata=(uchar*)data;
    uchar auchar;
    for(int i=0;i<len;i++)
    {
        auchar=udata[i];
        for(int j=0;j<8;j++)
        {
            sbits.push_back(auchar&1);
            auchar=auchar>>1;
        }
    }
    Decode(sbits);

    if(!psinkdevice.isNull())
    {
        QIODevice *out=psinkdevice.data();
        if(out->isOpen())out->write(decodedbytes);
    }

    return len;
}

