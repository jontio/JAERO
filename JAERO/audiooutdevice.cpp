#include "audiooutdevice.h"
#include <QDebug>
#include <math.h>

AudioOutDevice::AudioOutDevice(QObject *parent)
    :   QIODevice(parent),
      m_audioOutput(NULL)
{
    setSettings(settings);
    clear();
}

void AudioOutDevice::clear()
{
    circ_buffer.resize(16000);
    circ_buffer_head=0;
    circ_buffer_tail=circ_buffer.size()/2;
    for(int i=0;i<circ_buffer.size();i++)
    {
        double dval=0.1*sin(2.0*M_PI*500*((double)i)/(((double)(settings.Fs))));
        dval=0;
        circ_buffer[i]=floor(0.75*dval*32767.0+0.5);
    }
}

void AudioOutDevice::start()
{
    clear();
    open(QIODevice::ReadOnly);
    if(m_audioOutput)m_audioOutput->start(this);
}

void AudioOutDevice::stop()
{
    if(m_audioOutput)m_audioOutput->stop();
    close();
}

void AudioOutDevice::setSettings(Settings _settings)
{

    bool wasopen=isOpen();
    stop();

    //if Fs has changed or the audio device doesnt exist or the input device has changed then need to redo the audio device
    if((_settings.Fs!=settings.Fs)||(!m_audioOutput)||(_settings.audio_device_out!=settings.audio_device_out))
    {
        settings=_settings;

        if(m_audioOutput)m_audioOutput->deleteLater();

        //set the format
        m_format.setSampleRate(settings.Fs);
        m_format.setChannelCount(1);
        m_format.setSampleSize(16);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::SignedInt);

        //setup
        m_audioOutput = new QAudioOutput(settings.audio_device_out, m_format, this);
        m_audioOutput->setBufferSize(settings.Fs*settings.buffersizeinsecs);//buffersizeinsecs seconds of buffer
    }
    settings=_settings;

    clear();

    if(wasopen)start();
}

AudioOutDevice::~AudioOutDevice()
{
    stop();
}

qint64 AudioOutDevice::readData(char *data, qint64 maxlen)
{

    qint16 *ptr = reinterpret_cast<qint16 *>(data);
    int numofsamples=(maxlen/sizeof(qint16));

    for(int i=0;i<numofsamples;i++)
    {

        //calc where head is wrt to us the tail
        int forward=abs(circ_buffer_tail-circ_buffer_head);
        int backwards=circ_buffer.size()-forward;
        int dis=std::min(forward,backwards);
        if(circ_buffer_tail>circ_buffer_head)
        {
            int tmp=backwards;
            backwards=forward;
            forward=tmp;
        }

        //failed to avoid the head so move as far away as posible
        if(circ_buffer_head==circ_buffer_tail)
        {
            circ_buffer_tail=circ_buffer_head+circ_buffer.size()/2;
            qDebug()<<"AudioOutDevice: underflow";
        }

        //dithering
        if(dis>1000)
        {
            circ_buffer_tail++;circ_buffer_tail%=circ_buffer.size();
        }
         else
         {
            *ptr=0;
            if(backwards<forward)//we are going to slow, skip a frame
            {
                circ_buffer_tail+=2;circ_buffer_tail%=circ_buffer.size();
            }
            if(backwards>forward) //we are going to fast, pad a frame
            {

            }
         }
        *ptr=circ_buffer[circ_buffer_tail];

        ptr++;

    }


    return maxlen;
}

void AudioOutDevice::audioin(const QByteArray &signed16array)
{
    const qint16 *ptr = reinterpret_cast<const qint16 *>(signed16array.data());
    int numofsamples=(signed16array.size()/sizeof(qint16));
    for(int i=0;i<numofsamples;i++)
    {

        circ_buffer_head++;circ_buffer_head%=circ_buffer.size();
        circ_buffer[circ_buffer_head]=*ptr;

        ptr++;
    }


}

qint64 AudioOutDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

