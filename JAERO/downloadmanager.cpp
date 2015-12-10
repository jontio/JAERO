#include "downloadmanager.h"

#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <stdio.h>
#include <QDebug>
#include <QMessageBox>
#include <QDir>

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent), downloadedCount(0), totalCount(0)
{
    pd.setModal(true);
    pd.close();
}

DownloadManager::~DownloadManager()
{

}

void DownloadManager::append(const QList<QUrlExt> &urlextList)
{
    foreach (QUrlExt urlext, urlextList)
        append(urlext);

    if (downloadQueue.isEmpty())
        QTimer::singleShot(0, this, SIGNAL(finished()));
}

void DownloadManager::append(const QUrlExt &urlext)
{
    if (downloadQueue.isEmpty())
        QTimer::singleShot(0, this, SLOT(startNextDownload()));

    downloadQueue.enqueue(urlext);
    ++totalCount;
}

QString DownloadManager::saveFileName(const QUrlExt &urlext)
{
    QString path = urlext.url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())basename = "download";
    if(!urlext.filename.isEmpty())basename=urlext.filename;

    QFileInfo fileinfo(basename);
    basename=fileinfo.fileName();
    QString dirpath=fileinfo.dir().absolutePath();

    if(urlext.overwrite)return dirpath+"/tmp."+basename;

    if (QFile::exists(dirpath+"/"+basename)) {
        // already exists, don't overwrite
        int i = 0;
        basename += '.';
        while (QFile::exists(dirpath+"/"+basename + QString::number(i)))
            ++i;

        basename += QString::number(i);
    }

    return dirpath+"/tmp."+basename;
}

void DownloadManager::startNextDownload()
{
    if (downloadQueue.isEmpty())
    {
        pd.hide();
        qDebug(qPrintable(((QString)"").sprintf("%d/%d files downloaded successfully\n", downloadedCount, totalCount)));

        if(downloadedCount||(totalCount>1))
        {
            QMessageBox msgBox;
            msgBox.setText("Downloading done.");
            msgBox.setInformativeText(((QString)"").sprintf("%d out of %d files downloaded successfully\n", downloadedCount, totalCount));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();
        }

        emit finished();
        return;
    }


    QUrlExt urlext=downloadQueue.dequeue();
    QUrl url = urlext.url;

    QString filename = saveFileName(urlext);
    output.setFileName(filename);

    QFileInfo fileinfo(filename);
    QDir dir;
    dir.mkpath(fileinfo.dir().absolutePath());
    if (!output.open(QIODevice::WriteOnly))
    {


        QMessageBox msgBox;
        msgBox.setText("Cant open file \""+filename+"\" for saving.");
        msgBox.setInformativeText(output.errorString());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        qCritical(qPrintable(((QString)"").sprintf(

                                 "Problem opening save file '%s' for download '%s': %s\n",
                                                 qPrintable(filename), url.toEncoded().constData(),
                                                 qPrintable(output.errorString())

                                 )));

        startNextDownload();
        return;                 // skip this download
    }

    QNetworkRequest request(url);
    currentDownload = manager.get(request);
    connect(currentDownload, SIGNAL(downloadProgress(qint64,qint64)),
            SLOT(downloadProgress(qint64,qint64)));
    connect(currentDownload, SIGNAL(finished()),
            SLOT(downloadFinished()));
    connect(currentDownload, SIGNAL(readyRead()),
            SLOT(downloadReadyRead()));

    connect(&pd,SIGNAL(canceled()),currentDownload,SLOT(abort()));

    // prepare the output
    pd.setLabelText("Downloading "+url.toEncoded());
    pd.setValue(0);
    pd.show();

}

void DownloadManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{

    double progress=qRound(100.0*((double)bytesReceived)/((double)bytesTotal));
    pd.setValue(progress);
}

void DownloadManager::downloadFinished()
{

    pd.setValue(0);
    pd.setLabelText("");
    pd.close();

    output.close();

    if (currentDownload->error())
    {
        output.remove();

        // download failed

        qCritical(qPrintable(((QString)"").sprintf(

                                 "Download Failed: %s", qPrintable(currentDownload->errorString())

                                 )));

        QMessageBox msgBox;
        msgBox.setText("Download failed.");
        msgBox.setInformativeText(currentDownload->errorString());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    } else {

        //download done

        //rename
        QFileInfo fileinfo(output.fileName());
        QString str=fileinfo.dir().absolutePath()+"/"+fileinfo.fileName().mid(4);
        QFile oldfile(str);
        oldfile.setPermissions(QFile::ReadOther | QFile::WriteOther);
        if(oldfile.exists()&&(!oldfile.remove()))
        {
            QMessageBox msgBox;
            msgBox.setText("Can't rename file file.");
            msgBox.setInformativeText(oldfile.errorString());
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            qDebug()<<oldfile.errorString();
        }
        output.rename(str);


        ++downloadedCount;
    }

    currentDownload->deleteLater();
    startNextDownload();
}

void DownloadManager::downloadReadyRead()
{
    output.write(currentDownload->readAll());
}

