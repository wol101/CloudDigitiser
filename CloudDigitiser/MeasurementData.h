#ifndef MEASUREMENTDATA_H
#define MEASUREMENTDATA_H

#include <QMap>
#include <QString>
#include <stdio.h>
#include "pipeline/math_3d.h"

class MeasurementData
{
public:
    MeasurementData();

    void Insert(const QString &code, float x, float y, float z);
    void Delete(const QString &code);
    int Write(FILE *fout);
    QMap<QString, Vector3f> *GetPoints() { return &m_Points; }
    bool GetChanged() { return m_Changed; }
    void SetChanged(bool changed) { m_Changed = changed; }


protected:
    QMap<QString, Vector3f> m_Points;
    bool m_Changed;
};

#endif // MEASUREMENTDATA_H
