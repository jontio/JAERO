#ifndef COMPRESSEDAUDIODISKWRITER_H
#define COMPRESSEDAUDIODISKWRITER_H

#include <QObject>

#include <QFile>
#include <QTimer>

extern "C"
{
#include <vorbis/vorbisenc.h>
}

class CompressedAudioDiskWriter : public QObject
{
    Q_OBJECT
public:
    explicit CompressedAudioDiskWriter(QObject *parent = 0);
    ~CompressedAudioDiskWriter();

    void setLogDir(QString dir);

signals:
public slots:
    void audioin(const QByteArray &signed16arraymono);
    void Call_progress_Slot(QByteArray infofield);
private slots:
    void timeoutslot();
private:

    bool os_valid;
    bool og_valid;
    bool op_valid;
    bool vi_valid;
    bool vc_valid;
    bool vd_valid;
    bool vb_valid;

    ogg_stream_state os; // take physical pages, weld into a logical stream of packets
    ogg_page         og; // one Ogg bitstream page.  Vorbis packets are inside
    ogg_packet       op; // one raw packet of data for decode

    vorbis_info      vi; // struct that stores all the static vorbis bitstream settings
    vorbis_comment   vc; // struct that stores all the user comments

    vorbis_dsp_state vd; // central working state for the packet->PCM decoder
    vorbis_block     vb; // local working space for packet->PCM decode

    QFile outfile;

    int slowdown;

    QTimer *timeout;

    QString logdir;

    void closeFile();
    void openFileForOutput(QString filename);

    QByteArray current_call_progress_infofield;
};

#endif // COMPRESSEDAUDIODISKWRITER_H
