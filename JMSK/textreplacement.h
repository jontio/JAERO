#ifndef TEXTREPLACEMENT_H
#define TEXTREPLACEMENT_H

#include <QObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QMap>

class TextReplacementMap : public QObject
{
    Q_OBJECT
public:
    explicit TextReplacementMap(QObject *parent = 0) : QObject(parent)
    {
        //
    }
    QMap<QString,QString> map;
};

class TextReplacement : public QObject
{
    Q_OBJECT
public:
    explicit TextReplacement(QObject *parent = 0);
    void Replace(QString &text);
    void Restore(QString &text);
    void RestoreIfUnchanged(QString &text);
    void Replace(QPlainTextEdit *text);
    void Restore(QPlainTextEdit *text);
    void RestoreIfUnchanged(QPlainTextEdit *text);
    void Replace();
    void Restore();
    void RestoreIfUnchanged();
    void setPlainTextEdit(QPlainTextEdit *plaintextedit);
    TextReplacementMap *textmap;
    bool replacedtext;
signals:
public slots:
    void onTXstart()
    {
        if(laststate==1)return;
        Replace();
        laststate=1;
    }
    void onTXstop()
    {
        if(laststate==0)return;
        RestoreIfUnchanged();
        laststate=0;
    }
    void onstatechange(bool state)
    {
        if(state)onTXstart();
         else onTXstop();
    }
private slots:
private:
    int laststate;
    QString orgtext;
    QString orgtext_converted;
    QPointer<QPlainTextEdit> plaintextedit;
    void updatevolitilekeyvalues();    
};

#endif // TEXTREPLACEMENT_H
