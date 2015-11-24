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

    dl1.setLength(12);//delay for decode encode BER check
    dl2.setLength(576-6);//delay's data to next frame
    dl3.setLength(1);

    preambledetector.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011

    //TODO: set size automatically
    setBaudRate(baud600);

}

void AeroL::setBaudRate(BaudRate rate)
{
    switch(rate)
    {
    case baud600:
        leaver.setSize(6);//9 for 1200 baud, 6 for 600 baud
        block.resize(6*64);
        break;
    case baud1200:
        leaver.setSize(9);//9 for 1200 baud, 6 for 600 baud
        block.resize(9*64);
        break;
    default:
        break;
    }
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

    for(int i=0;i<bits.size();i++)
    {
        if(cntr<1000000000)cntr++;
        if(cntr<16)
        {

            if(cntr==0)
            {
                frameinfo=bits[i];

            }
             else
             {
                frameinfo<<=1;
                frameinfo|=bits[i];
             }
        }
        if(cntr==15)
        {
            //decodedbytes.push_back('\n');

            //decodedbytes.append(((QString)"%1\n").arg(frameinfo));

            //delay frame info by one frame needed as viterbi decoder and delaylime make 1 frame delay
            quint16 tval=frameinfo;
            frameinfo=lastframeinfo;
            lastframeinfo=tval;

            //if(framecounter1!=framecounter2)decodedbytes.push_back("Error: Frame Counter mismatch\n");
            // else decodedbytes.push_back((((QString)"Format ID = %1\nSuper Frame Marker = %2\nFrame Counter = %3\n").arg(formatid).arg(supfrmaker).arg(framecounter1)).toLatin1());

            formatid=(frameinfo>>12)&0x000F;
            supfrmaker=(frameinfo>>8)&0x000F;
            framecounter1=(frameinfo>>4)&0x000F;
            framecounter2=(frameinfo>>0)&0x000F;

            //decodedbytes.push_back((((QString)"Last info is Format ID = %1\nSuper Frame Marker = %2\nFrame Counter = %3\n").arg(formatid).arg(supfrmaker).arg(framecounter1)).toLatin1());
        }
        if(cntr>=16)
        {

            //fill block
            static int blockcnt=-1;
            if(cntr==16)blockcnt=-1;
            int idx=(cntr-16)%block.size();
            block[idx]=bits[i];
            if(idx==(block.size()-1))//block is now filled
            {

                blockcnt++;

                //deinterleaver
                QVector<int> deleaveredblock=leaver.deinterleave(block);

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
                //decodedbytes+=((QString)"unencoded BER estimate=%1%\n").arg(QString::number( 100.0*unencoded_BER_estimate, 'f', 1));

                //delay line for frame alignment
                dl2.update(deconvol);

                //scrambler
                scrambler.update(deconvol);

                //pack the bits into bytes
                int charptr=0;
                uchar ch=0;
                for(int h=0;h<deconvol.size();h++)
                {
                    ch|=deconvol[h]*128;
                    charptr++;charptr%=8;
                    if(charptr==0)
                    {
                        infofield+=ch;//actual data of information field in bytearray
                        //decodedbytes+=((QString)"0x%1 ").arg(((QString)"").sprintf("%02X", ch)) ;//for console
                        ch=0;
                    }
                     else ch>>=1;
                }
                //decodedbytes+='\n';


                if((cntr-16)==(1152-1))//frame is done when this is true
                {

                    //run through all bytes in info field for console
                    //for(int k=0;k<infofield.size();k++)decodedbytes+=((QString)"0x%1 ").arg(((QString)"").sprintf("%02X", (uchar)infofield[k])) ;//for console
                    //decodedbytes+='\n';

                    //run through all SUs and check CRCs
                    //12 bytes for all SUs in P signal i think? or do extended SUs exist in the P signal????
                    char *infofieldptr=infofield.data();
                    bool allgood=true;
                    if(framecounter1==framecounter2)decodedbytes+=((QString)"").sprintf("Frame %d\n", framecounter1);
                     else decodedbytes+="frame count error\n";
                    if(formatid!=1)decodedbytes+="format ID error\n";
                    for(int k=0;k<infofield.size()/12;k++)
                    {
                        quint16 crc_calc=crc16.calcusingbytes(&infofieldptr[k*12],10);
                        quint16 crc_rec=(((uchar)infofield[k*12+11])<<8)|((uchar)infofield[k*12+10]);

                        decodedbytes+=(k+'0');//SU number in frame
                        for(int j=0;j<10;j++)
                        {
                            decodedbytes+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)infofield[k*12+j]));
                            //if(((uchar)infofield[j])>30&&((uchar)infofield[k*12+j])<127)decodedbytes+=(char)infofield[k*12+j];
                            // else decodedbytes+='.';
                        }
                        decodedbytes+=((QString)"").sprintf(" rec = %04X calc = %04X", crc_rec,crc_calc);
                        if(crc_calc==crc_rec)
                        {
                            MessageType message=(MessageType)((uchar)infofield[k*12]);
                            decodedbytes+=" OK ";
                            switch(message)
                            {
                            case Reserved_0:
                                decodedbytes+="Reserved_0";
                                break;
                            case Fill_in_signal_unit:
                                decodedbytes+="Fill_in_signal_unit";
                                break;
                            case AES_system_table_broadcast_GES_Psmc_and_Rsmc_channels_COMPLETE:
                                decodedbytes+="AES_system_table_broadcast_GES_Psmc_and_Rsmc_channels_COMPLETE";
                                break;
                            case AES_system_table_broadcast_GES_beam_support_COMPLETE:
                                decodedbytes+="AES_system_table_broadcast_GES_beam_support_COMPLETE";
                                break;
                            case AES_system_table_broadcast_index:
                                decodedbytes+="AES_system_table_broadcast_index";
                                break;
                            case AES_system_table_broadcast_satellite_identification_COMPLETE:
                                decodedbytes+="AES_system_table_broadcast_satellite_identification_COMPLETE";
                                break;

                            //SYSTEM LOG-ON/LOG-OFF
                            case Log_on_request:
                                decodedbytes+="Log_on_request";
                                break;
                            case Log_on_confirm:
                                decodedbytes+="Log_on_confirm";
                                break;
                            case Log_control_P_channel_log_off_request:
                                decodedbytes+="Log_control_P_channel_log_off_request";
                                break;
                            case Log_control_P_channel_log_on_reject:
                                decodedbytes+="Log_control_P_channel_log_on_reject";
                                break;
                            case Log_control_P_channel_log_on_interrogation:
                                decodedbytes+="Log_control_P_channel_log_on_interrogation";
                                break;
                            case Log_on_log_off_acknowledge_P_channel:
                                decodedbytes+="Log_on_log_off_acknowledge_P_channel";
                                break;
                            case Log_control_P_channel_log_on_prompt:
                                decodedbytes+="Log_control_P_channel_log_on_prompt";
                                break;
                            case Log_control_P_channel_data_channel_reassignment:
                                decodedbytes+="Log_control_P_channel_data_channel_reassignment";
                                break;

                            case Reserved_18:
                                decodedbytes+="Reserved_18";
                                break;
                            case Reserved_19:
                                decodedbytes+="Reserved_19";
                                break;
                            case Reserved_26:
                                decodedbytes+="Reserved_26";
                                break;

                            //CALL INITIATION
                            case Data_EIRP_table_broadcast_complete_sequence:
                                decodedbytes+="Data_EIRP_table_broadcast_complete_sequence";
                                break;

                            case T_channel_assignment:
                                decodedbytes+="T_channel_assignment";
                                break;

                            //CHANNEL INFORMATION
                            case P_R_channel_control_ISU:
                                decodedbytes+="P_R_channel_control_ISU";
                                break;
                            case T_channel_control_ISU:
                                decodedbytes+="T_channel_control_ISU";
                                break;

                            //ACKNOWLEDGEMENT
                            case Request_for_acknowledgement_RQA_P_channel:
                                decodedbytes+="Request_for_acknowledgement_RQA_P_channel";
                                break;
                            case Acknowledge_RACK_TACK_P_channel:
                                decodedbytes+="Acknowledge_RACK_TACK_P_channel";
                                break;


                            case User_data_ISU_RLS_P_T_channel:
                                decodedbytes+="User_data_ISU_RLS_P_T_channel";
                                break;
                            case User_data_3_octet_LSDU_RLS_P_channel:
                                decodedbytes+="User_data_3_octet_LSDU_RLS_P_channel";
                                break;
                            case User_data_4_octet_LSDU_RLS_P_channel:
                                decodedbytes+="User_data_4_octet_LSDU_RLS_P_channel";
                                break;
                            default:
                                if((message&0xC0)==0xC0)
                                {
                                    decodedbytes+="SUBSEQUENT SIGNAL UNITS";
                                }
                                break;
                            }
                            decodedbytes+='\n';
                        }
                         else
                         {
                            decodedbytes+=" Bad CRC\n";
                            allgood=false;
                         }

                        /*if(crc_calc==crc_rec)qDebug()<<k<<((QString)"").sprintf("rec = %02X", crc_rec)<<((QString)"").sprintf("calc = %02X", crc_calc)<<"OK"<<unencoded_BER_estimate*100.0;
                         else
                         {
                            allgood=false;
                            qDebug()<<k<<((QString)"").sprintf("rec = %02X", crc_rec)<<((QString)"").sprintf("calc = %02X", crc_calc)<<"Bad CRC"<<unencoded_BER_estimate*100.0;
                         }
                         */

                    }

                    if(!allgood)decodedbytes+="Got at least one bad SU (a SU failed CRC check)\n";//tell the console



                }

            }

            //raw
            //if((cntr-16)<1152)decodedbytes+=('0'+bits[i])+QChar(',');
        }

       // if(cntr+1==1200)cntr=-1;
       // if((framecounter1!=framecounter2||cntr>1300)&&preambledetector.Update(bits[i]))
        if(preambledetector.Update(bits[i]))
        {
            if(cntr+1!=1200)decodedbytes+="Error short frame!!! probably the soundcard droped some sound card buffers\n";
            decodedbytes+=((QString)"Bits for frame = %1\n").arg(cntr+1);
            cntr=-1;
            decodedbytes+="\nGot sync\n";
            scrambler.reset();
            infofield.clear();
        }

        if(cntr+1==1200)
        {
            scrambler.reset();
            cntr=-1;
            infofield.clear();
        }


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

