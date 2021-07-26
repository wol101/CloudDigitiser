#include <QGridLayout>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QColor>
#include <QDomDocument>
#include <QtAlgorithms>
#include <QTextStream>

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "PointCloud.h"
#include "BatchProcess.h"
#include "BatchProcessDialog.h"
#include "CalibrateDialog.h"
#include "MeasurementData.h"
#include "AboutDialog.h"
#include "Preferences.h"
#include "PreferencesDialog.h"
#include "PointCloudWindow.h"
#include "Misc.h"
#include "DefineSampleBoxDialog.h"
#include "SampleBoxData.h"
#include "ProcessSampleBox.h"


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) >= 0 ? (a) : -(a))
#define SQUARE(a) ((a) * (a))

#define OPEN_LAST_FILE 1

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (m_Preferences.contains("MainWindowSize")) resize(m_Preferences.settingsItem("MainWindowSize").value.toSize());
    if (m_Preferences.contains("MainWindowPos")) move(m_Preferences.settingsItem("MainWindowPos").value.toPoint());

//    if (m_Preferences.MainWindowGeometry.size() > 0) restoreGeometry(m_Preferences.MainWindowGeometry);
//    if (m_Preferences.MainWindowState.size() > 0) restoreState(m_Preferences.MainWindowState);

    // put SimulationWindow into existing widgetGLWidget
    m_pointCloudWindow = new PointCloudWindow();
//    m_topLeftWidget->setShowDemo(true);
//    m_topLeftWidget->initializeIrrlicht(); // Irrlicht must be initialised BEFORE createWindowContainer to show the demo
//    m_topLeftWidget->initialiseScene(); // this will delete the demo

    QBoxLayout *boxLayout = new QBoxLayout(QBoxLayout::LeftToRight, ui->widgetPointCloudPlaceholder);
    boxLayout->setMargin(0);
    QWidget *container = QWidget::createWindowContainer(m_pointCloudWindow);
    boxLayout->addWidget(container);

    m_pointCloudWindow->initializeIrrlicht(); // this doesn't show anything because it is after the createWindowContainer but it stops things crashing later

    // connect the GLWidget to the MainWindow
    QObject::connect(m_pointCloudWindow, SIGNAL(EmitUpdateModel(bool)), this, SLOT(UpdateModel(bool)));

    QObject::connect(m_pointCloudWindow, SIGNAL(EmitStatusString(QString)), this, SLOT(SetStatusString(QString)));

    m_pointCloudWindow->SetCameraRotationAxisAngle(0, 0, 1, 0); // this should be front view
    m_pointCloudWindow->EnableTrackball(true);
    m_pointCloudWindow->ApplySettings(m_Preferences);
    m_pointCloudWindow->SetToolMode(PointCloudWindow::moveTool);

    m_pointCloud = 0;

    if (m_Preferences.settingsItem("LoadPreviousFileAtStartup").value.toBool())
    {
        m_Preferences.getValue("LastOpenedPLYFile", &m_fileName);
        OpenFile();
        actionDefaultView_triggered();
        UpdateModel(true);
    }
    QDir::setCurrent(QFileInfo(m_Preferences.settingsItem("LastOpenedPLYFile").value.toString()).absolutePath());

    m_defineSampleBoxDialog = new DefineSampleBoxDialog(this);
    m_defineSampleBoxDialog->hide();
    QObject::connect(m_defineSampleBoxDialog, SIGNAL(DefineSampleBoxDialogDataChanged()), this, SLOT(DefineSampleBoxDialogData()));

    EnableActions();
}

MainWindow::~MainWindow()
{
    if (m_pointCloud) delete m_pointCloud;
    for (QMap<QString, MeasurementData *>::const_iterator i = m_MeasurementDataMap.constBegin(); i != m_MeasurementDataMap.constEnd(); i++) delete i.value();
    for (QMap<QString, SampleBoxData *>::const_iterator i = m_SampleBoxDataMap.constBegin(); i != m_SampleBoxDataMap.constEnd(); i++) delete i.value();
    delete m_defineSampleBoxDialog;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bool unsavedData = false;
    QMap<QString, MeasurementData *>::const_iterator i = m_MeasurementDataMap.constBegin();
    while (i != m_MeasurementDataMap.constEnd())
    {
        if (i.value()->GetChanged())
        {
            unsavedData = true;
            break;
        }
        ++i;
    }

    if (unsavedData)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, QApplication::applicationName(),
                                                                  tr("You have unsaved measurements.\n") + tr("Are you sure you want to quit the application?"),
                                                                  QMessageBox::No | QMessageBox::Yes);
        if (reply == QMessageBox::No)
        {
            return;
        }
    }

//    m_Preferences.value("MainWindowGeometry") = saveGeometry();
//    m_Preferences.value("MainWindowState") = saveState();
    m_Preferences.insert("MainWindowSize", size());
    m_Preferences.insert("MainWindowPos", pos());
    m_Preferences.Write();

    QMainWindow::closeEvent(event);
}

void MainWindow::actionOpen_triggered()
{
    m_Preferences.getValue("LastOpenedPLYFile", &m_fileName);
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open PLY File"), m_fileName, tr("Point Cloud Files (*.ply)"), 0/*, QFileDialog::DontUseNativeDialog*/);

    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        m_fileName = fileName;

        OpenFile();
        // if (m_MeasurementDataMap.size() == 0) DefaultView();
        UpdateModel(true);
    }
}

void MainWindow::actionAbout_triggered()
{
    AboutDialog aboutDialog(this);

    int status = aboutDialog.exec();

    if (status == QDialog::Accepted)
    {
    }
}

void MainWindow::actionQuit_triggered()
{
    close();
}

void MainWindow::actionFitLine_triggered()
{
    if (m_pointCloud == 0) return;
    QClipboard *clipboard = QApplication::clipboard();
    if (m_pointCloud->FitLineToAlpha(PointCloud::selected, PointCloud::unselected))
    {
        clipboard->setText("Fit unsuccessful", QClipboard::Clipboard);
        statusBar()->showMessage("Fit unsuccessful");
    }
    Vector3f point = m_pointCloud->GetLastPoint();
    Vector3f vector = m_pointCloud->GetLastVector();
    clipboard->setText(QString("%1\t%2\t%3\t%4\t%5\t%6").arg(point.x).arg(point.y).arg(point.z).arg(vector.x).arg(vector.y).arg(vector.z), QClipboard::Clipboard);
    statusBar()->showMessage(QString("Point %1 %2 %3 Vector %4 %5 %6").arg(point.x).arg(point.y).arg(point.z).arg(vector.x).arg(vector.y).arg(vector.z));
    Vector3f minBound = m_pointCloud->GetSelectionMinBound();
    Vector3f maxBound = m_pointCloud->GetSelectionMaxBound();
    Vector3f diagonal = maxBound - minBound;
    float length = diagonal.Magnitude() / 2;
    Vector3f lineStart = point - length * vector;
    Vector3f lineEnd = point + length * vector;
    m_pointCloudWindow->BuildFittedLineSceneNode(irr::core::vector3df(lineStart.x, lineStart.y, lineStart.z), irr::core::vector3df(lineEnd.x, lineEnd.y, lineEnd.z));
    UpdateModel(true);
}

void MainWindow::actionFitPlane_triggered()
{
    if (m_pointCloud == 0) return;
    QClipboard *clipboard = QApplication::clipboard();
    if (m_pointCloud->FitPlaneToAlpha(PointCloud::selected, PointCloud::unselected))
    {
        clipboard->setText("Fit unsuccessful", QClipboard::Clipboard);
        statusBar()->showMessage("Fit unsuccessful");
    }
    Vector3f point = m_pointCloud->GetLastPoint();
    Vector3f vector = m_pointCloud->GetLastVector();
    clipboard->setText(QString("%1\t%2\t%3\t%4\t%5\t%6").arg(point.x).arg(point.y).arg(point.z).arg(vector.x).arg(vector.y).arg(vector.z), QClipboard::Clipboard);
    statusBar()->showMessage(QString("Point %1 %2 %3 Vector %4 %5 %6").arg(point.x).arg(point.y).arg(point.z).arg(vector.x).arg(vector.y).arg(vector.z));
    Vector3f minBound = m_pointCloud->GetSelectionMinBound();
    Vector3f maxBound = m_pointCloud->GetSelectionMaxBound();
    Vector3f diagonal = maxBound - minBound;
    m_pointCloudWindow->BuildFittedPlaneSceneNode(irr::core::vector3df(point.x, point.y, point.z), irr::core::vector3df(vector.x, vector.y, vector.z), diagonal.Magnitude());
    UpdateModel(true);
}

void MainWindow::actionFitPoint_triggered()
{
    if (m_pointCloud == 0) return;
    QClipboard *clipboard = QApplication::clipboard();
    if (m_pointCloud->FitPointToAlpha(PointCloud::selected, PointCloud::unselected))
    {
        clipboard->setText("Fit unsuccessful", QClipboard::Clipboard);
        statusBar()->showMessage("Fit unsuccessful");
    }
    Vector3f point = m_pointCloud->GetLastPoint();
    clipboard->setText(QString("%1\t%2\t%3").arg(point.x).arg(point.y).arg(point.z), QClipboard::Clipboard);
    statusBar()->showMessage(QString("Point %1 %2 %3").arg(point.x).arg(point.y).arg(point.z));
    Vector3f minBound = m_pointCloud->GetSelectionMinBound();
    Vector3f maxBound = m_pointCloud->GetSelectionMaxBound();
    Vector3f diagonal = maxBound - minBound;
    m_pointCloudWindow->BuildFittedPointSceneNode(irr::core::vector3df(point.x, point.y, point.z), diagonal.Magnitude() / 2);
    UpdateModel(true);
}

void MainWindow::actionRotateToX_triggered()
{
    if (m_pointCloud == 0) return;
    Vector3f vector = m_pointCloud->GetLastVector();
    m_pointCloud->RotateToVector(1, 0, 0);
    statusBar()->showMessage(QString("Rotated (%1,%2,%3) to (1,0,0)").arg(vector.x).arg(vector.y).arg(vector.z));
    UpdateModel(true);
}

void MainWindow::actionRotateToY_triggered()
{
    if (m_pointCloud == 0) return;
    Vector3f vector = m_pointCloud->GetLastVector();
    m_pointCloud->RotateToVector(0, 1, 0);
    statusBar()->showMessage(QString("Rotated (%1,%2,%3) to (0,1,0)").arg(vector.x).arg(vector.y).arg(vector.z));
    UpdateModel(true);
}

void MainWindow::actionRotateToZ_triggered()
{
    if (m_pointCloud == 0) return;
    Vector3f vector = m_pointCloud->GetLastVector();
    m_pointCloud->RotateToVector(0, 0, 1);
    statusBar()->showMessage(QString("Rotated (%1,%2,%3) to (0,0,1)").arg(vector.x).arg(vector.y).arg(vector.z));
    UpdateModel(true);
}

void MainWindow::actionRotateToXAboutZ_triggered()
{
    if (m_pointCloud == 0) return;
    Vector3f vector = m_pointCloud->GetLastVector();
    m_pointCloud->RotateToVector(1, 0, 0, 0, 0, 1);
    statusBar()->showMessage(QString("Rotated (%1,%2,%3) to (1,0,0) around (0,0,1)").arg(vector.x).arg(vector.y).arg(vector.z));
    UpdateModel(true);
}

void MainWindow::actionRotateToYAboutZ_triggered()
{
    if (m_pointCloud == 0) return;
    Vector3f vector = m_pointCloud->GetLastVector();
    m_pointCloud->RotateToVector(0, 1, 0, 0, 0, 1);
    statusBar()->showMessage(QString("Rotated (%1,%2,%3) to (1,0,0) around (0,0,1)").arg(vector.x).arg(vector.y).arg(vector.z));
    UpdateModel(true);
}

void MainWindow::actionRotateToZAboutX_triggered()
{
    if (m_pointCloud == 0) return;
    Vector3f vector = m_pointCloud->GetLastVector();
    m_pointCloud->RotateToVector(0, 0, 1, 1, 0, 0);
    statusBar()->showMessage(QString("Rotated (%1,%2,%3) to (0,0,1) around (1,0,0)").arg(vector.x).arg(vector.y).arg(vector.z));
    UpdateModel(true);
}

void MainWindow::actionRotateAboutX_triggered()
{
    if (m_pointCloud == 0) return;
    Quaternion q = MakeQuaternionFromAxisAngle(1, 0, 0, 180);
    m_pointCloud->RotateByQuaternion(q.x, q.y, q.z, q.w);
    UpdateModel(true);
}

void MainWindow::actionRotateAboutY_triggered()
{
    if (m_pointCloud == 0) return;
    Quaternion q = MakeQuaternionFromAxisAngle(0, 1, 0, 180);
    m_pointCloud->RotateByQuaternion(q.x, q.y, q.z, q.w);
    UpdateModel(true);
}

void MainWindow::actionRotateAboutZ_triggered()
{
    if (m_pointCloud == 0) return;
    Quaternion q = MakeQuaternionFromAxisAngle(0, 0, 1, 180);
    m_pointCloud->RotateByQuaternion(q.x, q.y, q.z, q.w);
    UpdateModel(true);
}

void MainWindow::actionSetOrigin_triggered()
{
    if (m_pointCloud == 0) return;
    Vector3f lastPoint = m_pointCloud->GetLastPoint();
    m_pointCloud->Translate(-lastPoint.x, -lastPoint.y, -lastPoint.z);
    statusBar()->showMessage(QString("Setting Origin to %1 %2 %3").arg(lastPoint.x).arg(lastPoint.y).arg(lastPoint.z));
    UpdateModel(true);
}

void MainWindow::actionApplyMatrix_triggered()
{
    if (m_pointCloud == 0) return;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open transformation matrix file"), ".", tr("Text Files (*.txt)"), 0/*, QFileDialog::DontUseNativeDialog*/);

    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        m_pointCloud->LoadMatrix(QFile::encodeName(fileName));
        UpdateModel(true);
    }
}

void MainWindow::actionSaveAs_triggered()
{
    if (m_pointCloud == 0) return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save PLY File"), m_fileName, tr("Point Cloud Files (*.ply)"), 0/*, QFileDialog::DontUseNativeDialog*/);

    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        m_fileName = fileName;
        SaveFile();
    }
}

void MainWindow::actionSaveAsMatrix_triggered()
{
    if (m_pointCloud == 0) return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save transformation matrix to file"), m_fileName, tr("Text Files (*.txt)"), 0/*, QFileDialog::DontUseNativeDialog*/);

    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        m_pointCloud->SaveMatrix(QFile::encodeName(fileName));
    }
}

void MainWindow::actionSaveMeasurementsAs_triggered()
{
    if (m_pointCloud == 0) return;

    QFileInfo info(m_fileName);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save measurements data to file"), info.completeBaseName() + ".xml", tr("XML Files (*.xml)"), 0/*, QFileDialog::DontUseNativeDialog*/);
    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        m_measurementsFileName = fileName;
        actionSaveMeasurements_triggered();
    }
}

void MainWindow::actionSaveMeasurements_triggered()
{
    if (m_pointCloud == 0) return;

    if (m_measurementsFileName.isEmpty())
    {
        QFileInfo info(m_fileName);
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save measurements data to file"), info.completeBaseName() + ".xml", tr("XML Files (*.xml)"), 0/*, QFileDialog::DontUseNativeDialog*/);
        if (fileName.isNull()) return;
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        m_measurementsFileName = fileName;
    }


    QFile file(m_measurementsFileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning("Unable to open to measurements file: %s", qPrintable(m_measurementsFileName));
        return;
    }

    QFileInfo filenameInfo(m_measurementsFileName);
    QDir parentFolder = QDir(filenameInfo.path());

    QDomDocument doc("CLOUDDIGITISERXML");
    QDomElement root = doc.createElement("CLOUDDIGITISERXML");
    doc.appendChild(root);

    QDomElement views = doc.createElement("VIEWS");
    root.appendChild(views);
    Vector3f centre = m_pointCloudWindow->GetCentrePoint();
    float width = m_pointCloudWindow->GetWidth();
    views.setAttribute("CentreTLX", centre.x);
    views.setAttribute("CentreTLY", centre.y);
    views.setAttribute("CentreTLZ", centre.z);
    views.setAttribute("WidthTL", width);

    QDomElement transform = doc.createElement("TRANSFORM");
    root.appendChild(transform);
    QString transformString;
    Matrix4f modelMatrix = m_pointCloud->GetTransformation();
    for (int i = 0 ; i < 4 ; i++)
        transformString += QString::asprintf("%.7g %.7g %.7g %.7g ", modelMatrix.m[i][0], modelMatrix.m[i][1], modelMatrix.m[i][2], modelMatrix.m[i][3]);
    transform.setAttribute("Matrix4x4", transformString.trimmed());

    QDomElement sampleBox = doc.createElement("SAMPLEBOX");
    root.appendChild(sampleBox);
    Vector3f minCorner = m_pointCloudWindow->GetSampleBoxMinCorner();
    Vector3f maxCorner = m_pointCloudWindow->GetSampleBoxMaxCorner();
    sampleBox.setAttribute("MinCorner", QString::asprintf("%.7g %.7g %.7g", minCorner.x, minCorner.y, minCorner.z));
    sampleBox.setAttribute("MaxCorner", QString::asprintf("%.7g %.7g %.7g", maxCorner.x, maxCorner.y, maxCorner.z));

    QList<QString> measurementDataKeys = m_MeasurementDataMap.keys();
    qStableSort(measurementDataKeys.begin(), measurementDataKeys.end(), Misc::numberInStringAsFileNameLessThan);
    for (int i = 0; i < measurementDataKeys.size(); i++)
    {
        QString absoluteFilename = measurementDataKeys[i];
        QString relativeFilename = parentFolder.relativeFilePath(absoluteFilename);
        if (relativeFilename.size() > 0) // this check probably shouldn't be necessary but I sometimes get measurement sets with empty filenames
        {
            QMap<QString, Vector3f> *points = m_MeasurementDataMap[measurementDataKeys[i]]->GetPoints();
            QDomElement measurementSet = doc.createElement("MEASUREMENTSET");
            root.appendChild(measurementSet);

            measurementSet.setAttribute("Filename", relativeFilename);

            QMap<QString, Vector3f>::const_iterator iter = points->constBegin();
            while (iter != points->constEnd())
            {
                QDomElement measurement = doc.createElement("MEASUREMENT");
                measurementSet.appendChild(measurement);
                measurement.setAttribute("Key", iter.key());
                measurement.setAttribute("X", iter.value().x);
                measurement.setAttribute("Y", iter.value().y);
                measurement.setAttribute("Z", iter.value().z);
                ++iter;
            }
        }
        m_MeasurementDataMap[measurementDataKeys[i]]->SetChanged(false);
    }

    QList<QString> sampleBoxDataKeys = m_SampleBoxDataMap.keys();
    qStableSort(sampleBoxDataKeys.begin(), sampleBoxDataKeys.end(), Misc::numberInStringAsFileNameLessThan);
    for (int i = 0; i < sampleBoxDataKeys.size(); i++)
    {
        QString absoluteFilename = sampleBoxDataKeys[i];
        QString relativeFilename = parentFolder.relativeFilePath(absoluteFilename);
        if (relativeFilename.size() > 0) // this check probably shouldn't be necessary but I sometimes get measurement sets with empty filenames
        {
            QDomElement measurementSet = doc.createElement("SAMPLEBOXDATA");
            root.appendChild(measurementSet);
            measurementSet.setAttribute("Filename", relativeFilename);
            measurementSet.setAttribute("Median", QString("%1 %2 %3").arg(m_SampleBoxDataMap[sampleBoxDataKeys[i]]->median().x).arg(m_SampleBoxDataMap[sampleBoxDataKeys[i]]->median().y).arg(m_SampleBoxDataMap[sampleBoxDataKeys[i]]->median().z));
            measurementSet.setAttribute("Mean", QString("%1 %2 %3").arg(m_SampleBoxDataMap[sampleBoxDataKeys[i]]->mean().x).arg(m_SampleBoxDataMap[sampleBoxDataKeys[i]]->mean().y).arg(m_SampleBoxDataMap[sampleBoxDataKeys[i]]->mean().z));
            measurementSet.setAttribute("NumPoints", QString("%1").arg(m_SampleBoxDataMap[sampleBoxDataKeys[i]]->nPoints()));
        }
    }

    // we need to add the processing instruction at the beginning of the document
    QString encoding("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    file.write(encoding.toUtf8());

    // and now the actual xml doc
    QString xmlString = doc.toString();
    QByteArray xmlData = xmlString.toUtf8();
    qint64 bytesWritten = file.write(xmlData);
    if (bytesWritten != xmlData.size()) qWarning("%s: xmlData.size() = %d, bytesWritten = %d", qPrintable(m_measurementsFileName), xmlData.size(), int(bytesWritten));
    file.close();
}

void MainWindow::actionOpenMeasurements_triggered()
{
    m_Preferences.getValue("LastOpenedMeasurementsFile", &m_measurementsFileName);
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open measurements data File"), m_measurementsFileName, tr("XML Files (*.xml)"), 0/*, QFileDialog::DontUseNativeDialog*/);

    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        bool unsavedData = false;
        m_measurementsFileName = fileName;
        m_Preferences.insert("LastOpenedMeasurementsFile", m_measurementsFileName);
        Matrix4f modelMatrix;
        Vector3f vec;
        QString str;

        for (QMap<QString, MeasurementData *>::const_iterator i = m_MeasurementDataMap.constBegin(); i != m_MeasurementDataMap.constEnd(); i++)
        {
            if (i.value()->GetChanged())
            {
                unsavedData = true;
                break;
            }
        }

        if (unsavedData)
        {
            int ret = QMessageBox::warning(this, tr("CloudDigitiser"),
                                           tr("You have unsaved measurements.\n"
                                              "Are you sure you want to delete the current measurements?"),
                                           QMessageBox::No | QMessageBox::Yes);
            if (ret == QMessageBox::No) return;
        }

        for (QMap<QString, MeasurementData *>::const_iterator i = m_MeasurementDataMap.constBegin(); i != m_MeasurementDataMap.constEnd(); i++) delete i.value();
        m_MeasurementDataMap.clear();

        for (QMap<QString, SampleBoxData *>::const_iterator i = m_SampleBoxDataMap.constBegin(); i != m_SampleBoxDataMap.constEnd(); i++) delete i.value();
        m_SampleBoxDataMap.clear();

        QFileInfo filenameInfo(m_measurementsFileName);
        QDir parentFolder = QDir(filenameInfo.path());

        QDomDocument doc;
        QFile file(m_measurementsFileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning("Unable to open to measurements file: %s", qPrintable(fileName));
            return;
        }
        if (!doc.setContent(&file))
        {
            qWarning("Unable to parse measurements file: %s", qPrintable(fileName));
            file.close();
            return;
        }
        file.close();

        bool setViews = false, setTransform = false, setSampleBox = false;
        Vector3f centreTL; //, centreTR, centreBL, centreBR;
        float widthTL; //, widthTR, widthBL, widthBR;
        Vector3f minCorner, maxCorner;
        QString newFileName;
        QDomElement docElem = doc.documentElement();
        if (docElem.tagName() != "CLOUDDIGITISERXML")
        {
            qWarning("Measurements file: %s is wrong type", qPrintable(fileName));
            return;
        }
        QDomNode n = docElem.firstChild();
        while(!n.isNull())
        {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if (!e.isNull())
            {
                if (e.tagName() == "VIEWS")
                {
                    setViews = true;
                    centreTL.x = e.attribute("CentreTLX").toFloat();
                    centreTL.y = e.attribute("CentreTLY").toFloat();
                    centreTL.z = e.attribute("CentreTLZ").toFloat();
                    widthTL = e.attribute("WidthTL").toFloat();
//                    centreTR.x = e.attribute("CentreTRX").toFloat();
//                    centreTR.y = e.attribute("CentreTRY").toFloat();
//                    centreTR.z = e.attribute("CentreTRZ").toFloat();
//                    widthTR = e.attribute("WidthTR").toFloat();
//                    centreBL.x = e.attribute("CentreBLX").toFloat();
//                    centreBL.y = e.attribute("CentreBLY").toFloat();
//                    centreBL.z = e.attribute("CentreBLZ").toFloat();
//                    widthBL = e.attribute("WidthBL").toFloat();
//                    centreBR.x = e.attribute("CentreBRX").toFloat();
//                    centreBR.y = e.attribute("CentreBRY").toFloat();
//                    centreBR.z = e.attribute("CentreBRZ").toFloat();
//                    widthBR = e.attribute("WidthBR").toFloat();
                }
                if (e.tagName() == "TRANSFORM")
                {
                    setTransform = true;
                    QString transformString = e.attribute("Matrix4x4");
                    QStringList transformStringTokens = transformString.split(" ", QString::SkipEmptyParts);
                    if (transformStringTokens.size() >= 16)
                    {
                        int count = 0;
                        for (int i = 0 ; i < 4 ; i++)
                        {
                            modelMatrix.m[i][0] = transformStringTokens[count++].toDouble();
                            modelMatrix.m[i][1] = transformStringTokens[count++].toDouble();
                            modelMatrix.m[i][2] = transformStringTokens[count++].toDouble();
                            modelMatrix.m[i][3] = transformStringTokens[count++].toDouble();
                        }
                    }
                }
                if (e.tagName() == "SAMPLEBOX")
                {
                    setSampleBox = true;
                    str = e.attribute("MinCorner");
                    QTextStream(&str) >> minCorner.x >> minCorner.y >> minCorner.z;
                    str = e.attribute("MaxCorner");
                    QTextStream(&str) >> maxCorner.x >> maxCorner.y >> maxCorner.z;
                }
                if (e.tagName() == "MEASUREMENTSET")
                {
                    QString filenameAttribute = e.attribute("Filename");
                    QString plyFile = parentFolder.absoluteFilePath(filenameAttribute);
                    if (filenameAttribute.size() > 0)
                    {
                        if (newFileName.length() == 0) newFileName = plyFile;
                        MeasurementData *measurementData = new MeasurementData();
                        m_MeasurementDataMap[plyFile] = measurementData;

                        for(QDomNode n2 = e.firstChild(); !n2.isNull(); n2 = n2.nextSibling())
                        {
                            QDomElement e2 = n2.toElement();
                            if (!e2.isNull())
                            {
                                if (e2.tagName() == "MEASUREMENT")
                                {
                                    QString key = e2.attribute("Key");
                                    float x = e2.attribute("X").toFloat();
                                    float y = e2.attribute("Y").toFloat();
                                    float z = e2.attribute("Z").toFloat();
                                    measurementData->Insert(key, x, y, z);
                                    measurementData->SetChanged(false);
                                }
                            }
                        }
                    }
                }
                if (e.tagName() == "SAMPLEBOXDATA")
                {
                    QString filenameAttribute = e.attribute("Filename");
                    QString plyFile = parentFolder.absoluteFilePath(filenameAttribute);
                    if (filenameAttribute.size() > 0)
                    {
                        if (newFileName.length() == 0) newFileName = plyFile;
                        SampleBoxData *sampleBoxData = new SampleBoxData();
                        m_SampleBoxDataMap[plyFile] = sampleBoxData;
                        sampleBoxData->setNPoints(e.attribute("NumPoints").toInt());
                        str = e.attribute("Mean");
                        QTextStream(&str) >> vec.x >> vec.y >> vec.z;
                        sampleBoxData->setMean(vec);
                        str = e.attribute("Median");
                        QTextStream(&str) >> vec.x >> vec.y >> vec.z;
                        sampleBoxData->setMedian(vec);
                    }
                }
            }
            n = n.nextSibling();
        }

        if (m_MeasurementDataMap.contains(m_fileName) == false) m_MeasurementDataMap[m_fileName] = new MeasurementData();
        m_pointCloudWindow->SetMeasurementData(m_MeasurementDataMap[m_fileName]);

        if (newFileName.length())
        {
            m_fileName = newFileName;
            OpenFile();
        }

        if (setViews)
        {
            m_pointCloudWindow->SetCentrePoint(centreTL);
            m_pointCloudWindow->SetWidth(widthTL);
        }

        if (setSampleBox)
        {
            m_pointCloudWindow->SetSampleBox(minCorner, maxCorner);
        }

        if (setTransform)
            m_pointCloud->SetTransformation(modelMatrix);

        UpdateModel(true);
    }
}

void MainWindow::actionImportFromMeasurements_triggered()
{
    m_Preferences.getValue("LastOpenedMeasurementsFile", &m_measurementsFileName);
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open measurements data File"), m_measurementsFileName, tr("XML Files (*.xml)"), 0/*, QFileDialog::DontUseNativeDialog*/);

    if (fileName.isNull() == false)
    {
        QDir::setCurrent(QFileInfo(fileName).absolutePath());
        bool unsavedData = false;
        m_measurementsFileName = fileName;
        m_Preferences.insert("LastOpenedMeasurementsFile", m_measurementsFileName);
        Matrix4f modelMatrix;
        Vector3f vec;
        QString str;

        for (QMap<QString, MeasurementData *>::const_iterator i = m_MeasurementDataMap.constBegin(); i != m_MeasurementDataMap.constEnd(); i++)
        {
            if (i.value()->GetChanged())
            {
                unsavedData = true;
                break;
            }
        }

        if (unsavedData)
        {
            int ret = QMessageBox::warning(this, tr("CloudDigitiser"),
                                           tr("You have unsaved measurements.\n"
                                              "Are you sure you want to delete the current measurements?"),
                                           QMessageBox::No | QMessageBox::Yes);
            if (ret == QMessageBox::No) return;
        }

        for (QMap<QString, MeasurementData *>::const_iterator i = m_MeasurementDataMap.constBegin(); i != m_MeasurementDataMap.constEnd(); i++) delete i.value();
        m_MeasurementDataMap.clear();

        for (QMap<QString, SampleBoxData *>::const_iterator i = m_SampleBoxDataMap.constBegin(); i != m_SampleBoxDataMap.constEnd(); i++) delete i.value();
        m_SampleBoxDataMap.clear();

        QFileInfo filenameInfo(m_measurementsFileName);
        QDir parentFolder = QDir(filenameInfo.path());

        QDomDocument doc;
        QFile file(m_measurementsFileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning("Unable to open to measurements file: %s", qPrintable(fileName));
            return;
        }
        if (!doc.setContent(&file))
        {
            qWarning("Unable to parse measurements file: %s", qPrintable(fileName));
            file.close();
            return;
        }
        file.close();

        bool setViews = false, setTransform = false, setSampleBox = false;
        Vector3f centreTL; //, centreTR, centreBL, centreBR;
        float widthTL; //, widthTR, widthBL, widthBR;
        Vector3f minCorner, maxCorner;
        QString newFileName;
        QDomElement docElem = doc.documentElement();
        if (docElem.tagName() != "CLOUDDIGITISERXML")
        {
            qWarning("Measurements file: %s is wrong type", qPrintable(fileName));
            return;
        }
        QDomNode n = docElem.firstChild();
        while(!n.isNull())
        {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if (!e.isNull())
            {
                if (e.tagName() == "VIEWS")
                {
                    setViews = true;
                    centreTL.x = e.attribute("CentreTLX").toFloat();
                    centreTL.y = e.attribute("CentreTLY").toFloat();
                    centreTL.z = e.attribute("CentreTLZ").toFloat();
                    widthTL = e.attribute("WidthTL").toFloat();
//                    centreTR.x = e.attribute("CentreTRX").toFloat();
//                    centreTR.y = e.attribute("CentreTRY").toFloat();
//                    centreTR.z = e.attribute("CentreTRZ").toFloat();
//                    widthTR = e.attribute("WidthTR").toFloat();
//                    centreBL.x = e.attribute("CentreBLX").toFloat();
//                    centreBL.y = e.attribute("CentreBLY").toFloat();
//                    centreBL.z = e.attribute("CentreBLZ").toFloat();
//                    widthBL = e.attribute("WidthBL").toFloat();
//                    centreBR.x = e.attribute("CentreBRX").toFloat();
//                    centreBR.y = e.attribute("CentreBRY").toFloat();
//                    centreBR.z = e.attribute("CentreBRZ").toFloat();
//                    widthBR = e.attribute("WidthBR").toFloat();
                }
                if (e.tagName() == "TRANSFORM")
                {
                    setTransform = true;
                    QString transformString = e.attribute("Matrix4x4");
                    QStringList transformStringTokens = transformString.split(" ", QString::SkipEmptyParts);
                    if (transformStringTokens.size() >= 16)
                    {
                        int count = 0;
                        for (int i = 0 ; i < 4 ; i++)
                        {
                            modelMatrix.m[i][0] = transformStringTokens[count++].toDouble();
                            modelMatrix.m[i][1] = transformStringTokens[count++].toDouble();
                            modelMatrix.m[i][2] = transformStringTokens[count++].toDouble();
                            modelMatrix.m[i][3] = transformStringTokens[count++].toDouble();
                        }
                    }
                }
                if (e.tagName() == "SAMPLEBOX")
                {
                    setSampleBox = true;
                    str = e.attribute("MinCorner");
                    QTextStream(&str) >> minCorner.x >> minCorner.y >> minCorner.z;
                    str = e.attribute("MaxCorner");
                    QTextStream(&str) >> maxCorner.x >> maxCorner.y >> maxCorner.z;
                }
            }
            n = n.nextSibling();
        }

        if (m_MeasurementDataMap.contains(m_fileName) == false) m_MeasurementDataMap[m_fileName] = new MeasurementData();
        m_pointCloudWindow->SetMeasurementData(m_MeasurementDataMap[m_fileName]);

        if (newFileName.length())
        {
            m_fileName = newFileName;
            OpenFile();
        }

        if (setViews)
        {
            m_pointCloudWindow->SetCentrePoint(centreTL);
            m_pointCloudWindow->SetWidth(widthTL);
        }

        if (setSampleBox)
        {
            m_pointCloudWindow->SetSampleBox(minCorner, maxCorner);
        }

        if (setTransform)
            m_pointCloud->SetTransformation(modelMatrix);

        UpdateModel(true);
    }
}

void MainWindow::actionNewMeasurements_triggered()
{
    if (m_pointCloud == 0) return;

    bool unsavedData = false;
    QMap<QString, MeasurementData *>::const_iterator i = m_MeasurementDataMap.constBegin();
    while (i != m_MeasurementDataMap.constEnd())
    {
        if (i.value()->GetChanged())
        {
            unsavedData = true;
            break;
        }
        ++i;
    }

    if (unsavedData)
    {
        int ret = QMessageBox::warning(this, tr("CloudDigitiser"),
                                       tr("You have unsaved measurements.\n"
                                          "Are you sure you want to delete the current measurements?"),
                                       QMessageBox::No | QMessageBox::Yes);
        if (ret == QMessageBox::No) return;
    }

    i = m_MeasurementDataMap.constBegin();
    while (i != m_MeasurementDataMap.constEnd())
    {
        delete i.value();
        ++i;
    }
    m_MeasurementDataMap.clear();
    m_MeasurementDataMap[m_fileName] = new MeasurementData();

    for (QMap<QString, SampleBoxData *>::const_iterator i = m_SampleBoxDataMap.constBegin(); i != m_SampleBoxDataMap.constEnd(); i++) delete i.value();
    m_SampleBoxDataMap.clear();

    m_pointCloudWindow->SetMeasurementData(m_MeasurementDataMap[m_fileName]);
    UpdateModel(false);
}

void MainWindow::actionBatchTransform_triggered()
{
    BatchProcessDialog batchProcessDialog(this);

    int status = batchProcessDialog.exec();

    if (status == QDialog::Accepted)
    {
        QString sourceFolder = batchProcessDialog.GetSourceFolder();
        QString destinationFolder = batchProcessDialog.GetDestinationFolder();
        QString transformationFile = batchProcessDialog.GetTransformationFile();
        BatchProcess batchProcess;
        batchProcess.SetInputs(sourceFolder, destinationFolder, transformationFile);
        batchProcess.Process();
    }
}

void MainWindow::actionPreferences_triggered()
{
    PreferencesDialog preferencesDialog(this);

    preferencesDialog.setPreferences(m_Preferences);

    int status = preferencesDialog.exec();

    if (status == QDialog::Accepted)
    {
        m_Preferences = preferencesDialog.preferences();
        m_Preferences.Write();
        m_pointCloudWindow->ApplySettings(m_Preferences);
        UpdateModel(true);
    }
}

void MainWindow::actionCalibratePoint_triggered()
{
    if (m_pointCloud == 0) return;

    Vector3f worldPoint(1.f, 0.f, 0.f);
    bool uniformScale = false;
    Matrix4f trans = m_pointCloud->GetTransformation();
    // the general case for extracting the scale uses SVD but this will work unless the scaling has become complicated (e.g. repeated scaling)
    float sx = Vector3f(trans.m[0][0], trans.m[0][1], trans.m[0][2]).Magnitude();
    float sy = Vector3f(trans.m[1][0], trans.m[1][1], trans.m[1][2]).Magnitude();
    float sz = Vector3f(trans.m[2][0], trans.m[2][1], trans.m[2][2]).Magnitude();

    CalibrateDialog calibrateDialog(this);
    Vector3f currentPoint = m_pointCloud->GetLastPoint();
    QMap<QString, QVariant> values;
    values.insert("doubleSpinBoxModelX", currentPoint.x);
    values.insert("doubleSpinBoxModelY", currentPoint.y);
    values.insert("doubleSpinBoxModelZ", currentPoint.z);
    values.insert("doubleSpinBoxWorldX", worldPoint.x);
    values.insert("doubleSpinBoxWorldY", worldPoint.y);
    values.insert("doubleSpinBoxWorldZ", worldPoint.z);
    values.insert("doubleSpinBoxXScale", sx);
    values.insert("doubleSpinBoxYScale", sy);
    values.insert("doubleSpinBoxZScale", sz);
    values.insert("checkBoxUniformScale", uniformScale);
    calibrateDialog.SetValues(values);

    int status = calibrateDialog.exec();

    if (status == QDialog::Accepted)
    {
        calibrateDialog.GetValues(&values);
        sx = values["doubleSpinBoxXScale"].toFloat();
        sy = values["doubleSpinBoxYScale"].toFloat();
        sz = values["doubleSpinBoxZScale"].toFloat();
        if (values["checkBoxUniformScale"].toBool()) { sy = sx; sz = sx; }
        m_pointCloud->Scale(sx, sy, sz);
        statusBar()->showMessage(QString("Scale factor (%1,%2,%3) applied").arg(sx).arg(sy).arg(sz));
        UpdateModel(true);
    }
}

void MainWindow::actionNextFile_triggered()
{
    if (m_pointCloud == 0) return;
    QFileInfo info(m_fileName);
    QString matchName = info.absoluteFilePath();

    // read all the ply files in folder and loop
    QDir dir = info.absoluteDir();
    QStringList nameFilters;
    nameFilters << "*.PLY" << "*.ply";
    QDir::Filters filter = QDir::Files;
    QFileInfoList info_list = dir.entryInfoList(nameFilters, filter, QDir::Name);
    qStableSort(info_list.begin(), info_list.end(), Misc::numberInFileNameLessThan);
    for (int i = 0; i < info_list.count() - 1; i++)
    {
        if (info_list.at(i).absoluteFilePath() == matchName)
        {
            m_fileName = info_list.at(i + 1).absoluteFilePath();
            OpenFile();
            UpdateModel(true);
            return;
        }
    }
}


void MainWindow::actionPreviousFile_triggered()
{
    if (m_pointCloud == 0) return;
    QFileInfo info(m_fileName);
    QString matchName = info.absoluteFilePath();

    // read all the ply files in folder and loop
    QDir dir = info.absoluteDir();
    QStringList nameFilters;
    nameFilters << "*.PLY" << "*.ply";
    QDir::Filters filter = QDir::Files;
    QFileInfoList info_list = dir.entryInfoList(nameFilters, filter, QDir::Name);
    qStableSort(info_list.begin(), info_list.end(), Misc::numberInFileNameLessThan);
    for (int i = 1; i < info_list.count(); i++)
    {
        if (info_list.at(i).absoluteFilePath() == matchName)
        {
            m_fileName = info_list.at(i - 1).absoluteFilePath();
            OpenFile();
            UpdateModel(true);
            return;
        }
    }
}

void MainWindow::actionDefaultView_triggered()
{
    if (m_pointCloud == 0) return;
    DefaultView();
    UpdateModel(false);
}

void MainWindow::actionRectSelect_triggered()
{
    ui->actionRectSelect->setEnabled(false);
    ui->actionSphereSelect->setEnabled(true);
    ui->actionLineSelect->setEnabled(true);
    ui->actionMoveTool->setEnabled(true);
    ui->actionNudge->setEnabled(true);
    m_pointCloudWindow->SetToolMode(PointCloudWindow::rectangleSelect);
    UpdateModel(false);
}

void MainWindow::actionSphereSelect_triggered()
{
    ui->actionRectSelect->setEnabled(true);
    ui->actionSphereSelect->setEnabled(false);
    ui->actionLineSelect->setEnabled(true);
    ui->actionMoveTool->setEnabled(true);
    ui->actionNudge->setEnabled(true);
    m_pointCloudWindow->SetToolMode(PointCloudWindow::sphereSelect);
    UpdateModel(false);
}

void MainWindow::actionLineSelect_triggered()
{
    ui->actionRectSelect->setEnabled(true);
    ui->actionSphereSelect->setEnabled(true);
    ui->actionLineSelect->setEnabled(false);
    ui->actionMoveTool->setEnabled(true);
    ui->actionNudge->setEnabled(true);
    m_pointCloudWindow->SetToolMode(PointCloudWindow::lineSelect);
    UpdateModel(false);
}


void MainWindow::actionMoveTool_triggered()
{
    ui->actionRectSelect->setEnabled(true);
    ui->actionSphereSelect->setEnabled(true);
    ui->actionLineSelect->setEnabled(true);
    ui->actionMoveTool->setEnabled(false);
    ui->actionNudge->setEnabled(true);
    m_pointCloudWindow->SetToolMode(PointCloudWindow::moveTool);
    UpdateModel(false);
}

void MainWindow::actionNudge_triggered()
{
    ui->actionRectSelect->setEnabled(true);
    ui->actionSphereSelect->setEnabled(true);
    ui->actionLineSelect->setEnabled(true);
    ui->actionMoveTool->setEnabled(true);
    ui->actionNudge->setEnabled(false);
    m_pointCloudWindow->SetToolMode(PointCloudWindow::nudgeTool);
    UpdateModel(false);
}

void MainWindow::actionViewLeft_triggered()
{
    m_pointCloudWindow->SetCameraRotationAxisAngle(0, 0, 1, -90);
    UpdateModel(false);
}

void MainWindow::actionViewRight_triggered()
{
    m_pointCloudWindow->SetCameraRotationAxisAngle(0, 0, 1, 90);
    UpdateModel(false);
}

void MainWindow::actionViewTop_triggered()
{
    // Quaternion q = MakeQuaternionFromAxisAngle(1, 0, 0, 90) * MakeQuaternionFromAxisAngle(0, 0, 1, 180);
    // m_pointCloudWindow->SetCameraRotation(q.x, q.y, q.z, q.w);
    m_pointCloudWindow->SetCameraRotationAxisAngle(1, 0, 0, -90);
    UpdateModel(false);
}

void MainWindow::actionViewBottom_triggered()
{
    // Quaternion q = MakeQuaternionFromAxisAngle(1, 0, 0, -90) * MakeQuaternionFromAxisAngle(0, 0, 1, 180);
    // m_pointCloudWindow->SetCameraRotation(q.x, q.y, q.z, q.w);
    m_pointCloudWindow->SetCameraRotationAxisAngle(1, 0, 0, 90);
    UpdateModel(false);
}

void MainWindow::actionViewFront_triggered()
{
    m_pointCloudWindow->SetCameraRotationAxisAngle(0, 0, 1, 0);
    UpdateModel(false);
}

void MainWindow::actionViewBack_triggered()
{
    m_pointCloudWindow->SetCameraRotationAxisAngle(0, 0, 1, 180);
    UpdateModel(false);
}

void MainWindow::actionResetTransformation_triggered()
{
    Matrix4f modelMatrix;
    modelMatrix.InitIdentity();
    m_pointCloud->SetTransformation(modelMatrix);
    statusBar()->showMessage(QString("Setting transformation to identity matrix"));
    UpdateModel(true);
}

void MainWindow::actionDefineSampleBox_triggered()
{
    QMap<QString, QVariant> values;
    values.insert("doubleSpinBoxMinX", m_pointCloudWindow->GetSampleBoxMinCorner().x);
    values.insert("doubleSpinBoxMinY", m_pointCloudWindow->GetSampleBoxMinCorner().y);
    values.insert("doubleSpinBoxMinZ", m_pointCloudWindow->GetSampleBoxMinCorner().z);
    values.insert("doubleSpinBoxMaxX", m_pointCloudWindow->GetSampleBoxMaxCorner().x);
    values.insert("doubleSpinBoxMaxY", m_pointCloudWindow->GetSampleBoxMaxCorner().y);
    values.insert("doubleSpinBoxMaxZ", m_pointCloudWindow->GetSampleBoxMaxCorner().z);
    m_defineSampleBoxDialog->SetValues(values);
    m_defineSampleBoxDialog->show();
}

void MainWindow::actionProcessSampleBox_triggered()
{
    ProcessSampleBox processSampleBox;
    QFileInfo info(m_fileName);
    processSampleBox.SetInputs(info.absolutePath(), m_pointCloud->GetTransformation(), m_pointCloudWindow->GetSampleBoxMinCorner(), m_pointCloudWindow->GetSampleBoxMaxCorner(), &m_SampleBoxDataMap);
    processSampleBox.Process();
    statusBar()->showMessage(QString("Sample box processing finished"));
    UpdateModel(true);
}

int MainWindow::SaveFile()
{
    int st = m_pointCloud->ExportPLY(QFile::encodeName(m_fileName));
    if (st)
    {
        return __LINE__;
    }
    // put the filename as a title
    if (m_fileName.size() <= 256) setWindowTitle(m_fileName);
    else setWindowTitle(QString("...") + m_fileName.right(256));

    // set last saved file
    m_Preferences.insert("LastSavedFile", m_fileName);

    return 0;
}

int MainWindow::OpenFile()
{
    Matrix4f modelMatrix;
    modelMatrix.InitIdentity();
    if (m_pointCloud)
    {
        modelMatrix = m_pointCloud->GetTransformation();
        delete m_pointCloud;
    }

    m_pointCloud = new PointCloud();
    int st = m_pointCloud->ImportPLY(QFile::encodeName(m_fileName));
    if (st)
    {
        delete m_pointCloud;
        m_pointCloud = 0;
        return __LINE__;
    }
    m_pointCloud->SetTransformation(modelMatrix);

    // put the filename as a title
    if (m_fileName.size() <= 256) setWindowTitle(m_fileName);
    else setWindowTitle(QString("...") + m_fileName.right(256));

    // set last opened file
    m_Preferences.insert("LastOpenedPLYFile", m_fileName);

    if (m_MeasurementDataMap.contains(m_fileName) == false) m_MeasurementDataMap[m_fileName] = new MeasurementData();

    m_pointCloudWindow->initializeIrrlicht();
    m_pointCloudWindow->initialiseScene();

    m_pointCloudWindow->SetPointCloud(m_pointCloud);
    m_pointCloudWindow->SetMeasurementData(m_MeasurementDataMap[m_fileName]);
    m_pointCloudWindow->SetMaxPointsToRender(m_Preferences.settingsItem("MaxDisplayPixels").value.toInt());

    statusBar()->showMessage(QString("%1 Loaded %2 points").arg(m_fileName).arg(m_pointCloud->GetNPoints()));
    return 0;
}

void MainWindow::DefaultView()
{
    if (m_pointCloud == 0) return;

    Vector3f minBound = m_pointCloud->GetMinBound();
    Vector3f maxBound = m_pointCloud->GetMaxBound();
    Vector3f centrePoint = (maxBound + minBound) * 0.5;
    float aspect = float(m_pointCloudWindow->width()) / float(m_pointCloudWindow->height());
    float maxWidth = aspect * MAX(MAX(maxBound.x - minBound.x, maxBound.y - minBound.y), maxBound.z - minBound.z);

    m_pointCloudWindow->SetCentrePoint(centrePoint);
    m_pointCloudWindow->SetWidth(maxWidth);
}

void MainWindow::UpdateModel(bool reloadBuffers)
{
    m_pointCloudWindow->UpdateModel(reloadBuffers);
    EnableActions();
}

void MainWindow::SetStatusString(QString s)
{
    statusBar()->showMessage(s);
}

void MainWindow::EnableActions()
{
    ui->actionOpen->setEnabled(true);
    ui->actionQuit->setEnabled(true);
    ui->actionAbout->setEnabled(true);
    ui->actionFitLine->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetNSelected() > 0));
    ui->actionFitPlane->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetNSelected() > 0));
    ui->actionFitPoint->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetNSelected() > 0));
    ui->actionApplyMatrix->setEnabled(m_pointCloud != 0);
    ui->actionImportFromMeasurementsFile->setEnabled(m_pointCloud != 0);
    ui->actionSaveAs->setEnabled(m_pointCloud != 0);
    ui->actionSaveAsMatrix->setEnabled(m_pointCloud != 0);
    ui->actionBatchTransform->setEnabled(true);
    ui->actionPrevFile->setEnabled(m_pointCloud != 0);
    ui->actionNextFile->setEnabled(m_pointCloud != 0);
    ui->actionDefaultView->setEnabled(m_pointCloud != 0);
    ui->actionRotateAboutX->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionRotateAboutY->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionRotateAboutZ->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionRotateToX->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionRotateToY->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionRotateToZ->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionResetTransformation->setEnabled(m_pointCloud != 0);
    ui->actionCalibratePoint->setEnabled(m_pointCloud != 0);
    ui->actionSetOrigin->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetPointFitted()));
    ui->actionRectSelect->setEnabled(m_pointCloud != 0 && m_pointCloudWindow->GetToolMode() != PointCloudWindow::rectangleSelect);
    ui->actionSphereSelect->setEnabled(m_pointCloud != 0 && m_pointCloudWindow->GetToolMode() != PointCloudWindow::sphereSelect);
    ui->actionLineSelect->setEnabled(m_pointCloud != 0 && m_pointCloudWindow->GetToolMode() != PointCloudWindow::lineSelect);
    ui->actionMoveTool->setEnabled(m_pointCloud != 0 && m_pointCloudWindow->GetToolMode() != PointCloudWindow::moveTool);
    ui->actionNudge->setEnabled(m_pointCloud != 0 && m_pointCloudWindow->GetToolMode() != PointCloudWindow::nudgeTool);
    ui->actionSaveMeasurements->setEnabled(m_pointCloud != 0);
    ui->actionPreferences->setEnabled(true);
    ui->actionDefineSampleBox->setEnabled(m_pointCloud != 0);
    ui->actionProcessSampleBox->setEnabled(m_pointCloud != 0 && m_pointCloudWindow->GetSampleBoxValid());
    ui->actionRotateToXAboutZ->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionRotateToYAboutZ->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionRotateToZAboutX->setEnabled((m_pointCloud != 0) && (m_pointCloud->GetLineFitted()));
    ui->actionNewMeasurements->setEnabled(m_pointCloud != 0);
    ui->actionOpenMeasurements->setEnabled(true);
    ui->actionSaveMeasurementsAs->setEnabled(m_pointCloud != 0);
    ui->actionViewLeft->setEnabled(m_pointCloud != 0);
    ui->actionViewRight->setEnabled(m_pointCloud != 0);
    ui->actionViewTop->setEnabled(m_pointCloud != 0);
    ui->actionViewBottom->setEnabled(m_pointCloud != 0);
    ui->actionViewFront->setEnabled(m_pointCloud != 0);
    ui->actionViewBack->setEnabled(m_pointCloud != 0);
}

void MainWindow::DefineSampleBoxDialogData()
{
    QMap<QString, QVariant> values;
    m_defineSampleBoxDialog->GetValues(&values);
    m_pointCloudWindow->SetSampleBox(Vector3f(values["doubleSpinBoxMinX"].toFloat(), values["doubleSpinBoxMinY"].toFloat(), values["doubleSpinBoxMinZ"].toFloat()),
            Vector3f(values["doubleSpinBoxMaxX"].toFloat(), values["doubleSpinBoxMaxY"].toFloat(), values["doubleSpinBoxMaxZ"].toFloat()));
    UpdateModel(false);
}
