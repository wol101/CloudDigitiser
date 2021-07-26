#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "PointCloud.h"
#include "DataFile/DataFile.h"
#include "octree/octree.h"
#include "pipeline/math_3d.h"
#include "geometric_tools/Wm5ApprLineFit3.h"
#include "geometric_tools/Wm5ApprPlaneFit3.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define BOUNDS(x, lo, hi) ((x) >= (lo) && (x) <= (hi))
#define SQUARE(x) ((x) * (x))

PointCloud::PointCloud()
{
    m_PointList = 0;
    m_NPoints = 0;
    m_PointOctree = 0;
    m_MinBound = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    m_MaxBound = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    m_SelectionMinBound = m_MinBound;
    m_SelectionMaxBound = m_MaxBound;
    m_OctreeSize = 8192;
    m_OctreeIndexEpsilon = 1e-6;
    m_XDel = 0;
    m_YDel = 0;
    m_ZDel = 0;
    m_UseOctree = false;
    m_LastVector = Vector3f(1.f, 0.f, 0.f);
    m_Transformation.InitIdentity();
    m_LastPoint = Vector3f(0.f, 0.f, 0.f);
    m_NSelected = 0;
    m_lineFitted = false;
    m_pointFitted = false;
}

PointCloud::~PointCloud()
{
    if (m_PointList) delete [] m_PointList;
    if (m_PointOctree) delete m_PointOctree;
}

int PointCloud::ImportPLY(const char *filename)
{
    enum FileType { ascii, binary_little_endian, binary_big_endian };
    const int k_lineSize = 1024;
    char line[k_lineSize];
    char *ptrs[16];
    char *ptr;
    int count, i, j;
    FileType fileType = ascii;
    int offset;
    int xOffset = 0, yOffset = 0, zOffset = 0, xSize = 0, ySize = 0, zSize = 0;
    int nxOffset = 0, nyOffset = 0, nzOffset = 0, nxSize = 0, nySize = 0, nzSize = 0;
    int diffuseRedOffset = 0, diffuseGreenOffset = 0, diffuseBlueOffset = 0, diffuseRedSize = 0, diffuseGreenSize = 0, diffuseBlueSize = 0;
    double dValue;
    int iValue;
    int ix, iy, iz;

    DataFile plyFile;
    if (plyFile.ReadFile(filename)) return __LINE__;

    // read the PLY header

    // magic number
    if (plyFile.ReadNextLine2(line, k_lineSize, true, "comment")) return __LINE__;
    if (strcmp(line, "ply")) return __LINE__;

    // ascii or binary
    if (plyFile.ReadNextLine2(line, k_lineSize, true, "comment")) return __LINE__;
    count = DataFile::ReturnTokens(line, ptrs, 16);
    if (count != 3) return __LINE__;

    if (strcmp(ptrs[1], "ascii") == 0) fileType = ascii;
    else if (strcmp(ptrs[1], "binary_little_endian") == 0) fileType = binary_little_endian;
    else if (strcmp(ptrs[1], "binary_big_endian") == 0) fileType = binary_big_endian;
    else return __LINE__;

    // vertices
    if (plyFile.ReadNextLine2(line, k_lineSize, true, "comment")) return __LINE__;
    count = DataFile::ReturnTokens(line, ptrs, 16);
    if (count != 3) return __LINE__;
    if (strcmp(ptrs[0], "element")) return __LINE__;
    if (strcmp(ptrs[1], "vertex")) return __LINE__;
    count = strtol(ptrs[2], 0, 10);
    if (count < 0) return __LINE__;
    m_NPoints = count;

    // properties
    offset = 0;
    while (1)
    {
        if (plyFile.ReadNextLine2(line, k_lineSize, true, "comment")) return __LINE__;
        count = DataFile::ReturnTokens(line, ptrs, 16);
        if (count != 3) break;
        if (strcmp(ptrs[0], "property")) break;

        if (strcmp(ptrs[2], "x") == 0) { xOffset = offset; xSize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "y") == 0) { yOffset = offset; ySize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "z") == 0) { zOffset = offset; zSize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "nx") == 0) { nxOffset = offset; nxSize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "ny") == 0) { nyOffset = offset; nySize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "nz") == 0) { nzOffset = offset; nzSize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "diffuse_red") == 0 || strcmp(ptrs[2], "red") == 0) { diffuseRedOffset = offset; diffuseRedSize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "diffuse_green") == 0 || strcmp(ptrs[2], "green") == 0) { diffuseGreenOffset = offset; diffuseGreenSize = TypeSize(ptrs[1]); }
        else if (strcmp(ptrs[2], "diffuse_blue") == 0 || strcmp(ptrs[2], "blue") == 0) { diffuseBlueOffset = offset; diffuseBlueSize = TypeSize(ptrs[1]); }

        if (fileType == ascii)
        {
            offset++;
        }
        else
        {
            offset += TypeSize(ptrs[1]);
        }
    }

    // fortunately we aren't interested in anything else in the header
    while (strcmp(line, "end_header"))
    {
        if (plyFile.ReadNextLine2(line, k_lineSize, true, "comment")) return __LINE__;
    }

    if (m_PointList) delete [] m_PointList;
    m_PointList = new Point[m_NPoints];

    if (fileType == ascii)
    {
        for (i = 0; i < m_NPoints; i++)
        {
            for (j = 0; j < offset; j++)
            {
                if (j == xOffset)
                {
                    if (plyFile.ReadNext(&dValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].x = (float)dValue;
                }
                else if (j == yOffset)
                {
                    if (plyFile.ReadNext(&dValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].y = (float)dValue;
                }
                else if (j == zOffset)
                {
                    if (plyFile.ReadNext(&dValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].z = (float)dValue;
                }
                else if (j == nxOffset)
                {
                    if (plyFile.ReadNext(&dValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].nx = (float)dValue;
                }
                else if (j == nyOffset)
                {
                    if (plyFile.ReadNext(&dValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].ny = (float)dValue;
                }
                else if (j == nzOffset)
                {
                    if (plyFile.ReadNext(&dValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].nz = (float)dValue;
                }
                else if (j == diffuseRedOffset)
                {
                    if (plyFile.ReadNext(&iValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].r = iValue;
                }
                else if (j == diffuseGreenOffset)
                {
                    if (plyFile.ReadNext(&iValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].g = iValue;
                }
                else if (j == diffuseBlueOffset)
                {
                    if (plyFile.ReadNext(&iValue)) { delete [] m_PointList; return __LINE__; }
                    m_PointList[i].b = iValue;
                }
            }
            m_PointList[i].a = unselected;
        }
    }
    else
    {
        ptr = plyFile.GetIndex();
        if (xSize == 4 && nxSize == 4 && diffuseRedSize == 1)
        {
            for (i = 0; i < m_NPoints; i++)
            {
                m_PointList[i].x = *((float *)(ptr + xOffset));
                m_PointList[i].y = *((float *)(ptr + yOffset));
                m_PointList[i].z = *((float *)(ptr + zOffset));
                m_PointList[i].nx = *((float *)(ptr + nxOffset));
                m_PointList[i].ny = *((float *)(ptr + nyOffset));
                m_PointList[i].nz = *((float *)(ptr + nzOffset));
                m_PointList[i].r = *((unsigned char *)(ptr + diffuseRedOffset));
                m_PointList[i].g = *((unsigned char *)(ptr + diffuseGreenOffset));
                m_PointList[i].b = *((unsigned char *)(ptr + diffuseBlueOffset));
                m_PointList[i].a = 255;
                ptr += offset;
                if ((ptr - plyFile.GetRawData()) > static_cast<ptrdiff_t>(plyFile.GetSize())) { delete [] m_PointList; return __LINE__; }
            }
        }
        else if (xSize == 8 && nxSize == 8 && diffuseRedSize == 1)
        {
            for (i = 0; i < m_NPoints; i++)
            {
                m_PointList[i].x = (float) *((double *)(ptr + xOffset));
                m_PointList[i].y = (float) *((double *)(ptr + yOffset));
                m_PointList[i].z = (float) *((double *)(ptr + zOffset));
                m_PointList[i].nx = (float) *((double *)(ptr + xOffset));
                m_PointList[i].ny = (float) *((double *)(ptr + yOffset));
                m_PointList[i].nz = (float) *((double *)(ptr + zOffset));
                m_PointList[i].r = *((unsigned char *)(ptr + diffuseRedOffset));
                m_PointList[i].g = *((unsigned char *)(ptr + diffuseGreenOffset));
                m_PointList[i].b = *((unsigned char *)(ptr + diffuseBlueOffset));
                m_PointList[i].a = 255;
                ptr += offset;
                if ((ptr - plyFile.GetRawData()) > static_cast<ptrdiff_t>(plyFile.GetSize())) { delete [] m_PointList; return __LINE__; }
            }
        }
    }

    m_MinBound = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    m_MaxBound = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (i = 0; i < m_NPoints; i++)
    {
        if (m_PointList[i].x > m_MaxBound.x) m_MaxBound.x = m_PointList[i].x;
        if (m_PointList[i].x < m_MinBound.x) m_MinBound.x = m_PointList[i].x;
        if (m_PointList[i].y > m_MaxBound.y) m_MaxBound.y = m_PointList[i].y;
        if (m_PointList[i].y < m_MinBound.y) m_MinBound.y = m_PointList[i].y;
        if (m_PointList[i].z > m_MaxBound.z) m_MaxBound.z = m_PointList[i].z;
        if (m_PointList[i].z < m_MinBound.z) m_MinBound.z = m_PointList[i].z;
   }

    // now add the vertices to the octree
    if (m_UseOctree)
    {
        m_XDel = (m_MaxBound.x - m_MinBound.x) / ((double)m_OctreeSize - m_OctreeIndexEpsilon);
        m_YDel = (m_MaxBound.y - m_MinBound.y) / ((double)m_OctreeSize - m_OctreeIndexEpsilon);
        m_ZDel = (m_MaxBound.z - m_MinBound.z) / ((double)m_OctreeSize - m_OctreeIndexEpsilon);

        m_PointOctree = new Octree<int>(m_OctreeSize);
        for (i = 0; i < m_NPoints; i++)
        {
            ix = (int)((m_PointList[i].x - m_MinBound.x) / m_XDel);
            iy = (int)((m_PointList[i].y - m_MinBound.y) / m_YDel);
            iz = (int)((m_PointList[i].z - m_MinBound.z) / m_ZDel);
            m_PointOctree->set(ix, iy, iz, i);
        }
    }

    return 0;
}

int PointCloud::ExportPLY(const char *filename)
{
    FILE *fout = fopen(filename, "wb");
    if (fout == 0) return __LINE__;

    // write the PLY header

    // magic number
    fprintf(fout, "ply\n");
    // ascii or binary
    fprintf(fout, "format binary_little_endian 1.0\n");
    // vertices
    fprintf(fout, "element vertex %d\n", m_NPoints);
    fprintf(fout, "property float x\n");
    fprintf(fout, "property float y\n");
    fprintf(fout, "property float z\n");
    fprintf(fout, "property float nx\n");
    fprintf(fout, "property float ny\n");
    fprintf(fout, "property float nz\n");
    fprintf(fout, "property uchar diffuse_red\n");
    fprintf(fout, "property uchar diffuse_green\n");
    fprintf(fout, "property uchar diffuse_blue\n");
    fprintf(fout, "end_header\n");

    // now write the data (this could be buffered for better performance)
    int buffer_size = (6 * sizeof(float) + 3 * sizeof(unsigned char));
    unsigned char *buffer = new unsigned char[buffer_size];
    unsigned char *ptr;
    int i;
    size_t count;
    for (i = 0; i < m_NPoints; i++)
    {
        ptr = buffer;
        *((float *)ptr) = m_PointList[i].x;
        ptr += sizeof(float);
        *((float *)ptr) = m_PointList[i].y;
        ptr += sizeof(float);
        *((float *)ptr) = m_PointList[i].z;
        ptr += sizeof(float);
        *((float *)ptr) = m_PointList[i].nx;
        ptr += sizeof(float);
        *((float *)ptr) = m_PointList[i].ny;
        ptr += sizeof(float);
        *((float *)ptr) = m_PointList[i].nz;
        ptr += sizeof(float);
        *ptr = m_PointList[i].r;
        ptr++;
        *ptr = m_PointList[i].g;
        ptr++;
        *ptr = m_PointList[i].b;
        ptr++;
        count = fwrite(buffer, buffer_size, 1, fout);
        if (count != 1)
        {
            fclose(fout);
            delete [] buffer;
            return __LINE__;
        }
    }
    fclose(fout);
    delete [] buffer;
    return 0;
}

int PointCloud::TypeSize(const char *type)
{
    if (strcmp(type, "char") == 0) return 1;
    if (strcmp(type, "uchar") == 0) return 1;
    if (strcmp(type, "short") == 0) return 2;
    if (strcmp(type, "ushort") == 0) return 2;
    if (strcmp(type, "int") == 0) return 4;
    if (strcmp(type, "unit") == 0) return 4;
    if (strcmp(type, "float") == 0) return 4;
    if (strcmp(type, "double") == 0) return 8;
    return 0;
}


void PointCloud::SetAlpha(AlphaCodes alphaCode)
{
    unsigned char alpha = (unsigned char)alphaCode;
    int i;
    for (i = 0; i < m_NPoints; i++)
    {
        m_PointList[i].a = alpha;
    }
    if (alphaCode == PointCloud::selected) m_NSelected = m_NPoints;
    else m_NSelected = 0;
}

int PointCloud::SetAlphaBounded(const Vector3f &bound1, const Vector3f &bound2, const Matrix4f &worldTrans, AlphaCodes alphaCode)
{
    int i;
    float xl, xh, yl, yh, zl, zh;
    Vector4f v1, v2;
    Vector3f target;
    unsigned char alpha = (unsigned char)alphaCode;
    int count = 0;

    xl = MIN(bound1.x, bound2.x);
    yl = MIN(bound1.y, bound2.y);
    zl = MIN(bound1.z, bound2.z);
    xh = MAX(bound1.x, bound2.x);
    yh = MAX(bound1.y, bound2.y);
    zh = MAX(bound1.z, bound2.z);

    // this is quite slow because it transforms all points and checks them
    // it would probably be quicker to do a broad phase with a transformed bounding box
    // followed by a narrow phase of just the points that could possibly be affected
    Matrix4f modelWorldTrans = worldTrans * m_Transformation;
    for (i = 0; i < m_NPoints; i++)
    {
        v1 = Vector4f(m_PointList[i].x, m_PointList[i].y, m_PointList[i].z, 1.f);
        v2 = modelWorldTrans * v1;
        target.x = v2.x / v2.w;
        target.y = v2.y / v2.w;
        target.z = v2.z / v2.w;
        if (BOUNDS(target.x, xl, xh) && BOUNDS(target.y, yl, yh) && BOUNDS(target.z, zl, zh))
        {
            if (m_PointList[i].a != alpha)
            {
                if (alphaCode == PointCloud::selected) m_NSelected++;
                else if (m_PointList[i].a == (unsigned char)PointCloud::selected) m_NSelected--;
                m_PointList[i].a = alpha;
            }
            count++;
        }
    }
    return count;
 }

int PointCloud::SetAlphaLine(const Vector3f &startLine, const Vector3f &endLine, float screenRadius, const Matrix4f &worldTrans, AlphaCodes alphaCode, bool useScreenPixelRadius, bool ignoreZ)
{
    int i;
    int count = 0;
    Vector4f v1;
    unsigned char alpha = (unsigned char)alphaCode;
    Vector3f p, pLine, normal;
    Vector3f origin = startLine;
    Vector3f lineVec = (endLine - startLine);
    float useRadius2, lineDistance, distance2;
    if (useScreenPixelRadius) // this routine works in screen coordinates
    {
        useRadius2 = screenRadius * screenRadius;
    }
    else
    {
        // have to convert the radius to screen coordinates
        Vector4f origin4(0.f, 0.f, 0.f, 1.f);
        Vector4f radius4(screenRadius, 0.f, 0.f, 1.f);
        Vector3f transOrigin = (worldTrans * origin4).To3f();
        Vector3f transRadius = (worldTrans * radius4).To3f();
        Vector3f offset3 = transRadius - transOrigin;
        useRadius2 = offset3.Magnitude2();
    }

    if (ignoreZ) // ignore the z value and do an xy calculation
    {
        origin.z = 0.f;
        lineVec.z = 0.f;
    }
    float lineLength = lineVec.Magnitude();
    Vector3f lineVecNormalize = lineVec;
    lineVecNormalize.Normalize();

    // this is quite slow because it transforms all points and checks them
    // it would probably be quicker to do a broad phase with a transformed bounding box
    // followed by a narrow phase of just the points that could possibly be affected
    Matrix4f modelWorldTrans = worldTrans * m_Transformation;
    for (i = 0; i < m_NPoints; i++)
    {
        // convert to screen coordinates
        v1 = Vector4f(m_PointList[i].x, m_PointList[i].y, m_PointList[i].z, 1.f);
        p = (modelWorldTrans * v1).To3f();
        // convert to line coordinates
        if (ignoreZ) p.z = 0.f;
        pLine = p - origin;
        lineDistance = lineVecNormalize.Dot(pLine);
        normal = pLine - (lineDistance * lineVecNormalize);
        distance2 = normal.Magnitude2();
        if (distance2 <= useRadius2)
        {
            if (lineDistance >= 0 && lineDistance <= lineLength)
            {
                if (m_PointList[i].a != alpha)
                {
                    if (alphaCode == PointCloud::selected) m_NSelected++;
                    else if (m_PointList[i].a == (unsigned char)PointCloud::selected) m_NSelected--;
                    m_PointList[i].a = alpha;
                }
                count++;
            }
        }
    }
    return count;
}

int PointCloud::SetAlphaSphere(const Vector3f &screenSelect, float screenRadius, const Matrix4f &worldTrans, AlphaCodes alphaCode, bool useScreenPixelRadius)
{
    int count = 0;
    float distanceMetric;
    float minDistanceMetric = FLT_MAX;
    Point *ptr;
    Point *minDistancePtr;
    Vector3f v3;
    int i;
    unsigned char alpha = (unsigned char)alphaCode;

    // find closest point first
    // this is quite slow because it transforms all points and checks them
    // it would probably be quicker to do a broad phase with a transformed bounding box
    // followed by a narrow phase of just the points that could possibly be affected
    Matrix4f modelWorldTrans = worldTrans * m_Transformation;
    ptr = m_PointList;
    for (i = 0; i < m_NPoints; i++)
    {
        v3 = (modelWorldTrans * Vector4f(ptr->x, ptr->y, ptr->z, 1.f)).To3f();
        if (BOUNDS(v3.z, -1, 1))
        {
            distanceMetric = SQUARE(v3.x - screenSelect.x) + SQUARE(v3.y - screenSelect.y);
            if (distanceMetric < minDistanceMetric)
            {
                minDistanceMetric = distanceMetric;
                minDistancePtr = ptr;
            }
        }
        ptr++;
    }

    float radiusSquared;
    if (useScreenPixelRadius)
    {
        if (minDistanceMetric > SQUARE(screenRadius)) return 0;

        // now find the inverse transformed radius
        Vector4f origin(0.f, 0.f, 0.f, 1.f);
        Vector4f radius(screenRadius, 0.f, 0.f, 1.f);
        Matrix4f inverse = modelWorldTrans;
        inverse.Inverse();
        Vector3f transOrigin = (inverse * origin).To3f();
        Vector3f transRadius = (inverse * radius).To3f();
        Vector3f offset3 = transRadius - transOrigin;
        radiusSquared = offset3.Magnitude2();
    }
    else
    {
        radiusSquared = SQUARE(screenRadius);
    }

    // now select all the points within the radius
    ptr = m_PointList;
    for (i = 0; i < m_NPoints; i++)
    {
        v3 = Vector3f(ptr->x - minDistancePtr->x, ptr->y - minDistancePtr->y, ptr->z - minDistancePtr->z);
        if (v3.Magnitude2() <= radiusSquared)
        {
            if (ptr->a != alpha)
            {
                if (alphaCode == PointCloud::selected) m_NSelected++;
                else if (ptr->a == (unsigned char)PointCloud::selected) m_NSelected--;
                ptr->a = alpha;
            }
            count++;
        }
        ptr++;
    }
    return count;
}

void PointCloud::FindClosestPoint(const Vector3f &screenSelect, const Matrix4f &worldTrans, Vector3f *closestPoint)
{
    float distanceMetric;
    float minDistanceMetric = FLT_MAX;
    Point *ptr;
    Point *minDistancePtr;
    Vector3f v3;
    int i;

    if (m_NPoints == 0)
        return;

    // find closest point first
    // this is quite slow because it transforms all points and checks them
    // it would probably be quicker to do a broad phase with a transformed bounding box
    // followed by a narrow phase of just the points that could possibly be affected
    Matrix4f modelWorldTrans = worldTrans * m_Transformation;
    ptr = m_PointList;
    for (i = 0; i < m_NPoints; i++)
    {
        v3 = (modelWorldTrans * Vector4f(ptr->x, ptr->y, ptr->z, 1.f)).To3f();
        if (BOUNDS(v3.z, -1, 1))
        {
            distanceMetric = SQUARE(v3.x - screenSelect.x) + SQUARE(v3.y - screenSelect.y);
            if (distanceMetric < minDistanceMetric)
            {
                minDistanceMetric = distanceMetric;
                minDistancePtr = ptr;
            }
        }
        ptr++;
    }

    *closestPoint = (m_Transformation * Vector4f(minDistancePtr->x, minDistancePtr->y, minDistancePtr->z, 1.f)).To3f(); // output in world coordinates
    // *closestPoint = Vector3f(minDistancePtr->x, minDistancePtr->y, minDistancePtr->z);
}


int PointCloud::FitPointToSphere(const Vector3f &screenSelect, float screenRadius, const Matrix4f &worldTrans)
{
    float distanceMetric;
    float minDistanceMetric = FLT_MAX;
    Point *ptr;
    Point *minDistancePtr;
    Vector3f v3;
    int i;

    // find closest point first
    // this is quite slow because it transforms all points and checks them
    // it would probably be quicker to do a broad phase with a transformed bounding box
    // followed by a narrow phase of just the points that could possibly be affected
    Matrix4f modelWorldTrans = worldTrans * m_Transformation;
    ptr = m_PointList;
    for (i = 0; i < m_NPoints; i++)
    {
        v3 = (modelWorldTrans * Vector4f(ptr->x, ptr->y, ptr->z, 1.f)).To3f();
        if (BOUNDS(v3.z, -1, 1))
        {
            distanceMetric = SQUARE(v3.x - screenSelect.x) + SQUARE(v3.y - screenSelect.y);
            if (distanceMetric < minDistanceMetric)
            {
                minDistanceMetric = distanceMetric;
                minDistancePtr = ptr;
            }
        }
        ptr++;
    }
    if (minDistanceMetric > SQUARE(screenRadius)) return __LINE__;

    // now find the inverse transformed radius
    Vector4f origin(0.f, 0.f, 0.f, 1.f);
    Vector4f radius(screenRadius, 0.f, 0.f, 1.f);
    Matrix4f inverse = modelWorldTrans;
    inverse.Inverse();
    Vector3f transOrigin = (inverse * origin).To3f();
    Vector3f transRadius = (inverse * radius).To3f();
    Vector3f offset3 = transRadius - transOrigin;
    float radiusSquared = offset3.Magnitude2();

    // now sum all the points within the radius
    float xSum = 0, ySum = 0, zSum = 0;
    int numPoints = 0;
    ptr = m_PointList;
    for (i = 0; i < m_NPoints; i++)
    {
        v3 = Vector3f(ptr->x - minDistancePtr->x, ptr->y - minDistancePtr->y, ptr->z - minDistancePtr->z);
        if (v3.Magnitude2() <= radiusSquared)
        {
            numPoints++;
            xSum += ptr->x;
            ySum += ptr->y;
            zSum += ptr->z;
        }
        ptr++;
    }

    Vector4f point(0.f, 0.f, 0.f, 1.f);
    point.x = xSum / numPoints;
    point.y = ySum / numPoints;
    point.z = zSum / numPoints;
    point = m_Transformation * point;
    m_LastPoint = point.To3f();

    return 0;
}

int PointCloud::FitLineToAlpha(AlphaCodes oldAlphaCode, AlphaCodes newAlphaCode)
{
    int numPoints = 0;
    int i;
    unsigned char oldAlpha = (unsigned char)oldAlphaCode;
    unsigned char newAlpha = (unsigned char)newAlphaCode;

    m_SelectionMinBound = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    m_SelectionMaxBound = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (i = 0; i < m_NPoints; i++)
    {
        if (m_PointList[i].a == oldAlpha)
        {
            numPoints++;
            if (m_PointList[i].x > m_SelectionMaxBound.x) m_SelectionMaxBound.x = m_PointList[i].x;
            if (m_PointList[i].x < m_SelectionMinBound.x) m_SelectionMinBound.x = m_PointList[i].x;
            if (m_PointList[i].y > m_SelectionMaxBound.y) m_SelectionMaxBound.y = m_PointList[i].y;
            if (m_PointList[i].y < m_SelectionMinBound.y) m_SelectionMinBound.y = m_PointList[i].y;
            if (m_PointList[i].z > m_SelectionMaxBound.z) m_SelectionMaxBound.z = m_PointList[i].z;
            if (m_PointList[i].z < m_SelectionMinBound.z) m_SelectionMinBound.z = m_PointList[i].z;
        }
    }
    if (numPoints < 2) return __LINE__;

    Wm5::Vector3<float> *points = new Wm5::Vector3<float>[numPoints];
    numPoints = 0;
    for (i = 0; i < m_NPoints; i++)
    {
        if (m_PointList[i].a == oldAlpha)
        {
            Vector4f point(m_PointList[i].x, m_PointList[i].y, m_PointList[i].z, 1.f);
            point = m_Transformation * point;
            points[numPoints] = Wm5::Vector3<float>(point.x / point.w, point.y / point.w, point.z / point.w);
            numPoints++;
            m_PointList[i].a = newAlpha;
        }
    }

    m_bestFitLine = OrthogonalLineFit3(numPoints, points);
    m_LastVector.x = m_bestFitLine.Direction[0];
    m_LastVector.y = m_bestFitLine.Direction[1];
    m_LastVector.z = m_bestFitLine.Direction[2];
    m_LastPoint.x = m_bestFitLine.Origin[0];
    m_LastPoint.y = m_bestFitLine.Origin[1];
    m_LastPoint.z = m_bestFitLine.Origin[2];
    if (newAlpha != PointCloud::selected) m_NSelected = 0;
    m_pointFitted = true;
    m_lineFitted = true;

    delete [] points;
    return 0;
}

int PointCloud::FitPlaneToAlpha(AlphaCodes oldAlphaCode, AlphaCodes newAlphaCode)
{
    int numPoints = 0;
    int i;
    float xSum = 0, ySum = 0, zSum = 0;
    unsigned char oldAlpha = (unsigned char)oldAlphaCode;
    unsigned char newAlpha = (unsigned char)newAlphaCode;

    m_SelectionMinBound = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    m_SelectionMaxBound = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (i = 0; i < m_NPoints; i++)
    {
        if (m_PointList[i].a == oldAlpha)
        {
            numPoints++;
            if (m_PointList[i].x > m_SelectionMaxBound.x) m_SelectionMaxBound.x = m_PointList[i].x;
            if (m_PointList[i].x < m_SelectionMinBound.x) m_SelectionMinBound.x = m_PointList[i].x;
            if (m_PointList[i].y > m_SelectionMaxBound.y) m_SelectionMaxBound.y = m_PointList[i].y;
            if (m_PointList[i].y < m_SelectionMinBound.y) m_SelectionMinBound.y = m_PointList[i].y;
            if (m_PointList[i].z > m_SelectionMaxBound.z) m_SelectionMaxBound.z = m_PointList[i].z;
            if (m_PointList[i].z < m_SelectionMinBound.z) m_SelectionMinBound.z = m_PointList[i].z;
            xSum += m_PointList[i].x;
            ySum += m_PointList[i].y;
            zSum += m_PointList[i].z;
        }
    }
    if (numPoints < 3) return __LINE__;

    Vector4f point(0.f, 0.f, 0.f, 1.f);
    point.x = xSum / numPoints;
    point.y = ySum / numPoints;
    point.z = zSum / numPoints;
    point = m_Transformation * point;
    m_LastPoint = point.To3f();

    Wm5::Vector3<float> *points = new Wm5::Vector3<float>[numPoints];
    numPoints = 0;
    for (i = 0; i < m_NPoints; i++)
    {
        if (m_PointList[i].a == oldAlpha)
        {
            Vector4f point(m_PointList[i].x, m_PointList[i].y, m_PointList[i].z, 1.f);
            point = m_Transformation * point;
            points[numPoints] = Wm5::Vector3<float>(point.x / point.w, point.y / point.w, point.z / point.w);
            numPoints++;
            m_PointList[i].a = newAlpha;
        }
    }

    m_BestFitPlane = OrthogonalPlaneFit3(numPoints, points);
    m_LastVector.x = m_BestFitPlane.Normal[0];
    m_LastVector.y = m_BestFitPlane.Normal[1];
    m_LastVector.z = m_BestFitPlane.Normal[2];
    if (newAlpha != PointCloud::selected) m_NSelected = 0;
    m_pointFitted = true;
    m_lineFitted = true;

    delete [] points;
    return 0;
}

int PointCloud::FitPointToAlpha(AlphaCodes oldAlphaCode, AlphaCodes newAlphaCode)
{
    float xSum = 0, ySum = 0, zSum = 0;
    int i, numPoints = 0;
    unsigned char oldAlpha = (unsigned char)oldAlphaCode;
    unsigned char newAlpha = (unsigned char)newAlphaCode;

    m_SelectionMinBound = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    m_SelectionMaxBound = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (i = 0; i < m_NPoints; i++)
    {
        if (m_PointList[i].a == oldAlpha)
        {
            numPoints++;
            if (m_PointList[i].x > m_SelectionMaxBound.x) m_SelectionMaxBound.x = m_PointList[i].x;
            if (m_PointList[i].x < m_SelectionMinBound.x) m_SelectionMinBound.x = m_PointList[i].x;
            if (m_PointList[i].y > m_SelectionMaxBound.y) m_SelectionMaxBound.y = m_PointList[i].y;
            if (m_PointList[i].y < m_SelectionMinBound.y) m_SelectionMinBound.y = m_PointList[i].y;
            if (m_PointList[i].z > m_SelectionMaxBound.z) m_SelectionMaxBound.z = m_PointList[i].z;
            if (m_PointList[i].z < m_SelectionMinBound.z) m_SelectionMinBound.z = m_PointList[i].z;
            xSum += m_PointList[i].x;
            ySum += m_PointList[i].y;
            zSum += m_PointList[i].z;
            m_PointList[i].a = newAlpha;
        }
    }
    if (numPoints < 1) return __LINE__;

    Vector4f point(0.f, 0.f, 0.f, 1.f);
    point.x = xSum / numPoints;
    point.y = ySum / numPoints;
    point.z = zSum / numPoints;
    point = m_Transformation * point;
    m_LastPoint = point.To3f();
    if (newAlpha != PointCloud::selected) m_NSelected = 0;
    m_pointFitted = true;
    return 0;
}

// this is the shortest path rotation
void PointCloud::RotateToVector(float x, float y, float z)
{
    Vector3f v2(x, y, z);
    Vector3f v;
    Quaternion q;
    const float epsilon = 1e-6f;

    q.w = sqrtf(m_LastVector.Magnitude2() * v2.Magnitude2()) + m_LastVector.Dot(v2);
    if (q.w < epsilon) // this only occurs if a 180 degree rotation is needed
    {
        Vector3f perp; // this is a perpendicular vector (m_LastVector dot perp = 0)
        if (fabs(m_LastVector.z) > epsilon) perp = Vector3f(0, -m_LastVector.z, m_LastVector.y);
        else perp = Vector3f(-m_LastVector.y, m_LastVector.x, 0);
        q = MakeQuaternionFromAxisAngle(perp.x, perp.y, perp.z, 180);
    }
    else
    {
        v = m_LastVector.Cross(v2);
        q.x = v.x;
        q.y = v.y;
        q.z = v.z;
        q.Normalize();
    }

    RotateByQuaternion(q.x, q.y, q.z, q.w);
}

// this one rotates about the given axis
void PointCloud::RotateToVector(float x, float y, float z, float ax, float ay, float az)
{
    Vector3f target(x, y, z);
    Vector3f axis(ax, ay, az);

    // need to find the angle between two planes from their normals
    Vector3f n1 = m_LastVector.Cross(axis);
    Vector3f n2 = target.Cross(axis);

    // general formula for angle between 2 vectors (Va, Vb) in plane with known normal (Vn)
    // note: '/ ( |Va| * |Vb| )' can be removed since it cancels out in atan2
    // sina = |Va x Vb| / ( |Va| * |Vb| )
    // cosa = (Va . Vb) / ( |Va| * |Vb| )
    // angle = atan2( sina, cosa )
    // sign = Vn . ( Va x Vb )
    // if(sign<0) angle=-angle

    float sina = n1.Cross(n2).Magnitude();
    float cosa = n1.Dot(n2);
    float angle = atan2(sina, cosa);
    float sign = axis.Dot(n1.Cross(n2));
    if (sign < 0)
        angle = -angle;

    Quaternion q = MakeQuaternionFromAxisAngle(axis.x, axis.y, axis.z, ToDegree(angle));
    RotateByQuaternion(q.x, q.y, q.z, q.w);
}

void PointCloud::RotateByAxisAngle(float x, float y, float z, float degrees)
{
    Quaternion q = MakeQuaternionFromAxisAngle(x, y, z, degrees);
    RotateByQuaternion(q.x, q.y, q.z, q.w);
}

void PointCloud::RotateByQuaternion(float x, float y, float z, float w)
{
    Quaternion q;
    Matrix4f m;
    q.x = x;
    q.y = y;
    q.z = z;
    q.w = w;

//    m_MinBound = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
//    m_MaxBound = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
//    Vector3f v;

//    for (int i = 0; i < m_NPoints; i++)
//    {
//        v = QVRotate(q, Vector3f(m_PointList[i].x, m_PointList[i].y, m_PointList[i].z));
//        m_PointList[i].x = v.x;
//        m_PointList[i].y = v.y;
//        m_PointList[i].z = v.z;

//        if (m_PointList[i].x > m_MaxBound.x) m_MaxBound.x = m_PointList[i].x;
//        if (m_PointList[i].x < m_MinBound.x) m_MinBound.x = m_PointList[i].x;
//        if (m_PointList[i].y > m_MaxBound.y) m_MaxBound.y = m_PointList[i].y;
//        if (m_PointList[i].y < m_MinBound.y) m_MinBound.y = m_PointList[i].y;
//        if (m_PointList[i].z > m_MaxBound.z) m_MaxBound.z = m_PointList[i].z;
//        if (m_PointList[i].z < m_MinBound.z) m_MinBound.z = m_PointList[i].z;
//    }

    m.InitQuaternion(q);
    m_Transformation = m * m_Transformation;
    //for (int i = 0 ; i < 4 ; i++) std::cerr << m_Transformation.m[i][0] << " " << m_Transformation.m[i][1] << " " << m_Transformation.m[i][2] << " " << m_Transformation.m[i][3] << "\n";
}

void PointCloud::Translate(float x, float y, float z)
{
    Matrix4f m;

//    for (int i = 0; i < m_NPoints; i++)
//    {
//        m_PointList[i].x += x;
//        m_PointList[i].y += y;
//        m_PointList[i].z += z;
//    }

//    m_MinBound.x = m_MinBound.x + x;
//    m_MinBound.y = m_MinBound.y + y;
//    m_MinBound.z = m_MinBound.z + z;
//    m_MaxBound.x = m_MaxBound.x + x;
//    m_MaxBound.y = m_MaxBound.y + y;
//    m_MaxBound.z = m_MaxBound.z + z;

    m.InitTranslationTransform(x, y, z);
    m_Transformation = m * m_Transformation;
    //for (int i = 0 ; i < 4 ; i++) std::cerr << m_Transformation.m[i][0] << " " << m_Transformation.m[i][1] << " " << m_Transformation.m[i][2] << " " << m_Transformation.m[i][3] << "\n";
}

void PointCloud::Scale(float x, float y, float z)
{
    Matrix4f m;

//    for (int i = 0; i < m_NPoints; i++)
//    {
//        m_PointList[i].x *= x;
//        m_PointList[i].y *= y;
//        m_PointList[i].z *= z;
//    }

//    m_MinBound.x = m_MinBound.x * x;
//    m_MinBound.y = m_MinBound.y * y;
//    m_MinBound.z = m_MinBound.z * z;
//    m_MaxBound.x = m_MaxBound.x * x;
//    m_MaxBound.y = m_MaxBound.y * y;
//    m_MaxBound.z = m_MaxBound.z * z;

    m.InitScaleTransform(x, y, z);
    m_Transformation = m * m_Transformation;
    //for (int i = 0 ; i < 4 ; i++) std::cerr << m_Transformation.m[i][0] << " " << m_Transformation.m[i][1] << " " << m_Transformation.m[i][2] << " " << m_Transformation.m[i][3] << "\n";
}


void PointCloud::ApplyTransformation()
{
    Vector4f v;
    int i;
    Matrix4f normalTransformMatrix = m_Transformation;
    normalTransformMatrix.Inverse().Transpose();

    m_MinBound = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    m_MaxBound = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (i = 0; i < m_NPoints; i++)
    {
        v.x = m_PointList[i].x;
        v.y = m_PointList[i].y;
        v.z = m_PointList[i].z;
        v.w = 1.f;
        v = m_Transformation * v;
        m_PointList[i].x = v.x / v.w;
        m_PointList[i].y = v.y / v.w;
        m_PointList[i].z = v.z / v.w;
        // a bit more complicated for the normal
        // normalTransformed = transpose(inverse(M)) * normal
        // and the w is ignored because normals are only direction
        v.x = m_PointList[i].nx;
        v.y = m_PointList[i].ny;
        v.z = m_PointList[i].nz;
        v.w = 0.f;
        v = normalTransformMatrix * v;
        m_PointList[i].nx = v.x;
        m_PointList[i].ny = v.y;
        m_PointList[i].nz = v.z;

        if (m_PointList[i].x > m_MaxBound.x) m_MaxBound.x = m_PointList[i].x;
        if (m_PointList[i].x < m_MinBound.x) m_MinBound.x = m_PointList[i].x;
        if (m_PointList[i].y > m_MaxBound.y) m_MaxBound.y = m_PointList[i].y;
        if (m_PointList[i].y < m_MinBound.y) m_MinBound.y = m_PointList[i].y;
        if (m_PointList[i].z > m_MaxBound.z) m_MaxBound.z = m_PointList[i].z;
        if (m_PointList[i].z < m_MinBound.z) m_MinBound.z = m_PointList[i].z;
    }

    m_Transformation.InitIdentity();
    //for (int i = 0 ; i < 4 ; i++) std::cerr << m_Transformation.m[i][0] << " " << m_Transformation.m[i][1] << " " << m_Transformation.m[i][2] << " " << m_Transformation.m[i][3] << "\n";
}

int PointCloud::SaveMatrix(const char * filename)
{
    FILE *fout = fopen(filename, "wb");
    if (fout == 0) return __LINE__;

    for (int i = 0 ; i < 4 ; i++)
    {
        fprintf(fout, "%.7e %.7e %.7e %.7e\n", m_Transformation.m[i][0], m_Transformation.m[i][1], m_Transformation.m[i][2], m_Transformation.m[i][3]);
    }
    fclose(fout);
    return 0;
}

int PointCloud::LoadMatrix(const char * filename)
{
    const int k_lineSize = 1024;
    char line[k_lineSize];
    char *ptrs[16];
    int i, count;

    DataFile matrixFile;
    if (matrixFile.ReadFile(filename)) return __LINE__;

    for (i = 0 ; i < 4 ; i++)
    {
        matrixFile.ReadNextLine(line, k_lineSize, true, '#', '\\'); // allow comment lines and continuation characters
        count = DataFile::ReturnTokens(line, ptrs, 16);
        if (count < 4) return __LINE__;
        m_Transformation.m[i][0] = (float)strtod(ptrs[0], 0);
        m_Transformation.m[i][1] = (float)strtod(ptrs[1], 0);
        m_Transformation.m[i][2] = (float)strtod(ptrs[2], 0);
        m_Transformation.m[i][3] = (float)strtod(ptrs[3], 0);
    }

    return 0;
}



