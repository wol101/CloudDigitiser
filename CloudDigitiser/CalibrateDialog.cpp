#include <cmath>

#include "CalibrateDialog.h"
#include "ui_CalibrateDialog.h"

CalibrateDialog::CalibrateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrateDialog)
{
    ui->setupUi(this);

    ui->doubleSpinBoxStepSize->setDecimals(9);
    ui->doubleSpinBoxStepSize->setMinimum(1e-9);
    ui->doubleSpinBoxStepSize->setMaximum(1);
    StepChanged(ui->doubleSpinBoxStepSize->value());
    DecimalsChanged(ui->spinBoxDecimals->value());

    QObject::connect(ui->doubleSpinBoxStepSize, SIGNAL(valueChanged(double)), this, SLOT(StepChanged(double)));
    QObject::connect(ui->spinBoxDecimals, SIGNAL(valueChanged(int)), this, SLOT(DecimalsChanged(int)));

    QObject::connect(ui->doubleSpinBoxModelX, SIGNAL(valueChanged(double)), this, SLOT(ModelXChanged(double)));
    QObject::connect(ui->doubleSpinBoxModelY, SIGNAL(valueChanged(double)), this, SLOT(ModelYChanged(double)));
    QObject::connect(ui->doubleSpinBoxModelZ, SIGNAL(valueChanged(double)), this, SLOT(ModelZChanged(double)));
    QObject::connect(ui->doubleSpinBoxWorldX, SIGNAL(valueChanged(double)), this, SLOT(WorldXChanged(double)));
    QObject::connect(ui->doubleSpinBoxWorldY, SIGNAL(valueChanged(double)), this, SLOT(WorldYChanged(double)));
    QObject::connect(ui->doubleSpinBoxWorldZ, SIGNAL(valueChanged(double)), this, SLOT(WorldZChanged(double)));
    QObject::connect(ui->doubleSpinBoxXScale, SIGNAL(valueChanged(double)), this, SLOT(XScaleChanged(double)));
    QObject::connect(ui->doubleSpinBoxYScale, SIGNAL(valueChanged(double)), this, SLOT(YScaleChanged(double)));
    QObject::connect(ui->doubleSpinBoxZScale, SIGNAL(valueChanged(double)), this, SLOT(ZScaleChanged(double)));
    QObject::connect(ui->checkBoxUniformScale, SIGNAL(stateChanged(int)), this, SLOT(UniformScaleChanged(int)));
}

CalibrateDialog::~CalibrateDialog()
{
    delete ui;
}

void CalibrateDialog::SetValues(const QMap<QString, QVariant> &values)
{
    ui->doubleSpinBoxModelX->setValue(values.value("doubleSpinBoxModelX", 0).toDouble());
    ui->doubleSpinBoxModelY->setValue(values.value("doubleSpinBoxModelY", 0).toDouble());
    ui->doubleSpinBoxModelZ->setValue(values.value("doubleSpinBoxModelZ", 0).toDouble());
    ui->doubleSpinBoxWorldX->setValue(values.value("doubleSpinBoxWorldX", 0).toDouble());
    ui->doubleSpinBoxWorldY->setValue(values.value("doubleSpinBoxWorldY", 0).toDouble());
    ui->doubleSpinBoxWorldZ->setValue(values.value("doubleSpinBoxWorldZ", 0).toDouble());
    ui->doubleSpinBoxXScale->setValue(values.value("doubleSpinBoxXScale", 0).toDouble());
    ui->doubleSpinBoxYScale->setValue(values.value("doubleSpinBoxYScale", 0).toDouble());
    ui->doubleSpinBoxZScale->setValue(values.value("doubleSpinBoxZScale", 0).toDouble());
    ui->checkBoxUniformScale->setChecked(values.value("checkBoxUniformScale", 0).toBool());
}

void CalibrateDialog::GetValues(QMap<QString, QVariant> *values)
{
    values->insert("doubleSpinBoxModelX", ui->doubleSpinBoxModelX->value());
    values->insert("doubleSpinBoxModelY", ui->doubleSpinBoxModelY->value());
    values->insert("doubleSpinBoxModelZ", ui->doubleSpinBoxModelZ->value());
    values->insert("doubleSpinBoxWorldX", ui->doubleSpinBoxWorldX->value());
    values->insert("doubleSpinBoxWorldY", ui->doubleSpinBoxWorldY->value());
    values->insert("doubleSpinBoxWorldZ", ui->doubleSpinBoxWorldZ->value());
    values->insert("doubleSpinBoxXScale", ui->doubleSpinBoxXScale->value());
    values->insert("doubleSpinBoxYScale", ui->doubleSpinBoxYScale->value());
    values->insert("doubleSpinBoxZScale", ui->doubleSpinBoxZScale->value());
    values->insert("checkBoxUniformScale", ui->checkBoxUniformScale->isChecked());
}

void CalibrateDialog::StepChanged(double d)
{
    ui->doubleSpinBoxModelX->setSingleStep(d);
    ui->doubleSpinBoxModelY->setSingleStep(d);
    ui->doubleSpinBoxModelZ->setSingleStep(d);
    ui->doubleSpinBoxWorldX->setSingleStep(d);
    ui->doubleSpinBoxWorldY->setSingleStep(d);
    ui->doubleSpinBoxWorldZ->setSingleStep(d);
    ui->doubleSpinBoxXScale->setSingleStep(d);
    ui->doubleSpinBoxYScale->setSingleStep(d);
    ui->doubleSpinBoxZScale->setSingleStep(d);
}

void CalibrateDialog::DecimalsChanged(int i)
{
    ui->doubleSpinBoxModelX->setDecimals(i);
    ui->doubleSpinBoxModelY->setDecimals(i);
    ui->doubleSpinBoxModelZ->setDecimals(i);
    ui->doubleSpinBoxWorldX->setDecimals(i);
    ui->doubleSpinBoxWorldY->setDecimals(i);
    ui->doubleSpinBoxWorldZ->setDecimals(i);
    ui->doubleSpinBoxXScale->setDecimals(i);
    ui->doubleSpinBoxYScale->setDecimals(i);
    ui->doubleSpinBoxZScale->setDecimals(i);
}

void CalibrateDialog::ModelXChanged(double d)
{
    if (std::fabs(d) < std::pow(10, -ui->spinBoxDecimals->value())) d = std::copysign(std::pow(10, -ui->spinBoxDecimals->value()), d);
    double scale = d / ui->doubleSpinBoxWorldX->value();
    ui->doubleSpinBoxXScale->setValue(scale);
}

void CalibrateDialog::ModelYChanged(double d)
{
    if (std::fabs(d) < std::pow(10, -ui->spinBoxDecimals->value())) d = std::copysign(std::pow(10, -ui->spinBoxDecimals->value()), d);
    double scale = d / ui->doubleSpinBoxWorldY->value();
    ui->doubleSpinBoxYScale->setValue(scale);
}

void CalibrateDialog::ModelZChanged(double d)
{
    if (std::fabs(d) < std::pow(10, -ui->spinBoxDecimals->value())) d = std::copysign(std::pow(10, -ui->spinBoxDecimals->value()), d);
    double scale = d / ui->doubleSpinBoxWorldY->value();
    ui->doubleSpinBoxZScale->setValue(scale);
}

void CalibrateDialog::WorldXChanged(double d)
{
    double scale = ui->doubleSpinBoxModelX->value() / d;
    ui->doubleSpinBoxXScale->setValue(scale);
}

void CalibrateDialog::WorldYChanged(double d)
{
    double scale = ui->doubleSpinBoxModelX->value() / d;
    ui->doubleSpinBoxYScale->setValue(scale);
}

void CalibrateDialog::WorldZChanged(double d)
{
    double scale = ui->doubleSpinBoxModelX->value() / d;
    ui->doubleSpinBoxZScale->setValue(scale);
}

void CalibrateDialog::XScaleChanged(double d)
{
    if ( ui->checkBoxUniformScale->isChecked())
    {
        ui->doubleSpinBoxYScale->setValue(d);
        ui->doubleSpinBoxZScale->setValue(d);
    }
}

void CalibrateDialog::YScaleChanged(double d)
{
    if ( ui->checkBoxUniformScale->isChecked())
    {
        ui->doubleSpinBoxXScale->setValue(d);
        ui->doubleSpinBoxZScale->setValue(d);
    }
}

void CalibrateDialog::ZScaleChanged(double d)
{
    if ( ui->checkBoxUniformScale->isChecked())
    {
        ui->doubleSpinBoxXScale->setValue(d);
        ui->doubleSpinBoxYScale->setValue(d);
    }
}

void CalibrateDialog::UniformScaleChanged(int i)
{
    if ( ui->checkBoxUniformScale->isChecked())
    {
        double scale = ui->doubleSpinBoxXScale->value();
        ui->doubleSpinBoxYScale->setValue(scale);
        ui->doubleSpinBoxZScale->setValue(scale);
    }
}

