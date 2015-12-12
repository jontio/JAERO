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
#include <assert.h>

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

    QString filename_final=dirpath+"/"+basename;
    QString filename_tmp=filename_final+".temp";

    if (QFile::exists(filename_tmp)||tmptofilalfilenamemap.contains(filename_tmp)) {
        // already exists, don't overwrite
        int i = 0;
        filename_tmp += '.';
        while (QFile::exists(filename_tmp + QString::number(i))||tmptofilalfilenamemap.contains(filename_tmp))
            ++i;

        filename_tmp += QString::number(i);
    }

    if(!urlext.overwrite)
    {
        if (QFile::exists(filename_final)||tmptofilalfilenamemap.values().contains(filename_final)) {
            // already exists, don't overwrite
            int i = 0;
            filename_final += '.';
            while (QFile::exists(filename_final + QString::number(i))||tmptofilalfilenamemap.values().contains(filename_final))
                ++i;

            filename_final += QString::number(i);
        }
    }

    assert(tmptofilalfilenamemap.contains(filename_tmp)==false);
    tmptofilalfilenamemap.insert(filename_tmp,filename_final);

    return filename_tmp;
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
        msgBox.setText("Cant open file \""+tmptofilalfilenamemap.value(filename)+"\" for saving.");
        msgBox.setInformativeText(output.errorString());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        qCritical(qPrintable(((QString)"").sprintf(

                                 "Problem opening save file '%s' for download '%s': %s\n",
                                                 qPrintable(tmptofilalfilenamemap.value(filename)), url.toEncoded().constData(),
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

    QString filename_final=tmptofilalfilenamemap.take(output.fileName());
    assert(!filename_final.isEmpty());

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
        QFile oldfile(filename_final);
        oldfile.setPermissions(QFile::ReadOther | QFile::WriteOther);
        if(oldfile.exists()&&(!oldfile.remove()))
        {
            QMessageBox msgBox;
            msgBox.setText("Can't rename fial file.");
            msgBox.setInformativeText(oldfile.errorString());
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            qDebug()<<oldfile.errorString();
        }
        if(output.rename(filename_final))emit downloadresult(currentDownload->url(),true);
         else emit downloadresult(currentDownload->url(),false);

        ++downloadedCount;
    }

    currentDownload->deleteLater();
    startNextDownload();
}

void DownloadManager::downloadReadyRead()
{
    output.write(currentDownload->readAll());
}

