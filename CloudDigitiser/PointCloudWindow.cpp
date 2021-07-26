#include "PointCloudWindow.h"
#include "irrlicht-1.9/source/CCameraSceneNode.h"
#include "PointCloud.h"
#include "PointCloudSceneNode.h"
#include "StrokeFont.h"
#include "Preferences.h"
#include "pipeline/pipeline.h"
#include "pipeline/math_3d.h"

#include <QMouseEvent>
#include <QBitmap>
#include <QPainter>
#include <QCursor>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QMessageBox>
#include <QString>

#include <float.h>
#include <iostream>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) >= 0 ? (a) : -(a))

PointCloudWindow::PointCloudWindow(QWindow *parent)
    : IrrlichtWindow(parent)
{
    m_PointCloud = 0;
    m_XYAspect = 1;
    m_StartSelect = Vector3f(0.f, 0.f, 0.f);
    m_EndSelect = Vector3f(0.f, 0.f, 0.f);
    m_ActiveSelect = false;
    m_WorldTrans.InitIdentity();
    m_CentrePoint = Vector3f(0.f, 0.f, 0.f);
    m_MinBound = Vector3f(-1.f, -1.f, -1.f);
    m_MaxBound = Vector3f(1.f, 1.f, 1.f);
    m_CameraRotation = Quaternion(0.f, 0.f, 0.f, 1.0f);
    m_Width = 100;
    m_CameraDistance = 1000;
    m_TrackballEnable = false;
    m_ToolMode = rectangleSelect;
    m_PixelSelectionDistance = 5;
    m_WorldSelectionDistance = 0.01;
    m_NudgeAngle = 0.1f;
    m_MeasurementData = 0;
    m_MaxPointsToRender = 10 * 1000 * 1000;
    m_NPointsToRender = 0;

    // setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    QBitmap cursor(32, 32); // recommended size
    QBitmap mask(32, 32); // recommended size
    QPoint centre(16, 16);
    QPainter cursorPaint(&cursor);
    QPainter maskPaint(&mask);
    for (int i = 0; i < 16; i++)
    {
        cursor.clear();
        mask.clear();
        cursorPaint.drawEllipse(centre, i, i);
        maskPaint.drawEllipse(centre, i, i);
        m_Cursor[i] = QCursor(cursor, mask, centre.x(), centre.y());
    }

    m_wireFrame = false;
    m_boundingBox = false;
    m_boundingBoxBuffers = false;
    m_normals = false;
    m_halfTransparency = false;
    m_camera = 0;
    m_fittedPointSceneNode = 0;
    m_fittedLineSceneNode = 0;
    m_fittedPlaneSceneNode = 0;
    m_sampleBoxSceneNode = 0;
    m_axisSceneNode = 0;
    m_pointCloudSceneNode = 0;
    m_fittedPointRadius = 0;
    m_fittedLineLength = 0;
    m_fittedPlaneSize = 0;
    m_AxisSize = 1;
    m_defaultTesselation = 64;
    m_useArrow = true;
    ambientProportion = 0.2f;
    diffuseProportion = 0.8f;
    specularProportion = 0.3f;
    shininess = 20;
    m_sampleBoxMinCorner = Vector3f(0, 0, 0);
    m_sampleBoxMaxCorner = Vector3f(0, 0, 0);
}

PointCloudWindow::~PointCloudWindow()
{
}

void PointCloudWindow::initialiseScene()
{
    if (sceneManager() == 0)
    {
        qWarning("Scene manager not initialised before calling initialiseScene");
        return;
    }
    sceneManager()->clear();
    m_pointCloudSceneNode = 0;
    m_camera = 0;
    m_axisSceneNode = 0;
    m_fittedPointSceneNode = 0;
    m_fittedLineSceneNode = 0;
    m_fittedPlaneSceneNode = 0;
    m_sampleBoxSceneNode = 0;

    irr::s32 state = irr::scene::EDS_OFF;
    if (m_boundingBox) state |= irr::scene::EDS_BBOX;
    if (m_boundingBoxBuffers) state |= irr::scene::EDS_BBOX_BUFFERS;
    if (m_wireFrame) state |= irr::scene::EDS_MESH_WIRE_OVERLAY;
    if (m_normals) state |= irr::scene::EDS_NORMALS;
    if (m_halfTransparency) state |= irr::scene::EDS_HALF_TRANSPARENCY;

    // create a default camera with an identity view matrix
    m_camera = sceneManager()->addCameraSceneNode(0, irr::core::vector3df(0, 0, 0), irr::core::vector3df(0, 0, 1));
    m_camera->setUpVector(irr::core::vector3df(0, 1, 0));
    sceneManager()->setActiveCamera(m_camera);

    // create some light
    sceneManager()->setAmbientLight(irr::video::SColorf(0.8f,0.8f,0.8f));
    irr::scene::ILightSceneNode *light = sceneManager()->addLightSceneNode(0, irr::core::vector3df(0, 0, 0), irr::video::SColorf(.8f, .8f, .8f), 1000.0f);
    light->setLightType(irr::video::ELT_DIRECTIONAL); // directional lights point (0,0,1) by default
    light->setRotation(irr::core::vector3df(-135, 45, 0)); // this should rotate it properly
}

void PointCloudWindow::updateCamera()
{
    irr::scene::CCameraSceneNode *camera = dynamic_cast<irr::scene::CCameraSceneNode *>(m_camera);
    if (camera == 0) return;

    // set the projection matrix
    m_XYAspect = (float)width() / (float)height();
    m_projectionMatrix.buildProjectionMatrixOrthoRH(m_Width, m_Width / m_XYAspect, 0, m_CameraDistance * 2);
    camera->setProjectionMatrix(m_projectionMatrix, true);

    // set the view matrix
    // Default Camera
    Vector3f cameraPos(0, 0, 0);
    Vector3f cameraDirection(0.f, 1.f, 0.f);
    Vector3f cameraUp(0.f, 0.f, 1.f);

    // move the camera as specified
    Quaternion cameraRotation;
    if (m_Trackball.GetActive() == false) cameraRotation = m_CameraRotation;
    else cameraRotation = m_Trackball.GetRotation() * m_CameraRotation;
    cameraDirection = QVRotate(cameraRotation, cameraDirection);
    cameraUp = QVRotate(cameraRotation, cameraUp);
    cameraPos = m_CentrePoint - (cameraDirection * m_CameraDistance);
    camera->setRHCoordinates(true);
    camera->setPosition(irr::core::vector3df(cameraPos.x, cameraPos.y, cameraPos.z));
    camera->updateAbsolutePosition();
    camera->setTarget(irr::core::vector3df(m_CentrePoint.x, m_CentrePoint.y, m_CentrePoint.z));
    camera->setUpVector(irr::core::vector3df(cameraUp.x, cameraUp.y, cameraUp.z));
    camera->updateMatrices();
    m_viewMatrix = camera->getViewMatrix();

    irr::core::matrix4 worldTrans = m_projectionMatrix * m_viewMatrix;
    // this is the right way round to convert from Irrlicht to math_3d
    // Matrix4f is [row][col]; irr::core::matrix4 is [col * 4 + row] (this is from looking at the code not the documentation)
    m_WorldTrans.m[0][0] = worldTrans[0]; m_WorldTrans.m[0][1] = worldTrans[4]; m_WorldTrans.m[0][2] = worldTrans[8];  m_WorldTrans.m[0][3] = worldTrans[12];
    m_WorldTrans.m[1][0] = worldTrans[1]; m_WorldTrans.m[1][1] = worldTrans[5]; m_WorldTrans.m[1][2] = worldTrans[9];  m_WorldTrans.m[1][3] = worldTrans[13];
    m_WorldTrans.m[2][0] = worldTrans[2]; m_WorldTrans.m[2][1] = worldTrans[6]; m_WorldTrans.m[2][2] = worldTrans[10]; m_WorldTrans.m[2][3] = worldTrans[14];
    m_WorldTrans.m[3][0] = worldTrans[3]; m_WorldTrans.m[3][1] = worldTrans[7]; m_WorldTrans.m[3][2] = worldTrans[11]; m_WorldTrans.m[3][3] = worldTrans[15];

//    m_WorldTrans = Matrix4f(worldTrans(0,0), worldTrans(1,0), worldTrans(2,0), worldTrans(3,0),
//                            worldTrans(0,1), worldTrans(1,1), worldTrans(2,1), worldTrans(3,1),
//                            worldTrans(0,2), worldTrans(1,2), worldTrans(2,2), worldTrans(3,2),
//                            worldTrans(0,3), worldTrans(1,3), worldTrans(2,3), worldTrans(3,3));

    // tests
//    irr::core::vector3df input(7, 8, 9);
//    irr::f32 result[4];
//    worldTrans.transformVect(result, input);
//    Vector4f input2(7, 8, 9, 1);
//    Vector4f result2 = m_WorldTrans * input2;
//    qDebug("irrlicht %g %g %g %g\n", result[0],  result[1],  result[2],  result[3]);
//    qDebug("pipeline %g %g %g %g\n", result2.x,  result2.y,  result2.z,  result2.w);

}

void PointCloudWindow::render()
{
    if (irrlichtDevice() && videoDriver() && sceneManager() && m_camera)
    {
        irrlichtDevice()->getTimer()->tick(); // need to do this by hand because we are not using the irr::IrrlichtDevice::run() method

        videoDriver()->beginScene(static_cast<irr::u16>(irr::video::ECBF_COLOR | irr::video::ECBF_DEPTH), backgroundColour());
        sceneManager()->drawAll();

        m_StrokeFont.ZeroLineBuffer();

        // now add the custom bits from the user interface

        if (m_ToolMode == nudgeTool)
        {
            float xTickInterval = 0.2f;
            float yTickInterval = xTickInterval * m_XYAspect;
            m_StrokeFont.SetRGBA(m_GridColour.red(), m_GridColour.green(), m_GridColour.blue(), m_GridColour.alpha());
            float tick = 0;
            while (tick < 1.0f)
            {
                m_StrokeFont.DrawLine(tick, -1.f, -0.99999f, tick, 1.f, -0.99999f);
                m_StrokeFont.DrawLine(-tick, -1.f, -0.99999f, -tick, 1.f, -0.99999f);
                tick += xTickInterval;
            }
            tick = 0;
            while (tick < 1.0f)
            {
                m_StrokeFont.DrawLine(-1.f, tick, -0.99999f, 1.f, tick, -0.99999f);
                m_StrokeFont.DrawLine(-1.f, -tick, -0.99999f, 1.f, -tick, -0.99999f);
                tick += yTickInterval;
            }
        }

        if (m_ActiveSelect)
        {
            if (m_ToolMode == rectangleSelect)
            {
                m_StrokeFont.SetRGBA(m_SelectionColour.red(), m_SelectionColour.green(), m_SelectionColour.blue(), m_SelectionColour.alpha());
                m_StrokeFont.DrawLine(m_StartSelect.x, m_StartSelect.y, -0.99999f, m_EndSelect.x, m_StartSelect.y, -0.99999f);
                m_StrokeFont.DrawLine(m_EndSelect.x, m_StartSelect.y, -0.99999f, m_EndSelect.x, m_EndSelect.y, -0.99999f);
                m_StrokeFont.DrawLine(m_EndSelect.x, m_EndSelect.y, -0.99999f, m_StartSelect.x, m_EndSelect.y, -0.9999f);
                m_StrokeFont.DrawLine(m_StartSelect.x, m_EndSelect.y, -0.9999f, m_StartSelect.x, m_StartSelect.y, -0.99999f);
            }
            else if (m_ToolMode == lineSelect)
            {
                m_StrokeFont.SetRGBA(m_SelectionColour.red(), m_SelectionColour.green(), m_SelectionColour.blue(), m_SelectionColour.alpha());
                m_StrokeFont.DrawLine(m_StartSelect.x, m_StartSelect.y, -0.99999f, m_EndSelect.x, m_EndSelect.y, -0.99999f);
            }
        }

        if (m_MeasurementData)
        {
            QMap<QString, Vector3f> *points = m_MeasurementData->GetPoints();
            if (points->size() > 0)
            {
                QMap<QString, Vector3f>::const_iterator iter = points->constBegin();
                m_StrokeFont.SetRGBA(m_LabelColour.red(), m_LabelColour.green(), m_LabelColour.blue(), m_LabelColour.alpha());
                Vector4f worldLocation, glLocation;
                m_StrokeFont.SetZ(-0.9999f);

                float cheight = 1.f / 20.f;
                float cwidth = cheight / m_XYAspect;
                float cheight2 = cheight / 2.f;
                float cwidth2 = cwidth / 2.f;
                while (iter != points->constEnd())
                {
                    worldLocation = Vector4f(iter.value().x, iter.value().y, iter.value().z, 1);
                    glLocation = m_WorldTrans * worldLocation;
                    m_StrokeFont.StrokeMarker(StrokeFont::XShape, glLocation.x / glLocation.w, glLocation.y / glLocation.w, cwidth2, cheight2);
                    m_StrokeFont.StrokeString(iter.key().toUtf8(), 1, cwidth2 + glLocation.x / glLocation.w, glLocation.y / glLocation.w, cwidth, cheight, 0, 1, 0);
                    ++iter;
                }
            }
        }

        // now draw the contents of m_StrokeFont
        irr::video::SMaterial lineMaterial;
        lineMaterial.Lighting = false;
        videoDriver()->setMaterial(lineMaterial);
        videoDriver()->setTransform(irr::video::ETS_VIEW, irr::core::IdentityMatrix);
        videoDriver()->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
        videoDriver()->setTransform(irr::video::ETS_PROJECTION, irr::core::IdentityMatrix);
        const irr::core::vector3df p1, p2;
        float *vertexPtr = m_StrokeFont.GetLineBuffer();
        unsigned char *colourPtr = m_StrokeFont.GetLineBufferVertexColours();
        for (int i = 0; i < m_StrokeFont.GetNumLines(); i++)
        {
            videoDriver()->draw3DLine(irr::core::vector3df(vertexPtr[0], vertexPtr[1], vertexPtr[2]), irr::core::vector3df(vertexPtr[3], vertexPtr[4], vertexPtr[5]), irr::video::SColor(colourPtr[3], colourPtr[0], colourPtr[1], colourPtr[2]));
            vertexPtr += 6;
            colourPtr += 8;
        }
        videoDriver()->endScene();
    }
}


void PointCloudWindow::mousePressEvent(QMouseEvent *event)
{
    if (m_PointCloud == 0) return;

    switch (m_ToolMode)
    {
    case rectangleSelect:
    case lineSelect:
        if (event->buttons() & Qt::LeftButton)
        {
            // scale to values between -1 and +1
            m_StartSelect.x = (2 * float(event->pos().x()) / float(width())) - 1.f;
            m_StartSelect.y = (2 * float(height() - event->pos().y()) / float(height())) - 1.f;
            m_EndSelect = m_StartSelect;
            m_ActiveSelect = true;
            renderLater();
        }
        break;

    case moveTool:
    case nudgeTool:
        if (event->buttons() & Qt::LeftButton && m_TrackballEnable && event->modifiers() == Qt::NoModifier)
        {
            setCursor(Qt::ClosedHandCursor);
            int trackballRadius;
            if (width() < height()) trackballRadius = (width() / 2.2);
            else trackballRadius = (height() / 2.2);
            Vector3f cameraDirection(0.f, 1.f, 0.f);
            Vector3f cameraUp(0.f, 0.f, 1.f);
            cameraDirection = QVRotate(m_CameraRotation, cameraDirection);
            cameraUp = QVRotate(m_CameraRotation, cameraUp);
            m_Trackball.StartTrackball(event->pos().x(), height() - event->pos().y(), width() / 2, height() / 2, trackballRadius, cameraUp, cameraDirection * -1.0f);
            // emit EmitStatusString(QString("Rotate %1 %2").arg(event->pos().x()).arg(event->pos().y()));
            updateCamera();
            renderLater();
        }
        else if (event->buttons() & Qt::RightButton && event->modifiers() == Qt::NoModifier)
        {
            setCursor(Qt::ClosedHandCursor);
            m_StartMove = Vector4f((2.f * float(event->pos().x()) / float(width())) - 1.f, (2.f * float(height() - event->pos().y()) / float(height())) - 1.f, 0.f, 1.f);
            m_WorldTransInverse = m_WorldTrans;
            m_WorldTransInverse.Inverse();
            m_StartMove = m_WorldTransInverse * m_StartMove;
            m_StartCentrePoint = m_CentrePoint;
            updateCamera();
            renderLater();
        }
        else if (event->buttons() & Qt::MiddleButton && event->modifiers() == Qt::NoModifier)
        {
            QPoint hotSpot = cursor().hotSpot();
            int x = event->pos().x() + hotSpot.x();
            int y = event->pos().y() + hotSpot.y();
            Vector3f screenSelect((2.f * float(x) / float(width())) - 1.f, (2.f * float(height() - y) / float(height())) - 1.f, 0);
            m_PointCloud->FindClosestPoint(screenSelect, m_WorldTrans, &m_CentrePoint);
            updateCamera();
            renderLater();
        }
       break;

    case sphereSelect:
        if (event->buttons() & Qt::LeftButton)
        {
            QPoint hotSpot = cursor().hotSpot();
            int x = event->pos().x() + hotSpot.x();
            int y = event->pos().y() + hotSpot.y();
            Vector3f screenSelect((2.f * float(x) / float(width())) - 1.f, (2.f * float(height() - y) / float(height())) - 1.f, 0);
            float screenRadius;
            if (m_PixelsForSelection) screenRadius = 2.f * m_PixelSelectionDistance / float(width());
            else
            {
                m_WorldTransInverse = m_WorldTrans;
                m_WorldTransInverse.Inverse();
                Vector3f inverseRadius = (m_WorldTransInverse * Vector4f(m_WorldSelectionDistance, 0, 0, 1)).To3f();
                Vector3f inverseOrigin = (m_WorldTransInverse * Vector4f(0, 0, 0, 1)).To3f();
                screenRadius = (inverseRadius - inverseOrigin).Magnitude();
            }
            int pointsInSelection;
            if (event->modifiers() == Qt::NoModifier)
                pointsInSelection = m_PointCloud->SetAlphaSphere(screenSelect, screenRadius, m_WorldTrans, PointCloud::selected, true);
            else
                pointsInSelection = m_PointCloud->SetAlphaSphere(screenSelect, screenRadius, m_WorldTrans, PointCloud::unselected, true);
            if (pointsInSelection == 0) m_PointCloud->SetAlpha(PointCloud::unselected);
            emit EmitUpdateModel(true);
        }
        break;

    }
}

void PointCloudWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_PointCloud == 0) return;

    switch (m_ToolMode)
    {
    case rectangleSelect:
    case lineSelect:
        if (event->buttons() & Qt::LeftButton)
        {

            m_EndSelect.x = (2.f * float(event->pos().x()) / float(width())) - 1.f;
            m_EndSelect.y = (2.f * float(height() - event->pos().y()) / float(height())) - 1.f;
            updateCamera();
            renderLater();
        }
        break;

    case moveTool:
    case nudgeTool:
        if (event->buttons() & Qt::LeftButton && m_TrackballEnable && event->modifiers() == Qt::NoModifier)
        {
            if (m_Trackball.GetActive())
            {
                m_Trackball.RollTrackballToClick(event->pos().x(), height() - event->pos().y());
                updateCamera();
                renderLater();
                // emit EmitStatusString(QString("Roll %1 %2").arg(event->pos().x()).arg(event->pos().y()));
            }
        }
        else if (event->buttons() & Qt::RightButton && event->modifiers() == Qt::NoModifier)
        {
            Vector4f currentMove((2.f * float(event->pos().x()) / float(width())) - 1.f, (2.f * float(height() - event->pos().y()) / float(height())) - 1.f, 0.f, 1.f);
            currentMove = m_WorldTransInverse * currentMove;
            m_CentrePoint = m_StartCentrePoint - (currentMove.To3f() - m_StartMove.To3f());
            updateCamera();
            renderLater();
        }
        break;

    case sphereSelect:
        break;

    }
}


void PointCloudWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_PointCloud == 0) return;

    switch (m_ToolMode)
    {
    case rectangleSelect:
        if (m_ActiveSelect)
        {
            if (m_EndSelect == m_StartSelect)
            {
                m_PointCloud->SetAlpha(PointCloud::unselected);
            }
            else
            {
                m_StartSelect.z = -1;
                m_EndSelect.z = 1;
                int pointsInSelection;
                if (event->modifiers() == Qt::NoModifier)
                    pointsInSelection = m_PointCloud->SetAlphaBounded(m_StartSelect, m_EndSelect, m_WorldTrans, PointCloud::selected);
                else
                    pointsInSelection = m_PointCloud->SetAlphaBounded(m_StartSelect, m_EndSelect, m_WorldTrans, PointCloud::unselected);
                if (pointsInSelection == 0) m_PointCloud->SetAlpha(PointCloud::unselected);
            }
            m_ActiveSelect = false;
            emit EmitUpdateModel(true);
        }
        break;

    case lineSelect:
        if (m_ActiveSelect)
        {
            if (m_EndSelect == m_StartSelect)
            {
                m_PointCloud->SetAlpha(PointCloud::unselected);
            }
            else
            {
                m_StartSelect.z = -1;
                m_EndSelect.z = 1;
                float screenRadius;
                if (m_PixelsForSelection) screenRadius = 2.f * m_PixelSelectionDistance / float(width());
                else
                {
                    m_WorldTransInverse = m_WorldTrans;
                    m_WorldTransInverse.Inverse();
                    Vector3f inverseRadius = (m_WorldTransInverse * Vector4f(m_WorldSelectionDistance, 0, 0, 1)).To3f();
                    Vector3f inverseOrigin = (m_WorldTransInverse * Vector4f(0, 0, 0, 1)).To3f();
                    screenRadius = (inverseRadius - inverseOrigin).Magnitude();
                }
                int pointsInSelection;
                if (event->modifiers() == Qt::NoModifier)
                    pointsInSelection = m_PointCloud->SetAlphaLine(m_StartSelect, m_EndSelect, screenRadius, m_WorldTrans, PointCloud::selected, true, true);
                else
                    pointsInSelection = m_PointCloud->SetAlphaLine(m_StartSelect, m_EndSelect, screenRadius, m_WorldTrans, PointCloud::unselected, true, true);
                if (pointsInSelection == 0) m_PointCloud->SetAlpha(PointCloud::unselected);
            }
            m_ActiveSelect = false;
            emit EmitUpdateModel(true);
        }
        break;

    case moveTool:
    case nudgeTool:
        setCursor(Qt::CrossCursor);
        if (m_Trackball.GetActive())
        {
            m_Trackball.SetActive(false);
            Quaternion q = m_Trackball.GetRotation();
            m_CameraRotation = q * m_CameraRotation;
        }
        break;

    case sphereSelect:
        break;
    }
}

void PointCloudWindow::wheelEvent(QWheelEvent * event)
{
    if (m_PointCloud == 0) return;
    double sensitivity;
    double scale;
    switch (m_ToolMode)
    {
    case sphereSelect:
    case rectangleSelect:
    case lineSelect:
        break;

    case moveTool:
    case nudgeTool:
        // assume each ratchet of the wheel gives a score of 120 (8 * 15 degrees)
        sensitivity = 1200;
        scale = 1.0 - double(ABS(event->delta())) / sensitivity;
        if (event->delta() < 0) scale = 1.0 / scale;
        m_Width *= scale;
        emit EmitStatusString(QString("Image Width %1").arg(m_Width));
        updateCamera();
        renderLater();
        break;

    }

}

void PointCloudWindow::keyPressEvent( QKeyEvent *e )
{
    if (m_PointCloud == 0) return;
    if (e->modifiers() & Qt::KeypadModifier) // handle view changes
    {
        if (e->key() == Qt::Key_1)
        {
            if (e->modifiers() == Qt::KeypadModifier) SetCameraRotationAxisAngle(0, 0, 1, 0); // front
            else SetCameraRotationAxisAngle(0, 0, 1, 180); // back
        }
        if (e->key() == Qt::Key_7)
        {
            if (e->modifiers() == Qt::KeypadModifier) SetCameraRotationAxisAngle(1, 0, 0, -90); // top
            else SetCameraRotationAxisAngle(1, 0, 0, 90); // bottom
       }
        if (e->key() == Qt::Key_3)
        {
            if (e->modifiers() == Qt::KeypadModifier) SetCameraRotationAxisAngle(0, 0, 1, 90); // right
            else SetCameraRotationAxisAngle(0, 0, 1, -90); // left
        }
        if (e->key() == Qt::Key_Period)
        {
            QPoint p = mapFromGlobal(QCursor::pos());
            QPoint hotSpot = cursor().hotSpot();
            int x = p.x() + hotSpot.x();
            int y = p.y() + hotSpot.y();
            Vector3f screenSelect((2.f * float(x) / float(width())) - 1.f, (2.f * float(height() - y) / float(height())) - 1.f, 0);
            Vector3f closestPoint(0, 0, 0);
            m_PointCloud->FindClosestPoint(screenSelect, m_WorldTrans, &closestPoint);
            SetCentrePoint(closestPoint);
        }
        // to do - think of some more uses for the keypad keys
        updateCamera();
        renderLater();
        return;
    }

    if (m_ToolMode != nudgeTool)
    {
        if ( (e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9) ||
             (e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z) )
        {
            char code = (char)e->key();
            if (e->modifiers() == Qt::NoModifier)
            {
                QPoint p = mapFromGlobal(QCursor::pos());
                QPoint hotSpot = cursor().hotSpot();
                int x = p.x() + hotSpot.x();
                int y = p.y() + hotSpot.y();
                //std::cerr << x << " " << y << "\n";
                //m_WorldTrans.Print();
                Vector3f screenSelect((2.f * float(x) / float(width())) - 1.f, (2.f * float(height() - y) / float(height())) - 1.f, 0);
                float screenRadius;
                if (m_PixelsForSelection) screenRadius = 2.f * m_PixelSelectionDistance / float(width());
                else
                {
                    m_WorldTransInverse = m_WorldTrans;
                    m_WorldTransInverse.Inverse();
                    Vector3f inverseRadius = (m_WorldTransInverse * Vector4f(m_WorldSelectionDistance, 0, 0, 1)).To3f();
                    Vector3f inverseOrigin = (m_WorldTransInverse * Vector4f(0, 0, 0, 1)).To3f();
                    screenRadius = (inverseRadius - inverseOrigin).Magnitude();
                }
                if (m_PointCloud->FitPointToSphere(screenSelect, screenRadius, m_WorldTrans)) return;
                QClipboard *clipboard = QApplication::clipboard();
                Vector3f point = m_PointCloud->GetLastPoint();
                clipboard->setText(QString("%1\t%2\t%3\t%4").arg(code).arg(point.x).arg(point.y).arg(point.z), QClipboard::Clipboard);
                emit EmitStatusString(QString("%1 %2 %3 %4").arg(code).arg(point.x).arg(point.y).arg(point.z));
                m_MeasurementData->Insert(QString(code), point.x, point.y, point.z);
                BuildMeasurementPointSceneNode(irr::core::vector3df(point.x, point.y, point.z), screenRadius, m_WorldTrans, QString(code));
            }
            else
            {
                m_MeasurementData->Delete(QString(code));
            }
            emit EmitUpdateModel(false);
        }
        else
        {
            IrrlichtWindow::keyPressEvent( e );
        }
    }
    else
    {
        float nudgeAngle = m_NudgeAngle; // degrees
        if (e->modifiers() != Qt::NoModifier) nudgeAngle = -nudgeAngle;
        if (e->key() == Qt::Key_X)
        {
            m_PointCloud->RotateByAxisAngle(1.f, 0.f, 0.f, nudgeAngle);
            emit EmitStatusString(QString("Rotate X  %1 degrees").arg(nudgeAngle));
            emit EmitUpdateModel(true);
        }
        else if (e->key() == Qt::Key_Y)
        {
            m_PointCloud->RotateByAxisAngle(0.f, 1.f, 0.f, nudgeAngle);
            emit EmitStatusString(QString("Rotate Y  %1 degrees").arg(nudgeAngle));
            emit EmitUpdateModel(true);
        }
        else if (e->key() == Qt::Key_Z)
        {
            m_PointCloud->RotateByAxisAngle(0.f, 0.f, 1.f, nudgeAngle);
            emit EmitStatusString(QString("Rotate Z  %1 degrees").arg(nudgeAngle));
            emit EmitUpdateModel(true);
        }
        else
        {
            IrrlichtWindow::keyPressEvent( e );
        }
    }
}


void PointCloudWindow::SetPointCloud(PointCloud * pointCloud)
{
    m_PointCloud = pointCloud;
}


void PointCloudWindow::UpdateModel(bool reloadBuffers)
{
    if (reloadBuffers) BuildPointCloudSceneNode();
    BuildAxesSceneNode();
    BuildSampleBoxSceneNode();
    updateCamera();
    renderLater();
}

void PointCloudWindow::SetCameraRotation(float x, float y, float z, float w)
{
    m_CameraRotation = Quaternion(x, y, z, w);
}

void PointCloudWindow::SetCameraRotationAxisAngle(float ax, float ay, float az, float degrees)
{
    m_CameraRotation = MakeQuaternionFromAxisAngle(ax, ay, az, degrees);
}

void PointCloudWindow::SetWidth(float width)
{
     m_Width = width;
}

void PointCloudWindow::SetCentrePoint(const Vector3f &centrePoint)
{
    m_CentrePoint = centrePoint;
}

void PointCloudWindow::SetToolMode(ToolMode toolMode)
{
    m_ToolMode = toolMode;
    switch (m_ToolMode)
    {
    case rectangleSelect:
    case lineSelect:
        setCursor(Qt::CrossCursor);
        break;

    case moveTool:
    case nudgeTool:
        setCursor(Qt::CrossCursor);
        break;

    case sphereSelect:
#ifdef USE_CUSTOM_CURSOR
        int i = (int)(m_SphereSelectRadius + 0.5f);
        if (i < 1) i = 1;
        else if (i > 16) i = 16;
        setCursor(m_Cursor[i - 1]);
#else
        setCursor(Qt::CrossCursor);
#endif
        break;

    }
}

void PointCloudWindow::ApplySettings(const Preferences &preferences)
{
    preferences.getValue("BackgroundColour", &m_BackgroundColour);
    preferences.getValue("SelectionColour", &m_SelectionColour);
    preferences.getValue("LabelColour", &m_LabelColour);
    preferences.getValue("GridColour", &m_GridColour);
    preferences.getValue("PixelsForSelection", &m_PixelsForSelection);
    preferences.getValue("CameraDistance", &m_CameraDistance);
    preferences.getValue("NudgeAngle", &m_NudgeAngle);
    preferences.getValue("AxisSize", &m_AxisSize);
    preferences.getValue("MaxDisplayPixels", &m_MaxPointsToRender);
    preferences.getValue("PixelSelectionDistance", &m_PixelSelectionDistance);
    preferences.getValue("WorldSelectionDistance", &m_WorldSelectionDistance);


    preferences.getValue("UseWireFrame", &m_wireFrame);
    preferences.getValue("DisplayBoundingBox", &m_boundingBox);
    preferences.getValue("DrawNormals", &m_normals);
    preferences.getValue("UseWireFrame", &m_wireFrame);
    preferences.getValue("CircularTesselation", &m_defaultTesselation);

    preferences.getValue("UseArrowForFittedLine", &m_useArrow);
    preferences.getValue("PointFitColour", &m_PointFitColour);
    preferences.getValue("LineFitColour", &m_LineFitColour);
    preferences.getValue("PlaneFitColour", &m_PlaneFitColour);

    preferences.getValue("AmbientProportion", &ambientProportion);
    preferences.getValue("DiffuseProportion", &diffuseProportion);
    preferences.getValue("SpecularProportion", &specularProportion);
    preferences.getValue("Shininess", &shininess);

    preferences.getValue("BuildMeasurementColour", &m_BuildMeasurementColour);


    setBackgroundColour(irr::video::SColor(m_BackgroundColour.alpha(), m_BackgroundColour.red(), m_BackgroundColour.green(), m_BackgroundColour.blue()));
}

void PointCloudWindow::BuildPointCloudSceneNode()
{
    if (m_pointCloudSceneNode)
        m_pointCloudSceneNode->remove();
    if (m_PointCloud)
    {
        irr::s32 idBitMask = 1 << 0;
        m_pointCloudSceneNode = new PointCloudSceneNode(sceneManager()->getRootSceneNode(), sceneManager(), idBitMask);
        m_pointCloudSceneNode->initializeVertices(m_PointCloud, m_MaxPointsToRender);
        irr::s32 state = irr::scene::EDS_OFF;
        if (m_boundingBox) state |= irr::scene::EDS_BBOX;
        if (m_boundingBoxBuffers) state |= irr::scene::EDS_BBOX_BUFFERS;
        if (m_wireFrame) state |= irr::scene::EDS_MESH_WIRE_OVERLAY;
        if (m_normals) state |= irr::scene::EDS_NORMALS;
        if (m_halfTransparency) state |= irr::scene::EDS_HALF_TRANSPARENCY;
        m_pointCloudSceneNode->setDebugDataVisible(state);
        m_pointCloudSceneNode->setName("PointCloud");
        m_pointCloudSceneNode->setAutomaticCulling(irr::scene::EAC_OFF); // no object based culling wanted
        m_pointCloudSceneNode->setMaterialType(irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);

        Matrix4f trans = m_PointCloud->GetTransformation();
        irr::core::matrix4 matrix;
        // Matrix4f is [row][col]; irr::core::matrix4 is [col * 4 + row] (this is from looking at the code not the documentation)
        matrix[0] =trans.m[0][0]; matrix[4] =trans.m[0][1]; matrix[8] =trans.m[0][2]; matrix[12]=trans.m[0][3];
        matrix[1] =trans.m[1][0]; matrix[5] =trans.m[1][1]; matrix[9] =trans.m[1][2]; matrix[13]=trans.m[1][3];
        matrix[2] =trans.m[2][0]; matrix[6] =trans.m[2][1]; matrix[10]=trans.m[2][2]; matrix[14]=trans.m[2][3];
        matrix[3] =trans.m[3][0]; matrix[7] =trans.m[3][1]; matrix[11]=trans.m[3][2]; matrix[15]=trans.m[3][3];
        m_pointCloudSceneNode->setRotation(matrix.getRotationDegrees());
        m_pointCloudSceneNode->setPosition(matrix.getTranslation());
        m_pointCloudSceneNode->setScale(matrix.getScale());
    }
}

// set up all the things needed to get the colour right
void PointCloudWindow::SetMeshColour(irr::scene::IMesh *mesh, const irr::video::SColor &colour)
{
    // sort out the mesh buffers
    irr::u32 r = colour.getRed();
    irr::u32 g = colour.getGreen();
    irr::u32 b = colour.getBlue();
    irr::u32 a = colour.getAlpha();
    float ambientProportion = 0.2, diffuseProportion = 0.8, specularProportion = 0.3, shininess = 20;
    irr::scene::IMeshBuffer *meshBuffer;
    irr::u32 meshBufferCount = mesh->getMeshBufferCount();
    for (irr::u32 i = 0; i < meshBufferCount; i++)
    {
        meshBuffer = mesh->getMeshBuffer(i);
        irr::video::SMaterial &material = meshBuffer->getMaterial();
        material.GouraudShading = false;
        material.NormalizeNormals = false;
        material.BackfaceCulling = false;
        material.FrontfaceCulling = false; // this can be set to true for most geometryCreator solids but turning it off is safer
        material.AmbientColor.set(a * ambientProportion, r * ambientProportion, g * ambientProportion, b * ambientProportion);
        material.DiffuseColor.set(a * diffuseProportion, r * diffuseProportion, g * diffuseProportion, b * diffuseProportion);
        material.SpecularColor.set(a * specularProportion, r * specularProportion, g * specularProportion, b * specularProportion);
        material.Shininess = shininess;
    }

    // do the per mesh settings
    mesh->setMaterialFlag(irr::video::EMF_GOURAUD_SHADING, false);
    mesh->setMaterialFlag(irr::video::EMF_NORMALIZE_NORMALS, false);
    mesh->setMaterialFlag(irr::video::EMF_BACK_FACE_CULLING, false);
    mesh->setMaterialFlag(irr::video::EMF_FRONT_FACE_CULLING, false);
}

void PointCloudWindow::BuildAxesSceneNode()
{
    if (m_axisSceneNode)
        m_axisSceneNode->remove();

    // create meshes for the axes
    irr::scene::IMesh *mesh;
    irr::scene::IMeshSceneNode *sceneNode;
    const irr::scene::IGeometryCreator *geometryCreator = sceneManager()->getGeometryCreator();

    float widthFraction = 0.1;
    mesh = geometryCreator->createCubeMesh(irr::core::vector3df(m_AxisSize * widthFraction, m_AxisSize * widthFraction, m_AxisSize * widthFraction));
    SetMeshColour(mesh, irr::video::SColor(255, 220, 220, 220)); // pale grey
    m_axisSceneNode = sceneManager()->addMeshSceneNode(mesh);
    mesh->drop();
    m_axisSceneNode->setAutomaticCulling(irr::scene::EAC_OFF);

    irr::u32 tesselationCylinder = m_defaultTesselation;
    irr::u32 tesselationCone = m_defaultTesselation;
    irr::f32 height = m_AxisSize;
    irr::f32 cylinderHeight = height * (1 - 2 * widthFraction);
    irr::f32 widthCylinder = height * 0.25 * widthFraction;
    irr::f32 widthCone = height * widthFraction * 0.5;
    irr::video::SColor colorCylinder(255, 255, 0, 0); // red
    irr::video::SColor colorCone = colorCylinder;
    mesh = geometryCreator->createArrowMesh(tesselationCylinder, tesselationCone, height, cylinderHeight, widthCylinder, widthCone, colorCylinder, colorCone);
    SetMeshColour(mesh, irr::video::SColor(255, 255, 0, 0));
    sceneNode = sceneManager()->addMeshSceneNode(mesh, m_axisSceneNode);
    mesh->drop();
    sceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    sceneNode->setRotation(irr::core::vector3df(0, 0, -90)); // rotates Y direction to X direction

    colorCylinder.set(255, 0, 255, 0); // green
    colorCone = colorCylinder;
    mesh = geometryCreator->createArrowMesh(tesselationCylinder, tesselationCone, height, cylinderHeight, widthCylinder, widthCone, colorCylinder, colorCone);
    SetMeshColour(mesh, irr::video::SColor(255, 0, 255, 0));
    sceneNode = sceneManager()->addMeshSceneNode(mesh, m_axisSceneNode);
    mesh->drop();
    sceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    sceneNode->setRotation(irr::core::vector3df(0, 0, 0)); // rotates Y direction to Y direction

    colorCylinder.set(255, 0, 0, 255); // blue
    colorCone = colorCylinder;
    mesh = geometryCreator->createArrowMesh(tesselationCylinder, tesselationCone, height, cylinderHeight, widthCylinder, widthCone, colorCylinder, colorCone);
    SetMeshColour(mesh, irr::video::SColor(255, 0, 255, 0));
    sceneNode = sceneManager()->addMeshSceneNode(mesh, m_axisSceneNode);
    mesh->drop();
    sceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    sceneNode->setRotation(irr::core::vector3df(90, 0, 0)); // rotates Y direction to Z direction
    // set the origin axis to 0,0,0
    m_axisSceneNode->setPosition(irr::core::vector3df(0, 0, 0));
    if (m_AxisSize == 0)
        m_axisSceneNode->setVisible(false);
}

void PointCloudWindow::BuildMeasurementPointSceneNode(const irr::core::vector3df &centre, float screenRadius, const Matrix4f &worldTrans, const QString &text)
{
    if (m_measurementPointSceneNodeMap.contains(text))
        m_measurementPointSceneNodeMap.value(text)->remove();

    // now find the inverse transformed radius
    Vector4f origin(0.f, 0.f, 0.f, 1.f);
    Vector4f radius(screenRadius, 0.f, 0.f, 1.f);
    Matrix4f inverse = worldTrans;
    inverse.Inverse();
    Vector3f transOrigin = (inverse * origin).To3f();
    Vector3f transRadius = (inverse * radius).To3f();
    Vector3f offset3 = transRadius - transOrigin;
    float radiusMagnitude = offset3.Magnitude();

    irr::scene::IMeshSceneNode *sphereSceneNode = sceneManager()->addSphereSceneNode(radiusMagnitude, m_defaultTesselation);
    SetMeshColour(sphereSceneNode->getMesh(), irr::video::SColor(m_BuildMeasurementColour.alpha(), m_BuildMeasurementColour.red(), m_BuildMeasurementColour.green(), m_BuildMeasurementColour.blue()));
    sphereSceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    sphereSceneNode->setPosition(centre);
    if (screenRadius == 0)
        sphereSceneNode->setVisible(false);

// this doesn't currently work
//    wchar_t array[text.size() + 1];
//    text.toWCharArray(array);
//    irr::scene::IBillboardTextSceneNode *billboardSceneNode = sceneManager()->addBillboardTextSceneNode(0, array, sphereSceneNode,
//                                                                                                        irr::core::dimension2df(radiusMagnitude, radiusMagnitude),
//                                                                                                        irr::core::vector3df(0, 0, radiusMagnitude * 1.1));

    m_measurementPointSceneNodeMap.insert(text, sphereSceneNode);
}


void PointCloudWindow::BuildFittedPointSceneNode(const irr::core::vector3df &centre, float radius)
{
    if (m_fittedPointSceneNode)
        m_fittedPointSceneNode->remove();

    m_fittedPointRadius = radius;
    m_fittedPointSceneNode = sceneManager()->addSphereSceneNode(m_fittedPointRadius, m_defaultTesselation);
    SetMeshColour(m_fittedPointSceneNode->getMesh(), irr::video::SColor(m_PointFitColour.alpha(), m_PointFitColour.red(), m_PointFitColour.green(), m_PointFitColour.blue()));
    m_fittedPointSceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    m_fittedPointSceneNode->setPosition(centre);
    if (m_fittedPointRadius == 0)
        m_fittedPointSceneNode->setVisible(false);
}

void PointCloudWindow::BuildFittedLineSceneNode(const irr::core::vector3df &lineStart, const irr::core::vector3df &lineEnd)
{
    if (m_fittedLineSceneNode)
        m_fittedLineSceneNode->remove();

    irr::scene::IMesh *mesh;
    const irr::scene::IGeometryCreator *geometryCreator = sceneManager()->getGeometryCreator();

    irr::core::vector3df lineVector = lineEnd - lineStart;
    m_fittedLineLength = lineVector.getLength();

    irr::f32 widthFraction = 0.1;
    if (m_useArrow)
    {
        irr::u32 tesselationCylinder = m_defaultTesselation;
        irr::u32 tesselationCone = m_defaultTesselation;
        irr::f32 cylinderHeight = m_fittedLineLength * (1 - 2 * widthFraction);
        irr::f32 widthCylinder = m_fittedLineLength * 0.25 * widthFraction;
        irr::f32 widthCone = m_fittedLineLength * widthFraction * 0.5;
        mesh = geometryCreator->createArrowMesh(tesselationCylinder, tesselationCone, m_fittedLineLength, cylinderHeight, widthCylinder, widthCone);
        SetMeshColour(mesh, irr::video::SColor(irr::video::SColor(m_LineFitColour.alpha(), m_LineFitColour.red(), m_LineFitColour.green(), m_LineFitColour.blue())));
        m_fittedLineSceneNode = sceneManager()->addMeshSceneNode(mesh, 0);
        mesh->drop();
    }
    else
    {
        irr::u32 cylinderTesselation = m_defaultTesselation;
        irr::f32 cylinderRadius = m_fittedLineLength * 0.125 * widthFraction;
        // create a y axis aligned cylinder with base at origin
        mesh = geometryCreator->createCylinderMesh(cylinderRadius, m_fittedLineLength, cylinderTesselation);
        SetMeshColour(mesh, irr::video::SColor(m_LineFitColour.alpha(), m_LineFitColour.red(), m_LineFitColour.green(), m_LineFitColour.blue()));
        m_fittedLineSceneNode = sceneManager()->addMeshSceneNode(mesh, 0);
        mesh->drop();
    }

    irr::core::vector3df from(0, 1, 0);
    irr::core::vector3df to = lineVector;
    irr::core::quaternion q;
    q.rotationFromTo(from, to);
    irr::core::vector3df euler;
    q.toEuler(euler);
    m_fittedLineSceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    m_fittedLineSceneNode->setPosition(lineStart);
    m_fittedLineSceneNode->setRotation(euler * irr::core::RADTODEG);
    if (m_fittedLineLength == 0)
        m_fittedLineSceneNode->setVisible(false);
}

void PointCloudWindow::BuildFittedPlaneSceneNode(const irr::core::vector3df &planeCentre, const irr::core::vector3df &normal, float size)
{
    if (m_fittedPlaneSceneNode)
        m_fittedPlaneSceneNode->remove();

    irr::scene::IMesh *mesh;
    const irr::scene::IGeometryCreator *geometryCreator = sceneManager()->getGeometryCreator();

    m_fittedPlaneSize = size;

    if (m_useArrow)
    {
        irr::u32 tesselationCylinder = m_defaultTesselation;
        irr::u32 tesselationCone = m_defaultTesselation;
        irr::f32 height = m_fittedPlaneSize;
        irr::f32 widthFraction = 0.1;
        irr::f32 cylinderHeight = height * (1 - 2 * widthFraction);
        irr::f32 widthCylinder = height * 0.25 * widthFraction;
        irr::f32 widthCone = height * widthFraction * 0.5;
        mesh = geometryCreator->createArrowMesh(tesselationCylinder, tesselationCone, height, cylinderHeight, widthCylinder, widthCone);
        SetMeshColour(mesh, irr::video::SColor(m_PlaneFitColour.alpha(), m_PlaneFitColour.red(), m_PlaneFitColour.green(), m_PlaneFitColour.blue()));
        m_fittedPlaneSceneNode = sceneManager()->addMeshSceneNode(mesh, 0);
        mesh->drop();
        mesh = geometryCreator->createPlaneMesh(irr::core::dimension2df(m_fittedPlaneSize, m_fittedPlaneSize));
        SetMeshColour(mesh, irr::video::SColor(m_PlaneFitColour.alpha(), m_PlaneFitColour.red(), m_PlaneFitColour.green(), m_PlaneFitColour.blue()));
        irr::scene::IMeshSceneNode* planeSceneNode = sceneManager()->addMeshSceneNode(mesh, m_fittedPlaneSceneNode);
        planeSceneNode->getMaterial(0).PolygonOffsetDirection = irr::video::EPO_FRONT; // this means that the texture will sit on top of anything else in the same plane
        planeSceneNode->getMaterial(0).PolygonOffsetFactor=7;
        mesh->drop();
    }
    else
    {
        // create a plane with a y axis aligned normal and centre base origin
        mesh = geometryCreator->createPlaneMesh(irr::core::dimension2df(m_fittedPlaneSize, m_fittedPlaneSize));
        SetMeshColour(mesh, irr::video::SColor(m_PlaneFitColour.alpha(), m_PlaneFitColour.red(), m_PlaneFitColour.green(), m_PlaneFitColour.blue()));
        m_fittedPlaneSceneNode = sceneManager()->addMeshSceneNode(mesh, 0);
        mesh->drop();
    }

    irr::core::vector3df from(0, 1, 0);
    irr::core::vector3df to = normal;
    irr::core::quaternion q;
    q.rotationFromTo(from, to);
    irr::core::vector3df euler;
    q.toEuler(euler);
    m_fittedPlaneSceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    m_fittedPlaneSceneNode->setPosition(planeCentre);
    m_fittedPlaneSceneNode->setRotation(euler * irr::core::RADTODEG);
    if (m_fittedPlaneSize == 0)
        m_fittedPlaneSceneNode->setVisible(false);
}

void PointCloudWindow::SetSampleBox(const Vector3f &minCorner, const Vector3f &maxCorner)
{
    m_sampleBoxMaxCorner = maxCorner;
    m_sampleBoxMinCorner = minCorner;
}

void PointCloudWindow::BuildSampleBoxSceneNode()
{
    if (m_sampleBoxSceneNode)
        m_sampleBoxSceneNode->remove();


    irr::core::vector3df minCorner(m_sampleBoxMinCorner.x, m_sampleBoxMinCorner.y, m_sampleBoxMinCorner.z);
    irr::core::vector3df maxCorner(m_sampleBoxMaxCorner.x, m_sampleBoxMaxCorner.y, m_sampleBoxMaxCorner.z);
    irr::core::vector3df position = minCorner.getInterpolated(maxCorner, 0.5);
    irr::core::vector3df rotation(0, 0, 0);
    irr::core::vector3df scale = maxCorner - minCorner;
    m_sampleBoxSceneNode = sceneManager()->addCubeSceneNode(1.0f, 0, -1, position, rotation, scale);
    irr::s32 state = irr::scene::EDS_OFF;
    state |= irr::scene::EDS_BBOX;
//    state |= irr::scene::EDS_BBOX_BUFFERS;
//    state |= irr::scene::EDS_MESH_WIRE_OVERLAY;
//    state |= irr::scene::EDS_NORMALS;
//    state |= irr::scene::EDS_HALF_TRANSPARENCY;
    m_sampleBoxSceneNode->setDebugDataVisible(state);
    m_sampleBoxSceneNode->setName("BuildSampleBox");
    m_sampleBoxSceneNode->setAutomaticCulling(irr::scene::EAC_OFF);
    if (scale.X == 0 || scale.Y == 0 || scale.Z == 0)
    {
        m_sampleBoxSceneNode->setVisible(false);
        m_sampleBoxValid = false;
    }
    else
    {
        m_sampleBoxValid = true;
    }
}

