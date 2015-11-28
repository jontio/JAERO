#include <QScrollBar>
#include <QTextBlock>
#include <QtCore/QDebug>
#include <QMimeData>
#include <QMenu>

#include "textinputwidget.h"


TextInputWidget::TextInputWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
    QPalette p = palette();
    p.setColor(QPalette::Base,QColor(245, 245, 245, 200));
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);
    QFont f=font();
    f.setPointSize(11);//12);
    setFont(f);

    setMaximumBlockCount(10000);

    setLineWrapMode(QPlainTextEdit::NoWrap);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,SIGNAL(customContextMenuRequested(const QPoint&)),
            this,SLOT(showContextMenu(const QPoint &)));

    lastredcharpos=0;

    defaultformat=currentCharFormat();
    defaultformat.setForeground(Qt::black);

    redformat=currentCharFormat();
    redformat.setForeground(QColor("#990000"));

    textinputdevice = new TextInputDevice(this);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(redupdate()));
    timer->start(500);

}

void TextInputWidget::showContextMenu(const QPoint &pt)
{
    QMenu *menu = createStandardContextMenu();
    QList<QAction *> actions=menu->actions();

    //hack menu
    for(int i=0;i<actions.size();i++)
    {
        //qDebug()<<actions[i]->text();
        if(actions[i]->text().contains("undo",Qt::CaseInsensitive))
        {
            menu->removeAction(actions[i]);
        }
        if(actions[i]->text().contains("redo",Qt::CaseInsensitive))
        {
            menu->removeAction(actions[i]);
        }
        if(actions[i]->text().contains("delete",Qt::CaseInsensitive))
        {
            menu->removeAction(actions[i]);
        }
        if(actions[i]->text().contains("cu",Qt::CaseInsensitive))
        {
            actions[i]->disconnect(SIGNAL(triggered(bool)));
            connect(actions[i],SIGNAL(triggered(bool)),this,SLOT(cut()));
        }
        if(actions[i]->text().contains("paste",Qt::CaseInsensitive))
        {
            actions[i]->disconnect(SIGNAL(triggered(bool)));
            connect(actions[i],SIGNAL(triggered(bool)),this,SLOT(paste()));
        }
    }

    menu->exec(mapToGlobal(pt));
    delete menu;
}

void TextInputWidget::redupdate()
{

    if(textinputdevice->charpos<0)return;

    if(lastredcharpos==textinputdevice->charpos)return;
    if(lastredcharpos>textinputdevice->charpos)lastredcharpos=0;

    QTextCursor cursor=QPlainTextEdit::textCursor();
    cursor.setPosition(lastredcharpos, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, textinputdevice->charpos-lastredcharpos);
    cursor.setCharFormat(redformat);

    lastredcharpos=textinputdevice->charpos;
}

void TextInputWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {

    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        QPlainTextEdit::keyPressEvent(e);
        break;
    default:

        document()->clearUndoRedoStacks();

        QTextCursor tc=QPlainTextEdit::textCursor();
        setCurrentCharFormat(defaultformat);//for keys

        if(tc.selectionStart()<textinputdevice->charpos)
        {
            //qDebug()<<e->key();
            if(tc.selectionStart()!=tc.selectionEnd())lastredcharpos=tc.selectionStart();
            if(e->modifiers()==Qt::ControlModifier)
            {
                if(e->key()==Qt::Key_X)break;
                if(e->key()==Qt::Key_Y)break;
                if(e->key()==Qt::Key_Z)break;
                if(e->key()==Qt::Key_C)QPlainTextEdit::keyPressEvent(e);
            }
            break;
        }
        if((tc.selectionEnd()==textinputdevice->charpos)&&(e->key()==Qt::Key_Backspace))break;


        if(e->text().size()&&e->text().toLatin1()[0])
        {

            //qDebug()<<"Key";
            //qDebug()<<"start "<<tc.selectionStart();
            //qDebug()<<"end "<<tc.selectionEnd();

            int selectionstart=tc.selectionStart();
            int selectionend=tc.selectionEnd();

            int charsremoved=abs(selectionend-selectionstart);

            if(e->key()==Qt::Key_Backspace&&!charsremoved)
            {
                   if(selectionstart)
                   {
                       selectionstart--;
                       charsremoved++;
                   }
            }

            if(e->key()==Qt::Key_Delete&&!charsremoved)
            {
                   charsremoved++;
            }


            textinputdevice->str.remove(selectionstart,charsremoved);

            if((e->key()==Qt::Key_Backspace||e->key()==Qt::Key_Delete||e->text().toLatin1()[0]<(char)0x20)&&!(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter))
            {
                // no chars added
            }
             else textinputdevice->str.insert(selectionstart,e->text());


        }

        QPlainTextEdit::keyPressEvent(e);

    }
}

void TextInputWidget::cut()
{
    QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier);
    keyPressEvent(event);
}

void TextInputWidget::paste()
{
    QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
    keyPressEvent(event);
}

void TextInputWidget::reset()
{

    textinputdevice->reset();
    textinputdevice->str=toPlainText();

    QTextCursor cursor=QPlainTextEdit::textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.setCharFormat(defaultformat);

}

void TextInputWidget::clear()
{
    textinputdevice->str.clear();
    textinputdevice->charpos=0;
    QPlainTextEdit::clear();
}

void TextInputWidget::insertFromMimeData ( const QMimeData * source )
{

    QTextCursor tc=QPlainTextEdit::textCursor();
    //qDebug()<<"Paste";

    if(tc.selectionStart()<textinputdevice->charpos)return;



    int selectionstart=tc.selectionStart();
    int selectionend=tc.selectionEnd();
    int charsremoved=selectionend-selectionstart;
    //int charsadded=source->text().size();

    //qDebug()<<"start "<<selectionstart;
    //qDebug()<<"end "<<selectionend;
    //qDebug()<<"changed "<<(charsadded-charsremoved);
    //qDebug()<<"peekpos "<<textinputdevice->charpos;


    textinputdevice->str.remove(selectionstart,charsremoved);
    textinputdevice->str.insert(selectionstart,source->text());

    tc.setCharFormat(defaultformat);//for pastes
    QPlainTextEdit::insertPlainText( source->text() );
}
