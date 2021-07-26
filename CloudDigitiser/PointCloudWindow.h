#ifndef POINTCLOUDWINDOW_H
#define POINTCLOUDWINDOW_H

#include <QMap>

#include "IrrlichtWindow.h"

#include "Trackball.h"
#include "MeasurementData.h"
#include "StrokeFont.h"

class PointCloud;
class QMouseEvent;
class Preferences;
class PointCloudSceneNode;

class PointCloudWindow : public IrrlichtWindow
{
    Q_OBJECT
public:
    PointCloudWindow(QWindow *parent = 0);
    ~PointCloudWindow();

    enum ToolMode { rectangleSelect, sphereSelect, lineSelect, moveTool, nudgeTool };

    float GetWidth() { return m_Width; }
    Vector3f GetCentrePoint() { return m_CentrePoint; }
    Vector3f GetSampleBoxMinCorner() { return m_sampleBoxMinCorner; }
    Vector3f GetSampleBoxMaxCorner() { return m_sampleBoxMaxCorner; }
    bool GetSampleBoxValid() { return m_sampleBoxValid; }

    void SetCameraRotation(float x, float y, float z, float w);
    void SetCameraRotationAxisAngle(float ax, float ay, float az, float degrees); // note: Values in degrees
    void SetCentrePoint(const Vector3f &centrePoint);
    void SetMaxPointsToRender(int maxPoints) { m_MaxPointsToRender = maxPoints; }
    void SetMeasurementData(MeasurementData *measurementData) { m_MeasurementData = measurementData; }
    void SetPointCloud(PointCloud * pointCloud); // note: GLWidget does not own the PointCloud
    void SetToolMode(ToolMode toolMode);
    void SetWidth(float width); // note: Values in world units
    void SetSampleBox(const Vector3f &minCorner, const Vector3f &maxCorner);


    ToolMode GetToolMode() { return m_ToolMode; }

    void ApplySettings(const Preferences &preferences);
    void EnableTrackball(bool trackball) { m_TrackballEnable = trackball; }
    void UpdateModel(bool reloadBuffers);

    void initialiseScene();
    void updateCamera();

    virtual void render();

    static void SetMeshColour(irr::scene::IMesh *mesh, const irr::video::SColor &colour);

    void BuildFittedPointSceneNode(const irr::core::vector3df &centre, float radius);
    void BuildFittedLineSceneNode(const irr::core::vector3df &lineStart, const irr::core::vector3df &lineEnd);
    void BuildFittedPlaneSceneNode(const irr::core::vector3df &planeCentre, const irr::core::vector3df &normal, float size);
    void BuildMeasurementPointSceneNode(const irr::core::vector3df &centre, float screenRadius, const Matrix4f &worldTrans, const QString &text);

signals:
    void EmitUpdateModel(bool reloadBuffers);
    void EmitStatusString(QString s);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent * event);
    void keyPressEvent(QKeyEvent *e);

    void BuildPointCloudSceneNode();
    void BuildAxesSceneNode();
    void BuildSampleBoxSceneNode();


    PointCloud *m_PointCloud;
    Matrix4f m_WorldTrans;
    Matrix4f m_WorldTransInverse;
    Vector3f m_StartSelect;
    Vector3f m_EndSelect;
    bool m_ActiveSelect;
    Vector3f m_MinBound;
    Vector3f m_MaxBound;
    float m_Width;
    float m_XYAspect;
    Vector3f m_CentrePoint;
    Quaternion m_CameraRotation;
    float m_CameraDistance;

    Trackball m_Trackball;
    bool m_TrackballEnable;
    ToolMode m_ToolMode;
    float m_PixelSelectionDistance;
    float m_WorldSelectionDistance;
    Vector4f m_StartMove;
    Vector3f m_StartCentrePoint;

    QCursor m_Cursor[16];
    MeasurementData *m_MeasurementData;

    QColor m_BackgroundColour;
    QColor m_SelectionColour;
    QColor m_LabelColour;
    QColor m_GridColour;
    float m_NudgeAngle;
    bool m_PixelsForSelection;

    StrokeFont m_StrokeFont;

    int m_MaxPointsToRender;
    int m_NPointsToRender;

    Vector3f m_sampleBoxMinCorner;
    Vector3f m_sampleBoxMaxCorner;
    bool m_sampleBoxValid;

    PointCloudSceneNode *m_pointCloudSceneNode;
    irr::scene::ICameraSceneNode* m_camera;
    irr::scene::IMeshSceneNode* m_axisSceneNode;
    irr::scene::IMeshSceneNode* m_fittedPointSceneNode;
    irr::scene::IMeshSceneNode* m_fittedLineSceneNode;
    irr::scene::IMeshSceneNode* m_fittedPlaneSceneNode;
    irr::scene::IMeshSceneNode* m_sampleBoxSceneNode;
    float m_AxisSize;
    float m_fittedPointRadius;
    float m_fittedLineLength;
    float m_fittedPlaneSize;
    int m_defaultTesselation;
    QColor m_PointFitColour;
    QColor m_LineFitColour;
    QColor m_PlaneFitColour;
    bool m_useArrow;
    float ambientProportion;
    float diffuseProportion;
    float specularProportion;
    float shininess;

    bool m_wireFrame;
    bool m_boundingBox;
    bool m_boundingBoxBuffers;
    bool m_normals;
    bool m_halfTransparency;

    irr::core::matrix4 m_projectionMatrix;
    irr::core::matrix4 m_viewMatrix;

    QMap<QString, irr::scene::IMeshSceneNode*> m_measurementPointSceneNodeMap;
    QColor m_BuildMeasurementColour;
};

#endif // POINTCLOUDWINDOW_H

