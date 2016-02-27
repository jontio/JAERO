/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "console.h"

#include <QScrollBar>

#include <QTextBlock>

#include <QtCore/QDebug>



Console::Console(QWidget *parent)
    : QPlainTextEdit(parent)
    , localEchoEnabled(false)
{

    //or act as a console device
    consoledevice = new ConsoleDevice(this);
    connect(consoledevice,SIGNAL(PlainData(QByteArray)),this,SLOT(putPlainData(QByteArray)));

    document()->setMaximumBlockCount(1000);
    /*QPalette p = palette();
    p.setColor(QPalette::Base, Qt::white);//black);
    p.setColor(QPalette::Text, Qt::black);//green);
    setPalette(p);*/
    QFont f=font();
    f.setPointSize(12);
    setFont(f);

    setReadOnly(true);

    enableupdates=true;

}

void Console::setEnableUpdates(bool enable, QString msg)
{
    if(!enableupdates)setPlainText(msg);
    if(enable==enableupdates)return;
    if(!enable)
    {
        savedstr=toPlainText();
        setPlainText(msg);
        setEnabled(false);
    }
     else
     {
        setPlainText(savedstr);
        setEnabled(true);
     }
    enableupdates=enable;
}

void Console::setEnableUpdates(bool enable)
{
    if(enable==enableupdates)return;
    if(!enable)
    {
        savedstr=toPlainText();
        setEnabled(false);
    }
     else
     {
        setPlainText(savedstr);
        moveCursor(QTextCursor::End);
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        setEnabled(true);
     }
    enableupdates=enable;
}

//sort of works but has problems when the top line is deleted, then everything moves up by 1 line. how to fix?
//if someone could do a better job of this that would be good.
void Console::putPlainData(const QByteArray &_data)
{

    QByteArray data=_data;
    data.replace((char)22,"");

    if(!enableupdates)return;
    const QTextCursor old_cursor = textCursor();
    const int old_scrollbar_value = verticalScrollBar()->value();
    const bool is_scrolled_down = old_scrollbar_value == verticalScrollBar()->maximum();

    // Move the cursor to the end of the document.
    moveCursor(QTextCursor::End);

    // Insert the text at the position of the cursor (which is the end of the document).
    textCursor().insertText(QString(data));

    if (old_cursor.hasSelection() || !is_scrolled_down)
    {
        // The user has selected text or scrolled away from the bottom: maintain position.
        setTextCursor(old_cursor);
        verticalScrollBar()->setValue(old_scrollbar_value);
    }
    else
    {
        // The user hasn't selected any text and the scrollbar is at the bottom: scroll to the bottom.
        moveCursor(QTextCursor::End);
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
}

void Console::setLocalEchoEnabled(bool set)
{
    localEchoEnabled = set;
}

void Console::clear()
{
    //savedstr.clear();
    if(!enableupdates)return;
    QPlainTextEdit::clear();
}

/*void Console::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
  //  case Qt::Key_Backspace:
  //  case Qt::Key_Left:
  //  case Qt::Key_Right:
  //  case Qt::Key_Up:
  //  case Qt::Key_Down:
  //      break;
    default:
        if (localEchoEnabled)
            QPlainTextEdit::keyPressEvent(e);
        emit getData(e->text().toLocal8Bit());
    }
}*/

/*void Console::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    setFocus();
}

void Console::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
}

void Console::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
}*/
