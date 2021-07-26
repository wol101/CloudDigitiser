#ifndef SAMPLEBOXDATA_H
#define SAMPLEBOXDATA_H

#include "pipeline/math_3d.h"

class PointCloud;

class SampleBoxData
{
public:
    SampleBoxData();

    void Calculate();

    Vector3f median() const;
    void setMedian(const Vector3f &median);

    Vector3f mean() const;
    void setMean(const Vector3f &mean);

    Vector3f minCorner() const;
    void setMinCorner(const Vector3f &minCorner);

    Vector3f maxCorner() const;
    void setMaxCorner(const Vector3f &maxCorner);

    PointCloud *pointCloud() const;
    void setPointCloud(PointCloud *pointCloud);

    int nPoints() const;
    void setNPoints(int nPoints);

protected:
    Vector3f m_median;
    Vector3f m_mean;
    Vector3f m_minCorner;
    Vector3f m_maxCorner;
    int m_nPoints;
    PointCloud *m_pointCloud;
};

#endif // SAMPLEBOXDATA_H
