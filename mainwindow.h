#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "documentadd.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void Print(QString id,QString status,QString relevance);
Ui::MainWindow *ui;

private slots:

    void on_ButtonToDocument_clicked();

    void on_ButtonToAddStopWords_clicked();

    void on_ButtonForSearch_clicked();

private:

    DocumentAdd *window_Doc;
public slots:
    void slotAddDocum(QString document1, QString document_id_string, QString status_string, QString rating_string);
};
#endif // MAINWINDOW_H
