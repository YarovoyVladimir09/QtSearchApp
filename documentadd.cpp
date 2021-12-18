#include "documentadd.h"
#include "ui_documentadd.h"


DocumentAdd::DocumentAdd(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DocumentAdd)
{
    ui->setupUi(this);




}
QString stop_words_line_from_="";
DocumentAdd::~DocumentAdd()
{
    delete ui;
}

void DocumentAdd::stop_slot(QString stop_words_line)
{
stop_words_line_from_=stop_words_line;
}

void DocumentAdd::on_ButtonToAddDocument_clicked()
{
    QString document=ui->document->text();
    QString document_id_string=ui->document_id_string->text();
    QString status_string=ui->status_string->text();
    QString rating_string=ui->rating_string->text();
    emit signal(document,document_id_string,status_string,rating_string);
    ui->document->setText("");
    ui->document_id_string->setText("");
    ui->status_string->setText("");
    ui->rating_string->setText("");
  //  QWidget::close();
}

