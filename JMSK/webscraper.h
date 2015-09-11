#ifndef WEBSCRAPER_H
#define WEBSCRAPER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegExp>
#include <QPointer>
#include <QTimer>
#include <QTextDocument>
#include <QDateTime>
#include <QMap>
#include "textreplacement.h"

struct ScrapeItem
{
    QString valueifmissing;
    QString url;
    QRegExp rx;
    int refreshintervalsecs;
    bool removehtmltags;
};

struct UrlItem
{
    QString page;
    QDateTime lastvisited;
    bool visited;
    UrlItem()
    {
        visited=false;
    }
};

class ScrapeMapContainer : public QObject
{
    Q_OBJECT
public:
    explicit ScrapeMapContainer(QObject *parent = 0) : QObject(parent)
    {
        //
    }
    QMap<QString,ScrapeItem> scrapemap;
};

class WebScraper : public QObject
{
    Q_OBJECT
public:
    explicit WebScraper(QObject *parent = 0);
    void setTextReplacementMap(TextReplacementMap *textmap);
    void setScrapingInterval(int secs);

    void setScrapeMap(ScrapeMapContainer &map);

signals:
public slots:
    void start();
    void stop();
    void cachescrape();
private slots:
    void replyFinished(QNetworkReply *reply);
    void scrape();
    void scrapenext();
private:
    void scrape(int index);

    QDateTime nowish;

    QPointer< TextReplacementMap > textmap;


    QNetworkAccessManager *manager;
    int currentscrapeitemindex;
    QTimer *maintimer;
    bool running;

    void seturlmapsunvisited();
    void removeunvisiteditems();

    QMap<QString,UrlItem> urlmap;//url,item
    QMap<QString,ScrapeItem> scrapemap;//[text],item

    QTextDocument *doc;



};

#endif // WEBSCRAPER_H
