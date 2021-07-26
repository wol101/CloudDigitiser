#include <QDir>
#include <QApplication>
#include <QProgressDialog>
#include <QString>
#include <QtGlobal>

#include "ProcessSampleBox.h"
#include "SampleBoxData.h"
#include "PointCloud.h"
#include "pipeline/math_3d.h"

ProcessSampleBox::ProcessSampleBox()
{
    m_transformation.InitIdentity();
    m_minCorner = Vector3f(0,0,0);
    m_maxCorner = Vector3f(0,0,0);
    m_sampleBoxDataMap = 0;
}

void ProcessSampleBox::SetInputs(QString folder, Matrix4f transformation, Vector3f minCorner, Vector3f maxCorner, QMap<QString, SampleBoxData *> *sampleBoxDataMap)
{
    m_folder = folder;
    m_transformation = transformation;
    m_minCorner = minCorner;
    m_maxCorner = maxCorner;
    m_sampleBoxDataMap = sampleBoxDataMap;
    if (m_sampleBoxDataMap->size() > 0) // this function clears any existing data in sampleBoxDataMap
    {
        for (QMap<QString, SampleBoxData *>::const_iterator i = m_sampleBoxDataMap->constBegin(); i != m_sampleBoxDataMap->constEnd(); i++) delete i.value();
        m_sampleBoxDataMap->clear();
    }
}

void ProcessSampleBox::Process()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // read all the ply files in m_sourceFolder and loop
    QDir dir = QDir(m_folder);
    QStringList nameFilters;
    nameFilters << "*.PLY" << "*.ply";
    QDir::Filters filter = QDir::Files;
    QFileInfoList info_list = dir.entryInfoList(nameFilters, filter, QDir::Name);

    QProgressDialog progressDialog("Batch Process Sample Box...", "Abort", 0, info_list.count(), 0);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.show();

    SampleBoxData *sampleBoxData;
    PointCloud *pointCloud;
    int err;

    for (int i = 0; i < info_list.count(); i++)
    {
        QApplication::processEvents();
        pointCloud = new PointCloud();
        err = pointCloud->ImportPLY(info_list.at(i).absoluteFilePath().toUtf8());
        if (err == 0)
        {
            pointCloud->SetTransformation(m_transformation);
            pointCloud->ApplyTransformation();
            sampleBoxData = new SampleBoxData();
            sampleBoxData->setMinCorner(m_minCorner);
            sampleBoxData->setMaxCorner(m_maxCorner);
            sampleBoxData->setPointCloud(pointCloud);
            sampleBoxData->Calculate();
            m_sampleBoxDataMap->insert(info_list.at(i).absoluteFilePath(), sampleBoxData);
        }
        delete pointCloud;
        progressDialog.setValue(i + 1);
        if (progressDialog.wasCanceled()) break;
    }

    QApplication::restoreOverrideCursor();
}
