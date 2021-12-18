#ifndef DOCUMENTADD_H
#define DOCUMENTADD_H

#include <QDialog>


namespace Ui {
class DocumentAdd;
}

class DocumentAdd : public QDialog
{
    Q_OBJECT

public:
    explicit DocumentAdd(QWidget *parent = nullptr);
    ~DocumentAdd();
public slots:
    void stop_slot(QString stop_words_line);

private slots:
    void on_ButtonToAddDocument_clicked();

private:
    Ui::DocumentAdd *ui;

signals:
    void signal(QString document1, QString document_id_string, QString status_string, QString rating_string);
};

#endif // DOCUMENTADD_H
