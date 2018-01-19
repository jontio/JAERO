#include "audioburstmskdemodulator.h"

#include <QDebug>

AudioBurstMskDemodulator::AudioBurstMskDemodulator(QObject *parent)
:   BurstMskDemodulator(parent),
  m_audioInput(NULL)
{
//
}

void AudioBurstMskDemodulator::start()
{
    BurstMskDemodulator::start();
    if(m_audioInput)m_audioInput->start(this);
}

void AudioBurstMskDemodulator::stop()
{
    if(m_audioInput)m_audioInput->stop();
    BurstMskDemodulator::stop();
}

void AudioBurstMskDemodulator::setSettings(Settings _settings)
{
    bool wasopen=isOpen();
    stop();

    //if Fs has changed or the audio device doesnt exist or the input device has changed then need to redo the audio device
    if((_settings.Fs!=settings.Fs)||(!m_audioInput)||(_settings.audio_device_in!=settings.audio_device_in))
    {
        settings=_settings;

        if(m_audioInput)m_audioInput->deleteLater();

        //set the format
        m_format.setSampleRate(settings.Fs);
        m_format.setChannelCount(1);
        m_format.setSampleSize(16);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::SignedInt);

        //setup
        m_audioInput = new QAudioInput(settings.audio_device_in, m_format, this);
        m_audioInput->setBufferSize(settings.Fs*settings.buffersizeinsecs);//buffersizeinsecs seconds of buffer
    }
    settings=_settings;
    BurstMskDemodulator::setSettings(settings);

    if(wasopen)start();

}

AudioBurstMskDemodulator::~AudioBurstMskDemodulator()
{
    stop();
}

