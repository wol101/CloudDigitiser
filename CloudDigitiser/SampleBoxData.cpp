#include <algorithm>

#include "SampleBoxData.h"
#include "PointCloud.h"

#define BOUNDS(x, lo, hi) ((x) >= (lo) && (x) <= (hi))

SampleBoxData::SampleBoxData()
{
    m_median = Vector3f(0, 0, 0);
    m_mean = Vector3f(0, 0, 0);
    m_minCorner = Vector3f(0, 0, 0);
    m_maxCorner = Vector3f(0, 0, 0);
    m_nPoints = 0;
    m_pointCloud = 0;
}

void SampleBoxData::Calculate()
{
    int maxPoints = m_pointCloud->GetNPoints();
    const Point *pointList = m_pointCloud->GetPointList();
    float *xPtr = new float[maxPoints];
    float *yPtr = new float[maxPoints];
    float *zPtr = new float[maxPoints];
    float xSum = 0;
    float ySum = 0;
    float zSum = 0;
    float x, y, z;
    m_nPoints = 0;
    for (int i = 0; i < maxPoints; i++)
    {
        if (BOUNDS(pointList[i].x, m_minCorner.x, m_maxCorner.x) && BOUNDS(pointList[i].y, m_minCorner.y, m_maxCorner.y) && BOUNDS(pointList[i].z, m_minCorner.z, m_maxCorner.z))
        {
            xPtr[m_nPoints] = pointList[i].x;
            yPtr[m_nPoints] = pointList[i].y;
            zPtr[m_nPoints] = pointList[i].z;
            xSum += pointList[i].x;
            ySum += pointList[i].y;
            zSum += pointList[i].z;
            m_nPoints++;
        }
    }
    if (m_nPoints > 0)
    {
        int n50 = m_nPoints / 2;
//        std::sort(xPtr, xPtr + nPoints);
//        std::sort(yPtr, yPtr + nPoints);
//        std::sort(zPtr, zPtr + nPoints);
        std::nth_element(xPtr, xPtr + n50, xPtr + m_nPoints); // nth_element only sorts as far as is needed so should be quicker
        std::nth_element(yPtr, yPtr + n50, yPtr + m_nPoints);
        std::nth_element(zPtr, zPtr + n50, zPtr + m_nPoints);
        m_median.x = xPtr[n50];
        m_median.y = yPtr[n50];
        m_median.z = zPtr[n50];
        m_mean.x = xSum / m_nPoints;
        m_mean.y = ySum / m_nPoints;
        m_mean.z = zSum / m_nPoints;
    }
    else
    {
        m_median.x = 0;
        m_median.y = 0;
        m_median.z = 0;
        m_mean.x = 0;
        m_mean.y = 0;
        m_mean.z = 0;
    }

    delete [] zPtr;
    delete [] yPtr;
    delete [] xPtr;
}

Vector3f SampleBoxData::median() const
{
    return m_median;
}

void SampleBoxData::setMedian(const Vector3f &median)
{
    m_median = median;
}

Vector3f SampleBoxData::mean() const
{
    return m_mean;
}

void SampleBoxData::setMean(const Vector3f &mean)
{
    m_mean = mean;
}

Vector3f SampleBoxData::minCorner() const
{
    return m_minCorner;
}

void SampleBoxData::setMinCorner(const Vector3f &minCorner)
{
    m_minCorner = minCorner;
}

Vector3f SampleBoxData::maxCorner() const
{
    return m_maxCorner;
}

void SampleBoxData::setMaxCorner(const Vector3f &maxCorner)
{
    m_maxCorner = maxCorner;
}

PointCloud *SampleBoxData::pointCloud() const
{
    return m_pointCloud;
}

void SampleBoxData::setPointCloud(PointCloud *pointCloud)
{
    m_pointCloud = pointCloud;
}

int SampleBoxData::nPoints() const
{
    return m_nPoints;
}

void SampleBoxData::setNPoints(int nPoints)
{
    m_nPoints = nPoints;
}
