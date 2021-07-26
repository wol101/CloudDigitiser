#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

#include "pipeline/math_3d.h"

#include "Preferences.h"

class QGridLayout;
class GLWidget;
class PointCloud;
class MeasurementData;
class PointCloudWindow;
class SampleBoxData;
class DefineSampleBoxDialog;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
    int OpenFile();
    int SaveFile();
    void Rescale();
    void DefaultView();

public slots:
    void actionOpen_triggered();
    void actionQuit_triggered();
    void actionAbout_triggered();
    void actionFitLine_triggered();
    void actionFitPlane_triggered();
    void actionRotateToX_triggered();
    void actionRotateToY_triggered();
    void actionRotateToZ_triggered();
    void actionRotateAboutX_triggered();
    void actionRotateAboutY_triggered();
    void actionRotateAboutZ_triggered();
    void actionRotateToXAboutZ_triggered();
    void actionRotateToYAboutZ_triggered();
    void actionRotateToZAboutX_triggered();
    void actionResetTransformation_triggered();
    void actionFitPoint_triggered();
    void actionSetOrigin_triggered();
    void actionApplyMatrix_triggered();
    void actionSaveAs_triggered();
    void actionSaveAsMatrix_triggered();
    void actionSaveMeasurements_triggered();
    void actionSaveMeasurementsAs_triggered();
    void actionOpenMeasurements_triggered();
    void actionImportFromMeasurements_triggered();
    void actionNewMeasurements_triggered();
    void actionBatchTransform_triggered();
    void actionPreferences_triggered();
    void actionNextFile_triggered();
    void actionPreviousFile_triggered();
    void actionDefaultView_triggered();
    void actionCalibratePoint_triggered();
    void actionRectSelect_triggered();
    void actionSphereSelect_triggered();
    void actionLineSelect_triggered();
    void actionMoveTool_triggered();
    void actionNudge_triggered();
    void actionViewLeft_triggered();
    void actionViewRight_triggered();
    void actionViewTop_triggered();
    void actionViewBottom_triggered();
    void actionViewFront_triggered();
    void actionViewBack_triggered();
    void actionDefineSampleBox_triggered();
    void actionProcessSampleBox_triggered();

    void UpdateModel(bool reloadBuffers);
    void SetStatusString(QString s);
    void DefineSampleBoxDialogData();

private:
    void EnableActions();

    Ui::MainWindow *ui;

    // QGridLayout *m_gridLayout;
    PointCloudWindow *m_pointCloudWindow;
    QString m_fileName;
    QString m_measurementsFileName;
    PointCloud *m_pointCloud;
    QMap<QString, MeasurementData *> m_MeasurementDataMap;
    QMap<QString, SampleBoxData *> m_SampleBoxDataMap;

    Preferences m_Preferences;
    DefineSampleBoxDialog *m_defineSampleBoxDialog;
};

#endif // MAINWINDOW_H
