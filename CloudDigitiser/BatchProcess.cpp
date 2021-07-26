#include <QDir>
#include <QApplication>
#include <QProgressDialog>
#include <QString>

#include "BatchProcess.h"
#include "PointCloud.h"
#include "pipeline/math_3d.h"

BatchProcess::BatchProcess()
{
}

void BatchProcess::SetInputs(QString sourceFolder, QString destinationFolder, QString transformationFile)
{
    m_sourceFolder = sourceFolder;
    m_destinationFolder = destinationFolder;
    m_transformationFile = transformationFile;
}

void BatchProcess::Process()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // Load transformation matrix
    PointCloud *pointCloud;
    pointCloud = new PointCloud();
    int err = pointCloud->LoadMatrix(m_transformationFile.toUtf8());
    Matrix4f matrix = pointCloud->GetTransformation();
    delete pointCloud;

    // read all the ply files in m_sourceFolder and loop
    QDir dir = QDir(m_sourceFolder);
    QStringList nameFilters;
    nameFilters << "*.PLY" << "*.ply";
    QDir::Filters filter = QDir::Files;
    QFileInfoList info_list = dir.entryInfoList(nameFilters, filter, QDir::Name);

    QProgressDialog progressDialog("Batch Process Transformations...", "Abort", 0, info_list.count(), 0);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.show();

    for (int i = 0; i < info_list.count(); i++)
    {
        QApplication::processEvents();
        pointCloud = new PointCloud();
        err = pointCloud->ImportPLY(info_list.at(i).absoluteFilePath().toUtf8());
        if (err == 0)
        {
            pointCloud->SetTransformation(matrix);
            pointCloud->ApplyTransformation();
            QFileInfo outputFileInfo(m_destinationFolder, info_list.at(i).fileName());
            pointCloud->ExportPLY(outputFileInfo.absoluteFilePath().toUtf8());
        }
        delete pointCloud;
        progressDialog.setValue(i + 1);
        if (progressDialog.wasCanceled()) break;
    }

    QApplication::restoreOverrideCursor();
}
