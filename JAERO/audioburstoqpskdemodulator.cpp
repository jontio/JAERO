#include "audioburstoqpskdemodulator.h"
#include <QDebug>

AudioBurstOqpskDemodulator::AudioBurstOqpskDemodulator(QObject *parent)
    :   BurstOqpskDemodulator(parent),
      m_audioInput(NULL)
{
    demod2=new BurstOqpskDemodulator(this);
    demod2->channel_select_other=true;
    connect(this,SIGNAL(writeDataSignal(const char*,qint64)),demod2,SLOT(writeDataSlot(const char*,qint64)),Qt::DirectConnection);

}

void AudioBurstOqpskDemodulator::start()
{
    BurstOqpskDemodulator::start();
    demod2->start();
    if(!settings.zmqAudio)
    {
        if(m_audioInput)m_audioInput->start(this);
    }
}

void AudioBurstOqpskDemodulator::stop()
{
    if(m_audioInput)m_audioInput->stop();
    if(!settings.zmqAudio)
    {
        BurstOqpskDemodulator::stop();
    }
    demod2->stop();
}

void AudioBurstOqpskDemodulator::setSettings(Settings _settings)
{
    bool wasopen=isOpen();
    stop();

    //if Fs has changed or the audio device doesnt exist or the input device has changed then need to redo the audio device
    if((_settings.Fs!=settings.Fs)||(!m_audioInput)||(_settings.audio_device_in!=settings.audio_device_in)||(_settings.channel_stereo!=settings.channel_stereo))
    {
        settings=_settings;

        if(m_audioInput)m_audioInput->deleteLater();

        //set the format
        m_format.setSampleRate(settings.Fs);
        if(settings.channel_stereo)m_format.setChannelCount(2);
        else m_format.setChannelCount(1);
        m_format.setSampleSize(16);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::SignedInt);

        //setup
        m_audioInput = new QAudioInput(settings.audio_device_in, m_format, this);
        m_audioInput->setBufferSize(settings.Fs*settings.buffersizeinsecs);//buffersizeinsecs seconds of buffer
    }
    settings=_settings;

    BurstOqpskDemodulator::setSettings(settings);
    demod2->setSettings(settings);

    if(wasopen)start();

}

AudioBurstOqpskDemodulator::~AudioBurstOqpskDemodulator()
{
    stop();
}
