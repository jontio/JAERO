#ifndef PLANELOG_H
#define PLANELOG_H

#include <QMainWindow>
#include "../aerol.h"

namespace Ui {
class PlaneLog;
}

class PlaneLog : public QWidget
{
    Q_OBJECT

public:
    explicit PlaneLog(QWidget *parent = 0);
    ~PlaneLog();
protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
public slots:
    void ACARSslot(ACARSItem &acarsitem);

private slots:

    void on_actionClear_triggered();

    void on_actionUpDown_triggered();

    void on_actionLeftRight_triggered();

    void on_actionStopSorting_triggered();

private:
    Ui::PlaneLog *ui;
    int wantedheightofrow;
    QToolBar * toolBar;
};

#endif // PLANELOG_H
