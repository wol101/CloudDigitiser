#ifndef PROCESSSAMPLEBOX_H
#define PROCESSSAMPLEBOX_H

#include <QMap>
#include <QString>
#include "pipeline/math_3d.h"

class SampleBoxData;

class ProcessSampleBox
{
public:
    ProcessSampleBox();

    void SetInputs(QString folder, Matrix4f transformation, Vector3f minCorner, Vector3f maxCorner, QMap<QString, SampleBoxData *> *sampleBoxDataMap);
    void Process();

private:
    QString m_folder;
    Matrix4f m_transformation;
    Vector3f m_minCorner;
    Vector3f m_maxCorner;
    QMap<QString, SampleBoxData *> *m_sampleBoxDataMap;
};

#endif // PROCESSSAMPLEBOX_H
