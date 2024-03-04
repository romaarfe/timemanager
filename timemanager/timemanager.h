#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <QDialog>
#include <QDate>
#include <QTime>
#include <QTableWidgetItem>
#include <QFont>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QMessageBox>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui {
class TimeManager;
}
QT_END_NAMESPACE

class TimeManager : public QDialog
{
    Q_OBJECT

public:
    TimeManager(QWidget *parent = nullptr);
    ~TimeManager();

private slots:
    void on_pushButton_released();
    void on_pushButton_2_released();
    void on_pushButton_3_released();

private:
    int dailyCounter;
    QDate workDate;
    QTime workTime;

    void createAndSetTableItem(int row, int column, const QString &text, bool isBold);
    void updateTableItem(int row, int column, const QString &text, bool isBold);
    void executeUpdateQuery(const QString &columnName, const QString &value);
    void calculateHours();
    void generatePdfReport(int selectedMonthIndex);

    Ui::TimeManager *ui;
};
#endif // TIMEMANAGER_H
