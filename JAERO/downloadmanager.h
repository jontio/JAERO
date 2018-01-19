#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QFile>
#include <QObject>
#include <QQueue>
#include <QTime>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QProgressDialog>

//not perfect but ok.
//temp nameing may have problems if user has a file called tmp.test.txt and tries to download test.txt then tmp.test.txt will be overwritte
//but who names a file tmp.text.txt and expects to keep it.

class DownloadManager: public QObject
{
    Q_OBJECT
public:
    struct QUrlExt{
        QUrl url;
        QString filename;
        bool overwrite;
        QUrlExt()
        {
           url.clear();
           filename.clear();
           overwrite=false;
        }
    };
    DownloadManager(QObject *parent = 0);
    ~DownloadManager();

    void append(const QUrlExt &urlext);
    void append(const QList<QUrlExt> &urlextList);
    QString saveFileName(const QUrlExt &urlext);

signals:
    void finished();
    void downloadresult(const QUrl &url,bool result);

private slots:
    void startNextDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadReadyRead();

private:

    QNetworkAccessManager manager;
    QQueue<QUrlExt> downloadQueue;
    QNetworkReply *currentDownload;
    QFile output;

    QProgressDialog pd;

    int downloadedCount;
    int totalCount;

    QMap<QString,QString> tmptofilalfilenamemap;
};



#endif // DOWNLOADMANAGER_H
