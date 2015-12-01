#ifndef PLANELOG_H
#define PLANELOG_H

#include <QWidget>
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
public slots:
    void update(ISUItem &isuitem);

private slots:
    void on_pushButtonsmall_clicked();

    void on_pushButtonlarge_clicked();

    void on_pushButtonstopsort_clicked();

    void on_pushButtonsrink_clicked();

    void on_pushButtonexpand_clicked();

    void on_pushButtonclear_clicked();

private:
    Ui::PlaneLog *ui;
    int wantedheightofrow;
};

#endif // PLANELOG_H
