#include "compressedaudiodiskwriter.h"

#include <QDebug>

#include <QFileInfo>
#include <QDir>
#include <QDateTime>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

void CompressedAudioDiskWriter::setLogDir(QString dir)
{
    closeFile();
    logdir=dir;
    QDir adir;
    if(!dir.isEmpty())adir.mkpath(dir);
}

void CompressedAudioDiskWriter::timeoutslot()
{
    timeout->stop();
    closeFile();
    qDebug()<<"timeout --> closed";
}

CompressedAudioDiskWriter::CompressedAudioDiskWriter(QObject *parent) : QObject(parent)
{
    slowdown=0;

    os_valid=false;
    og_valid=false;
    op_valid=false;
    vi_valid=false;
    vc_valid=false;
    vd_valid=false;
    vb_valid=false;

    //setLogDir("e:/");

    timeout = new QTimer(this);
    connect(timeout, SIGNAL(timeout()), this, SLOT(timeoutslot()));
    timeout->start(1000);


//    //tone test
//    openFileForOutput("e:/delme.ogg");
//    double x=0;
//    for(int i=0;i<1000;i++)
//    {
//        QByteArray ba;
//        ba.resize(200);
//        qint16 *ptr = reinterpret_cast<qint16 *>(ba.data());
//        for(int i=0;i<100;i++)
//        {
//            *ptr=15000.0*sin(2.0*M_PI*x);
//            x+=500.0/44100.0;
//            ptr++;
//        }
//        audioin(ba);
//    }
//    closeFile();

}

CompressedAudioDiskWriter::~CompressedAudioDiskWriter()
{
    closeFile();
}

void CompressedAudioDiskWriter::openFileForOutput(QString filename)
{
    //close old file if already open
    closeFile();

    //qDebug()<<filename;

    //rename new file if exists
    QFileInfo fileinfo(filename);
    //qDebug()<<fileinfo.absoluteDir().absolutePath();
    QString path=fileinfo.absoluteDir().absolutePath();
    for(int i=1;QFileInfo::exists(filename);i++)
    {
        if(fileinfo.suffix().isEmpty())filename=path+fileinfo.completeBaseName()+"_"+QString::number(i);
         else filename=path+fileinfo.completeBaseName()+"_"+QString::number(i)+"."+fileinfo.suffix();
    }

    //qDebug()<<filename;
    //return;

    //open file
    outfile.setFileName(filename);
    outfile.open(QIODevice::WriteOnly);

    //I think this is all init stuff and writing a header to disk about the codec format

    vorbis_info_init(&vi);
    int ret=vorbis_encode_init_vbr(&vi,2,8000,0.5);//0.1);//1 is the highest quality
    if(ret)
    {
        closeFile();
        qDebug()<<"something went wrong. damed if i know.";
        return;
    }

    //add a comment
    vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc,"ENCODER","CompressedAudioDiskWriter");

    //set up the analysis state and auxiliary encoding storage. yes what the fuck is this??
    vorbis_analysis_init(&vd,&vi);
    vorbis_block_init(&vd,&vb);

    //set up our packet->stream encoder
    //pick a random serial number; that way we can more likely build chained streams just by concatenation
    //Again WTF???
    srand(time(NULL));
    ogg_stream_init(&os,rand());

    //Vorbis streams begin with three headers; the initial header (with
    //most of the codec setup parameters) which is mandated by the Ogg
    //bitstream spec.  The second header holds any comment fields.  The
    //third header holds the bitstream codebook.  We merely need to
    //make the headers, then pass them to libvorbis one at a time;
    //libvorbis handles the additional Ogg bitstream constraints
    //Oh come on speak english!!!
    {
      ogg_packet header;
      ogg_packet header_comm;
      ogg_packet header_code;

      vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
      ogg_stream_packetin(&os,&header);//automatically placed in its own page
      ogg_stream_packetin(&os,&header_comm);
      ogg_stream_packetin(&os,&header_code);

      //This ensures the actual audio data will start on a new page, as per spec
      //And how does it do that???
      while(true)
      {
        int result=ogg_stream_flush(&os,&og);
        if(result==0)break;

        //I think this part it writes the header. not sure what the body is and why are we in a loop???
        outfile.write((char*)og.header,og.header_len);
        outfile.write((char*)og.body,og.body_len);

      }

    }

    vi_valid=true;//not sure if this should be here or higher up.
    os_valid=true;
    og_valid=true;
    op_valid=true;
    vc_valid=true;
    vd_valid=true;
    vb_valid=true;

}

void CompressedAudioDiskWriter::closeFile()
{

    //i think this maybe a flush???
    if((os_valid)&&(vb_valid)&&(vd_valid)&&(vc_valid)&&(vi_valid)&&outfile.isOpen())
    {
        QByteArray ba;
        audioin(ba);
    }

    outfile.close();

    if(os_valid)ogg_stream_clear(&os);
    if(vb_valid)vorbis_block_clear(&vb);
    if(vd_valid)vorbis_dsp_clear(&vd);
    if(vc_valid)vorbis_comment_clear(&vc);
    if(vi_valid)vorbis_info_clear(&vi);
    os_valid=false;
    og_valid=false;
    op_valid=false;
    vi_valid=false;
    vc_valid=false;
    vd_valid=false;
    vb_valid=false;
}

void CompressedAudioDiskWriter::audioin(const QByteArray &signed16arraymono)
{

    timeout->start(1000);

    if(!outfile.isOpen())
    {
        if(!slowdown)
        {
            closeFile();
            qDebug()<<"File not open for writing.";
            if(!logdir.isEmpty())
            {
                QString filename=logdir+"/audiolog_"+QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")+".ogg";
                qDebug()<<"opening "<<filename;
                openFileForOutput(filename);
            }
        }
        slowdown+=(signed16arraymono.size()/2);if(slowdown>8000)slowdown=0;
        return;
    }


    if((!os_valid)||(!vb_valid)||(!vd_valid)||(!vc_valid)||(!vi_valid))
    {
        if(!slowdown)
        {
            closeFile();
            qDebug()<<"stream not open for writing";
        }
        slowdown+=(signed16arraymono.size()/2);if(slowdown>8000)slowdown=0;
        return;
    }

    slowdown=0;

    long bytes=signed16arraymono.size();
    if(bytes)
    {

        float **buffer=vorbis_analysis_buffer(&vd,bytes*2);

        //uninterleave samples
        int i;
        for(i=0;i<bytes/2;i++)
        {
            buffer[0][i]=((signed16arraymono[i*2+1]<<8)|(0x00ff&(int)signed16arraymono[i*2]))/32768.f;
            buffer[1][i]=buffer[0][i];//mono. not sure how to make this lib do mono
        }

        //tell the library how much we actually submitted (in bytes)
        vorbis_analysis_wrote(&vd,i);

    }
     else //not sure what a zero len makes vorbis/ogg do. maybe a flush???
     {
        vorbis_analysis_wrote(&vd,0);
     }

    //...more magic pixie dust...
    int eos=0;
    while(vorbis_analysis_blockout(&vd,&vb)==1)
    {

        //analysis, assume we want to use bitrate management
        vorbis_analysis(&vb,NULL);
        vorbis_bitrate_addblock(&vb);
        while(vorbis_bitrate_flushpacket(&vd,&op))
        {

            //weld the packet into the bitstream
            ogg_stream_packetin(&os,&op);

            //write out pages (if any)
            while(!eos)
            {
                int result=ogg_stream_pageout(&os,&og);
                if(result==0)break;
                outfile.write((char*)og.header,og.header_len);
                outfile.write((char*)og.body,og.body_len);

                //this could be set above, but for illustrative purposes, I do it here (to show that vorbis does know where the stream ends)

                if(ogg_page_eos(&og))eos=1;
            }
        }
    }
}
