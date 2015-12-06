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
    QString imagesfolder;
    QString planelookup;
protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    //void contextMenuEvent(const QPoint &pos);
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
public slots:
    void ACARSslot(ACARSItem &acarsitem);

private slots:

    void on_actionClear_triggered();

    void on_actionUpDown_triggered();

    void on_actionLeftRight_triggered();

    void on_actionStopSorting_triggered();

    void on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_toolButtonimg_clicked();

    void on_actionCopy_triggered();

    void on_plainTextEditnotes_textChanged();


private:
    Ui::PlaneLog *ui;
    int wantedheightofrow;
    QToolBar * toolBar;
    void updateinfopain(int row);
    QList<int> savedsplitter2;

    int updateinfoplanrow;


};

#endif // PLANELOG_H
