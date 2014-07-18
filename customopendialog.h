#ifndef CUSTOMOPENDIALOG_H
#define CUSTOMOPENDIALOG_H

#include <QDialog>

namespace Ui {
class CustomOpenDialog;
}

class CustomOpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomOpenDialog(QWidget *parent = 0);
    ~CustomOpenDialog();

private:
    Ui::CustomOpenDialog *ui;

signals:
    void customOpen(QString fileName, int skip, int skipFirstRay, bool binary);

private slots:
    void openClicked();
    void successFinish();
};

#endif // CUSTOMOPENDIALOG_H
