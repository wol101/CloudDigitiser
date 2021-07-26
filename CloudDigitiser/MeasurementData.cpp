#include "MeasurementData.h"

MeasurementData::MeasurementData()
{
    m_Changed = false;
}

void MeasurementData::Insert(const QString &code, float x, float y, float z)
{
    m_Points[code] = Vector3f(x, y, z);
    m_Changed = true;
}

void MeasurementData::Delete(const QString &code)
{
    QMap<QString, Vector3f>::iterator i = m_Points.find(code);

    if (i != m_Points.end())
    {
        m_Points.erase(i);
        m_Changed = true;
    }
}

int MeasurementData::Write(FILE *fout)
{
    fprintf(fout, "%d\n", m_Points.size());
    QMap<QString, Vector3f>::const_iterator i = m_Points.constBegin();
    while (i != m_Points.constEnd())
    {
        fprintf(fout, "%s\t%g\t%g\t%g\n", i.key().toUtf8().constData(), i.value().x, i.value().y, i.value().z);
        ++i;
    }
    m_Changed = false;
    return 0;
}
