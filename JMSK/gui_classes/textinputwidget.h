#ifndef TEXTINPUTWIDGET_H
#define TEXTINPUTWIDGET_H

#include <QObject>
#include <QPlainTextEdit>
#include <QBuffer>
#include <QIODevice>
#include <QString>
#include <QDebug>
#include <QTimer>

class TextInputDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit TextInputDevice(QPlainTextEdit *parent ) : QIODevice(parent)
    {
        str.clear();
        charpos=0;
        preamble_ptr=0;
        postamble_ptr=0;
        emittedeof=false;
        idlebytes.push_back((char)0);
        //idlebytes.push_back(12);
        idlebytes.push_back(22);

        idle_on_eof=false;

        preamble="NOCALL NOCALL NOCALL NOCALL NOCALL\n";
        postamble="\n73 NOCALL";
    }

    int charpos;

    qint64 readData(char *data, qint64 maxlen)
    {

        if(!maxlen)return 0;


        //idling bytes
        if(idle_on_eof&&(preamble_ptr>=preamble.size()))
        {
            static char lastchar=0;
            if(charpos>=str.size())
            {
                int cnt=0;
                for(;cnt<maxlen;cnt++)
                {
                    data[cnt]=idlebytes[qrand()%idlebytes.size()];
                    lastchar=2;
                }
                return cnt;
            }
            if(lastchar>0)//for some reason a few SYN chars are needed right b4 the data
            {
                int cnt=0;
                for(;cnt<maxlen&&lastchar>0;cnt++)
                {
                    data[cnt]=22;
                    lastchar--;
                }
                return cnt;
            }

        }

        //preamble text
        if((charpos==0)&&(preamble_ptr<preamble.size()))
        {
            QString tstr=preamble.mid(preamble_ptr,maxlen);
            for(int i=0;i<tstr.size();i++)
            {
                data[i]=tstr.at(i).toLatin1();
            }
            preamble_ptr+=tstr.size();
            return tstr.size();
        }

        //postamble text
        if(charpos>=str.size()&&(postamble_ptr<postamble.size()))
        {
            QString tstr=postamble.mid(postamble_ptr,maxlen);
            for(int i=0;i<tstr.size();i++)
            {
                data[i]=tstr.at(i).toLatin1();
            }
            postamble_ptr+=tstr.size();
            return tstr.size();
        }

        //main text
        QString tstr=str.mid(charpos,maxlen);
        for(int i=0;i<tstr.size();i++)
        {
            data[i]=tstr.at(i).toLatin1();
        }
        charpos+=tstr.size();
        if(maxlen&&!tstr.size()&&!emittedeof)
        {
            emittedeof=true;
            emit eof();
        }
        return tstr.size();

    }
    qint64 writeData(const char *data, qint64 len)
    {
        Q_UNUSED(data);
        Q_UNUSED(len);
        return 0;
    }
    ~TextInputDevice()
    {
        //
    }
    bool reset()
    {
        str.clear();
        charpos=0;
        preamble_ptr=0;
        postamble_ptr=0;
        emittedeof=false;
        return QIODevice::reset();
    }
    QString str;
    QByteArray idlebytes;
    bool idle_on_eof;
    QString preamble;
    QString postamble;
signals:
    void eof();
public slots:
    void setIdle_on_eof(bool state)
    {
        idle_on_eof=state;
    }
private:
    bool emittedeof;
    int preamble_ptr;
    int postamble_ptr;
};

class TextInputWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit TextInputWidget(QWidget *parent = 0);
    TextInputDevice *textinputdevice;

    int preamble_bytes;
    int postamble_bytes;
    QString textamble;


public slots:
    void cut();
    void paste();
    void reset();
    void clear();

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    void insertFromMimeData ( const QMimeData * source );
    void dropEvent(QDropEvent * e)
    {
        Q_UNUSED(e);
    }
    void dragMoveEvent(QDragEnterEvent * e)
    {
        Q_UNUSED(e);
    }
    void dragEnterEvent(QDragEnterEvent * e)
    {
        Q_UNUSED(e);
    }

private:
    QTimer *timer;
    QTextCharFormat defaultformat;
    QTextCharFormat redformat;
    int lastredcharpos;

private slots:
    void redupdate();
    void showContextMenu(const QPoint &pt);

};

#endif // TEXTINPUTWIDGET_H
