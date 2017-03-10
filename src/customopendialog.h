#ifndef CUSTOMOPENDIALOG_H
#define CUSTOMOPENDIALOG_H

#include <QDialog>
#include <QDateTime>

namespace Ui {
    class CustomOpenDialog;
}

class CustomOpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomOpenDialog(QString lastOpenPath, QWidget *parent = 0);
    ~CustomOpenDialog();

private:
    Ui::CustomOpenDialog *ui;
    QString lastOpenPath;

signals:
    void customOpen(QString fileName, int skip, int skipFirstRay, QDateTime, bool binary);

private slots:
    void openClicked();
    void successFinish();
};

#endif // CUSTOMOPENDIALOG_H
