#include "textreplacement.h"
#include <QDateTime>
#include <QDebug>


TextReplacement::TextReplacement(QObject *parent) : QObject(parent)
{
    textmap = new TextReplacementMap(this);
    laststate=-1;
    replacedtext=false;
}

void TextReplacement::updatevolitilekeyvalues()
{
    QDateTime datetime=QDateTime::currentDateTimeUtc();

    textmap->map.insert("[time]",datetime.toLocalTime().time().toString());
    textmap->map.insert("[date]",datetime.toLocalTime().date().toString());
    textmap->map.insert("[time_utc]",datetime.time().toString());
    textmap->map.insert("[date_utc]",datetime.date().toString());
}

void TextReplacement::Replace(QString &text)
{
    orgtext=text;

    updatevolitilekeyvalues();

    for(int i=0;i<textmap->map.size();i++)text.replace(textmap->map.keys().at(i),textmap->map.values().at(i));

    orgtext_converted=text;
}

void TextReplacement::Restore(QString &text)
{
    text=orgtext;
}

void TextReplacement::RestoreIfUnchanged(QString &text)
{
    if(text==orgtext_converted)text=orgtext;
}

void TextReplacement::Replace(QPlainTextEdit *te)
{
    orgtext=te->toPlainText();
    orgtext_converted=orgtext;

    updatevolitilekeyvalues();

    for(int i=0;i<textmap->map.size();i++)orgtext_converted.replace(textmap->map.keys().at(i),textmap->map.values().at(i));


    if(orgtext==orgtext_converted)replacedtext=false;
     else
     {
        bool atend=te->textCursor().atEnd();
        te->setPlainText(orgtext_converted);
        if(atend)
        {
            QTextCursor cursor = te->textCursor();
            cursor.movePosition(QTextCursor::End);
            te->setTextCursor(cursor);
        }
        replacedtext=true;
     }

}

void TextReplacement::Restore(QPlainTextEdit *te)
{
    te->setPlainText(orgtext);
    replacedtext=false;
}

void TextReplacement::RestoreIfUnchanged(QPlainTextEdit *te)
{
    if(te->toPlainText()==orgtext_converted)te->setPlainText(orgtext);
}

void TextReplacement::Replace()
{
    if(plaintextedit.isNull())return;
    Replace(plaintextedit);
}

void TextReplacement::Restore()
{
    if(plaintextedit.isNull())return;
    Restore(plaintextedit);
}

void TextReplacement::RestoreIfUnchanged()
{
    if(plaintextedit.isNull())return;
    RestoreIfUnchanged(plaintextedit);
}

void TextReplacement::setPlainTextEdit(QPlainTextEdit *_plaintextedit)
{
    if(!_plaintextedit)return;
    plaintextedit=_plaintextedit;
}

