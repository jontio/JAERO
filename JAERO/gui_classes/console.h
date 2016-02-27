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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QPlainTextEdit>
#include <QBuffer>
#include <QIODevice>

class ConsoleDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit ConsoleDevice(QObject *parent = 0) : QIODevice(parent)
    {
        //
    }
    qint64 readData(char *data, qint64 len)
    {
        Q_UNUSED(data);
        Q_UNUSED(len);
        return 0;
    }
    qint64 writeData(const char *data, qint64 len)
    {
        ba.clear();
        ba.append(data,len);
        emit PlainData(ba);
        return len;
    }
    ~ConsoleDevice()
    {
        //
    }
signals:
    void PlainData(const QByteArray &data);
public slots:
private:
    QByteArray ba;
};

class Console : public QPlainTextEdit
{
    Q_OBJECT

signals:
    void getData(const QByteArray &data);

public:
    explicit Console(QWidget *parent = 0);

    void setLocalEchoEnabled(bool set);

    ConsoleDevice *consoledevice;
public slots:

    void clear();

    void setEnableUpdates(bool enable, QString msg);
    void setEnableUpdates(bool enable);
    void putPlainData(const QByteArray &data);
    //void putEncodedVaricodeData(const QByteArray &data);

protected:
    //virtual void keyPressEvent(QKeyEvent *e);
    //virtual void mousePressEvent(QMouseEvent *e);
    //virtual void mouseDoubleClickEvent(QMouseEvent *e);
    //virtual void contextMenuEvent(QContextMenuEvent *e);

private:
    bool localEchoEnabled;

    bool enableupdates;
    QString savedstr;
};

#endif // CONSOLE_H
