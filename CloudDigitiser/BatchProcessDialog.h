#ifndef BATCHPROCESSDIALOG_H
#define BATCHPROCESSDIALOG_H

#include <QDialog>

namespace Ui {
class BatchProcessDialog;
}

class BatchProcessDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit BatchProcessDialog(QWidget *parent = 0);
    ~BatchProcessDialog();

    QString GetSourceFolder();
    QString GetDestinationFolder();
    QString GetTransformationFile();

public slots:
    void sourceFolderBrowse_clicked();
    void destinationFolderBrowse_clicked();
    void transformationFileBrowse_clicked();

private:
    Ui::BatchProcessDialog *ui;
};

#endif // BATCHPROCESSDIALOG_H
