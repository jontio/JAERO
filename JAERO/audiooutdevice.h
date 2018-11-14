#ifndef AUDIOOUTDEVICE_H
#define AUDIOOUTDEVICE_H

#include <QObject>
#include <QAudioOutput>
#include <QVector>

class AudioOutDevice : public QIODevice
{
    Q_OBJECT
public:
    struct Settings
    {
        QAudioDeviceInfo audio_device_out;
        double buffersizeinsecs;
        double Fs;
        Settings()
        {
            audio_device_out=QAudioDeviceInfo::defaultOutputDevice();
            buffersizeinsecs=1.0;
            Fs=8000;
        }
    };
    explicit AudioOutDevice(QObject *parent = 0);
    ~AudioOutDevice();
    void start();
    void stop();
    void setSettings(Settings settings);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
public slots:
    void audioin(const QByteArray &signed16array);
private:
    Settings settings;
    QAudioFormat m_format;
    QAudioOutput *m_audioOutput;
    QVector<qint16> circ_buffer;
    int circ_buffer_head;
    int circ_buffer_tail;
    void clear();

};

#endif // AUDIOOUTDEVICE_H
