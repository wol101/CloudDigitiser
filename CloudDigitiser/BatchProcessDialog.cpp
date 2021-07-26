#include <QFileDialog>

#include "BatchProcessDialog.h"
#include "ui_BatchProcessDialog.h"

BatchProcessDialog::BatchProcessDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchProcessDialog)
{
    ui->setupUi(this);
}

BatchProcessDialog::~BatchProcessDialog()
{
    delete ui;
}

void BatchProcessDialog::sourceFolderBrowse_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Select Source Folder"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks /*| QFileDialog::DontUseNativeDialog*/);
    if (folder.isNull() == false)
    {
        QDir::setCurrent(folder);
        ui->lineEditSourceFolder->setText(folder);
    }
}

void BatchProcessDialog::destinationFolderBrowse_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Select Destination Folder"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks /*| QFileDialog::DontUseNativeDialog*/);
    if (folder.isNull() == false)
    {
        QDir::setCurrent(folder);
        ui->lineEditDestinationFolder->setText(folder);
    }
}

void BatchProcessDialog::transformationFileBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open transformation matrix file"), ".", tr("Text Files (*.txt)"), 0/*, QFileDialog::DontUseNativeDialog*/);
    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        ui->lineEditTransformationFile->setText(fileName);
    }
}

QString BatchProcessDialog::GetSourceFolder()
{
    return ui->lineEditSourceFolder->text();
}

QString BatchProcessDialog::GetDestinationFolder()
{
    return ui->lineEditDestinationFolder->text();
}

QString BatchProcessDialog::GetTransformationFile()
{
    return ui->lineEditTransformationFile->text();
}
