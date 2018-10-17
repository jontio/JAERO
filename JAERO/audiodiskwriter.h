#ifndef AUDIODISKWRITER_H
#define AUDIODISKWRITER_H

#include <QObject>

class AudioDiskWriter : public QObject
{
    Q_OBJECT
public:
    explicit AudioDiskWriter(QObject *parent = 0);

signals:

public slots:
};

#endif // AUDIODISKWRITER_H