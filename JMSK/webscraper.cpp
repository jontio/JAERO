#include "webscraper.h"
#include <QDebug>
#include <QTimer>

WebScraper::WebScraper(QObject *parent) : QObject(parent)
{
    doc = new QTextDocument(this);

    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(replyFinished(QNetworkReply*)));

    maintimer = new QTimer(this);
    connect(maintimer,SIGNAL(timeout()),this,SLOT(scrape()));

    running=false;

    setScrapingInterval(10);


}

void WebScraper::cachescrape()
{
    if(textmap.isNull())return;
    //qDebug()<<"Cache scrape";
    for(int iurl=0;iurl<urlmap.size();iurl++)
    {
        UrlItem urlitem=urlmap.values().at(iurl);
        QString url=urlmap.keys().at(iurl);
        //qDebug()<<"got"<<url<<"page.";
        for(int i=0;i<scrapemap.size();i++)
        {
            ScrapeItem ittscrapeitem=scrapemap.values().at(i);//get this scrape item
            if(ittscrapeitem.url!=url)continue;
            //qDebug()<<"       this is wanted by"<<scrapemap.keys().at(i)<<".";
            if(ittscrapeitem.rx.indexIn(urlitem.page)>=0)
            {
                if(ittscrapeitem.removehtmltags)
                {
                    doc->setHtml(ittscrapeitem.rx.cap(1));
                    textmap->map.insert(scrapemap.keys().at(i),doc->toPlainText());
                }
                 else textmap->map.insert(scrapemap.keys().at(i),ittscrapeitem.rx.cap(1));
            }
             else textmap->map.insert(scrapemap.keys().at(i),ittscrapeitem.valueifmissing);
        }
    }
}

void WebScraper::setScrapeMap(ScrapeMapContainer &map)
{
    scrapemap.clear();
    scrapemap=map.scrapemap;
    if(textmap.isNull())return;
    textmap.data()->map.clear();
    for(int i=0;i<scrapemap.size();i++)
    {
        textmap.data()->map.insert(scrapemap.keys().at(i),scrapemap.values().at(i).valueifmissing);
    }
    cachescrape();
}




void WebScraper::setTextReplacementMap(TextReplacementMap *_textmap)
{
    if(!_textmap)return;
    textmap=_textmap;
}

void WebScraper::scrape(int index)
{
    if(index>=scrapemap.size())
    {

        //remove all unvisited
        //qDebug()<<"remove all unvisited";
        for(int i=0;i<urlmap.size();i++)
        {
            UrlItem urlitem=urlmap.values().at(i);
            if(!urlitem.visited)
            {
                qDebug()<<"Web page removed due to being orphaned:"<<urlmap.keys().at(i);
                i-=urlmap.remove(urlmap.keys().at(i));
            }
        }

        if(running)start();
        return;//stop
    }
    currentscrapeitemindex=index;

    //qDebug()<<"scrape(int index)=="<<index;
    ScrapeItem scrapeitem=scrapemap.values().at(index);//get this scrape item
    UrlItem urlitem=urlmap.value(scrapeitem.url);//get url refering to this scrape item

    if(!urlitem.lastvisited.isNull())
    {
        if(urlitem.lastvisited.secsTo(nowish)>scrapeitem.refreshintervalsecs)
        {
            manager->get(QNetworkRequest(QUrl(scrapeitem.url)));
            return;
        }
         else
         {
            urlitem.visited=true;
            urlmap.insert(scrapeitem.url,urlitem);
            QTimer::singleShot(0,this,SLOT(scrapenext()));
            return;
         }
    }


    manager->get(QNetworkRequest(QUrl(scrapeitem.url)));
}

void WebScraper::scrapenext()
{
    currentscrapeitemindex++;
    scrape(currentscrapeitemindex);
}

void WebScraper::scrape()
{
    maintimer->stop();
    nowish=QDateTime::currentDateTimeUtc();
    //clear all visited
    for(int i=0;i<urlmap.size();i++)
    {
        UrlItem urlitem=urlmap.values().at(i);
        urlitem.visited=false;
        urlmap.insert(urlmap.keys().at(i),urlitem);
    }
    scrape(0);
}

void WebScraper::setScrapingInterval(int secs)
{
    if(secs<10)secs=10;
    maintimer->setInterval(secs*1000);
}

void WebScraper::start()
{
    //qDebug()<<"start";
    if(!running)
    {
        running=true;
        scrape();
    }
    maintimer->start();
}

void WebScraper::stop()
{
    //qDebug()<<"stop";
    running=false;
    maintimer->stop();
}

void WebScraper::replyFinished(QNetworkReply *reply)
{
    QString str;
    str=reply->readAll();

    if(currentscrapeitemindex>=scrapemap.values().size())
    {
        qDebug()<<"Warning this shouldn't happen WebScraper::replyFinished(QNetworkReply *reply)";
        return;
    }

    ScrapeItem scrapeitem=scrapemap.values().at(currentscrapeitemindex);//get this scrape item
    UrlItem urlitem=urlmap.value(scrapeitem.url);//get url refering to this scrape item

    qDebug()<<"got"<<scrapeitem.url<<"page.";

    urlitem.visited=true;
    urlitem.lastvisited=nowish;
    urlitem.page=str;
    urlmap.insert(scrapeitem.url,urlitem);

    if(!textmap.isNull())
    {
        for(int i=0;i<scrapemap.size();i++)
        {
            ScrapeItem ittscrapeitem=scrapemap.values().at(i);//get this scrape item
            if(ittscrapeitem.url!=scrapeitem.url)continue;
            qDebug()<<"       this is wanted by"<<scrapemap.keys().at(i)<<".";
            if(ittscrapeitem.rx.indexIn(str)>=0)
            {
                if(ittscrapeitem.removehtmltags)
                {
                    doc->setHtml(ittscrapeitem.rx.cap(1));
                    textmap->map.insert(scrapemap.keys().at(i),doc->toPlainText());
                }
                 else textmap->map.insert(scrapemap.keys().at(i),ittscrapeitem.rx.cap(1));
            }
             else textmap->map.insert(scrapemap.keys().at(i),ittscrapeitem.valueifmissing);
        }
    }

    reply->deleteLater();
    scrapenext();//next


}

