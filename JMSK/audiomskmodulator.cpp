#include "audiomskmodulator.h"
#include <QDebug>
#include <QTimer>

AudioMskModulator::AudioMskModulator(QObject *parent)
:   MskModulator(parent),
  m_audioOutput(NULL)
{
    //
    stopping=false;
}

void AudioMskModulator::start()
{
    stopping=false;
    MskModulator::start();
    if(m_audioOutput)m_audioOutput->start(this);
}

void AudioMskModulator::stop()
{
    MskModulator::stop();
    if(m_audioOutput)m_audioOutput->stop();
}

void AudioMskModulator::stopgraceful()
{
    stopping=true;
    QTimer::singleShot(1300, this, SLOT(stopgracefulslot()));//clear 1.3 seconds of buffer
}

void AudioMskModulator::stopgracefulslot()
{
    if(stopping)stop();
}


AudioMskModulator::~AudioMskModulator()
{
    stop();
}

void AudioMskModulator::stateChanged(QAudio::State state)
{
    if(state==QAudio::StoppedState)
    {
        QTimer::singleShot(200, this, SLOT(DelayedStateClosedSlot()));
    }
    if(state==QAudio::ActiveState)
    {
        QTimer::singleShot(200, this, SLOT(DelayedStateOpenSlot()));
    }
}

void AudioMskModulator::DelayedStateClosedSlot()
{
    if(m_audioOutput&&(m_audioOutput->state()!=QAudio::StoppedState))return;
    emit closed();
    emit statechanged(false);
}

void AudioMskModulator::DelayedStateOpenSlot()
{
    if(m_audioOutput&&(m_audioOutput->state()!=QAudio::ActiveState))return;
    emit opened();
    emit statechanged(true);
}


void AudioMskModulator::setSettings(Settings _settings)
{
    bool wasopen=isOpen();
    stop();

    //if Fs has changed or the audio device doesnt exist then need to redo the audio device
    if((_settings.Fs!=settings.Fs)||(!m_audioOutput))
    {
        settings=_settings;

        if(m_audioOutput)
        {
            disconnect(m_audioOutput,SIGNAL(stateChanged(QAudio::State)),this,SLOT(stateChanged(QAudio::State)));
            m_audioOutput->deleteLater();
        }

        //set the format
        m_format.setSampleRate(settings.Fs);
        m_format.setChannelCount(1);
        m_format.setSampleSize(16);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::SignedInt);

        //setup
        m_audioOutput = new QAudioOutput(settings.audio_device_out, m_format, this);
        m_audioOutput->setBufferSize(settings.Fs);//1 second buffer
        connect(m_audioOutput,SIGNAL(stateChanged(QAudio::State)),this,SLOT(stateChanged(QAudio::State)));
    }
    settings=_settings;

    MskModulator::setSettings(settings);

    if(wasopen)start();

}
