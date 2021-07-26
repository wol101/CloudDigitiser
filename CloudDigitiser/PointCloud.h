#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include "octree/octree.h"
#include "pipeline/math_3d.h"
#include "geometric_tools/Wm5ApprLineFit3.h"
#include "geometric_tools/Wm5ApprPlaneFit3.h"

struct Point
{
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

class PointCloud
{
public:
    PointCloud();
    ~PointCloud();

    enum AlphaCodes {selected = 31, unselected = 255};

    int ImportPLY(const char *filename);
    int ExportPLY(const char *filename);
    int SaveMatrix(const char * filename);
    int LoadMatrix(const char * filename);
    void SetTransformation(const Matrix4f &transformation) { m_Transformation = transformation; }
    void SetAlpha(AlphaCodes alphaCode);
    int SetAlphaBounded(const Vector3f &bound1, const Vector3f &bound2, const Matrix4f &worldTrans, AlphaCodes alphaCode);
    int SetAlphaLine(const Vector3f &startLine, const Vector3f &endLine, float screenRadius, const Matrix4f &worldTrans, AlphaCodes alphaCode, bool useScreenPixelRadius, bool ignoreZ);
    int SetAlphaSphere(const Vector3f &screenSelect, float screenRadius, const Matrix4f &worldTrans, AlphaCodes alphaCode, bool useScreenPixelRadius);
    int FitLineToAlpha(AlphaCodes oldAlphaCode, AlphaCodes newAlphaCode);
    int FitPlaneToAlpha(AlphaCodes oldAlphaCode, AlphaCodes newAlphaCode);
    int FitPointToAlpha(AlphaCodes oldAlphaCode, AlphaCodes newAlphaCode);
    int FitPointToSphere(const Vector3f &screenSelect, float screenRadius, const Matrix4f &worldTrans);
    void RotateToVector(float x, float y, float z);
    void RotateToVector(float x, float y, float z, float ax, float ay, float az);
    void RotateByQuaternion(float x, float y, float z, float w);
    void RotateByAxisAngle(float x, float y, float z, float degrees);
    void Translate(float x, float y, float z);
    void Scale(float x, float y, float z);
    void ApplyTransformation();
    void FindClosestPoint(const Vector3f &screenSelect, const Matrix4f &worldTrans, Vector3f *closestPoint);

    const Matrix4f &GetTransformation() { return m_Transformation; }
    const Vector3f &GetLastPoint() { return m_LastPoint; }
    const Vector3f &GetLastVector() { return m_LastVector; }
    const Point &GetPoint(int index) { return m_PointList[index]; }
    const Point *GetPointList() { return m_PointList; }
    int GetNPoints() { return m_NPoints; }
    int GetNSelected() { return m_NSelected; }
    const Vector3f &GetMinBound() { return m_MinBound; }
    const Vector3f &GetMaxBound() { return m_MaxBound; }
    bool GetLineFitted() { return m_lineFitted; }
    bool GetPointFitted() { return m_pointFitted; }
    const Vector3f &GetSelectionMinBound() { return m_SelectionMinBound; }
    const Vector3f &GetSelectionMaxBound() { return m_SelectionMaxBound; }

protected:
    static int TypeSize(const char *type);

    Point *m_PointList;
    int m_NPoints;
    int m_NSelected;
    Octree<int> *m_PointOctree;
    int m_OctreeSize;
    double m_OctreeIndexEpsilon;
    Vector3f m_MinBound;
    Vector3f m_MaxBound;
    Vector3f m_SelectionMinBound;
    Vector3f m_SelectionMaxBound;
    double m_XDel;
    double m_YDel;
    double m_ZDel;
    bool m_UseOctree;
    Wm5::Line3<float> m_bestFitLine;
    Wm5::Plane3<float> m_BestFitPlane;
    Vector3f m_LastPoint;
    Vector3f m_LastVector;
    Matrix4f m_Transformation;

    bool m_lineFitted;
    bool m_pointFitted;
};

#endif // POINTCLOUD_H
