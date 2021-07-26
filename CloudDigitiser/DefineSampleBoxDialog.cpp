#include "DefineSampleBoxDialog.h"
#include "ui_DefineSampleBoxDialog.h"

DefineSampleBoxDialog::DefineSampleBoxDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DefineSampleBoxDialog)
{
    ui->setupUi(this);

    IncrementChanged(ui->doubleSpinBoxIncrement->value());
    RangeChanged(ui->doubleSpinBoxRange->value());
    DecimalsChanged(ui->spinBoxDecimals->value());

    QObject::connect(ui->doubleSpinBoxIncrement, SIGNAL(valueChanged(double)), this, SLOT(IncrementChanged(double)));
    QObject::connect(ui->doubleSpinBoxRange, SIGNAL(valueChanged(double)), this, SLOT(RangeChanged(double)));
    QObject::connect(ui->spinBoxDecimals, SIGNAL(valueChanged(int)), this, SLOT(DecimalsChanged(int)));

    QObject::connect(ui->doubleSpinBoxMaxX, SIGNAL(valueChanged(double)), this, SLOT(MaxXChanged(double)));
    QObject::connect(ui->doubleSpinBoxMaxY, SIGNAL(valueChanged(double)), this, SLOT(MaxYChanged(double)));
    QObject::connect(ui->doubleSpinBoxMaxZ, SIGNAL(valueChanged(double)), this, SLOT(MaxZChanged(double)));
    QObject::connect(ui->doubleSpinBoxMinX, SIGNAL(valueChanged(double)), this, SLOT(MinXChanged(double)));
    QObject::connect(ui->doubleSpinBoxMinY, SIGNAL(valueChanged(double)), this, SLOT(MinYChanged(double)));
    QObject::connect(ui->doubleSpinBoxMinZ, SIGNAL(valueChanged(double)), this, SLOT(MinZChanged(double)));
}

DefineSampleBoxDialog::~DefineSampleBoxDialog()
{
    delete ui;
}

void DefineSampleBoxDialog::SetValues(const QMap<QString, QVariant> &values)
{
    ui->doubleSpinBoxMinX->setValue(values.value("doubleSpinBoxMinX", 0).toDouble());
    ui->doubleSpinBoxMinY->setValue(values.value("doubleSpinBoxMinY", 0).toDouble());
    ui->doubleSpinBoxMinZ->setValue(values.value("doubleSpinBoxMinZ", 0).toDouble());
    ui->doubleSpinBoxMaxX->setValue(values.value("doubleSpinBoxMaxX", 0).toDouble());
    ui->doubleSpinBoxMaxY->setValue(values.value("doubleSpinBoxMaxY", 0).toDouble());
    ui->doubleSpinBoxMaxZ->setValue(values.value("doubleSpinBoxMaxZ", 0).toDouble());
}

void DefineSampleBoxDialog::GetValues(QMap<QString, QVariant> *values)
{
    values->insert("doubleSpinBoxMinX", ui->doubleSpinBoxMinX->value());
    values->insert("doubleSpinBoxMinY", ui->doubleSpinBoxMinY->value());
    values->insert("doubleSpinBoxMinZ", ui->doubleSpinBoxMinZ->value());
    values->insert("doubleSpinBoxMaxX", ui->doubleSpinBoxMaxX->value());
    values->insert("doubleSpinBoxMaxY", ui->doubleSpinBoxMaxY->value());
    values->insert("doubleSpinBoxMaxZ", ui->doubleSpinBoxMaxZ->value());
}

void DefineSampleBoxDialog::IncrementChanged(double d)
{
    ui->doubleSpinBoxMaxX->setSingleStep(d);
    ui->doubleSpinBoxMaxY->setSingleStep(d);
    ui->doubleSpinBoxMaxZ->setSingleStep(d);
    ui->doubleSpinBoxMinX->setSingleStep(d);
    ui->doubleSpinBoxMinY->setSingleStep(d);
    ui->doubleSpinBoxMinZ->setSingleStep(d);
}

void DefineSampleBoxDialog::RangeChanged(double d)
{
    ui->doubleSpinBoxMaxX->setRange(-d, +d);
    ui->doubleSpinBoxMaxY->setRange(-d, +d);
    ui->doubleSpinBoxMaxZ->setRange(-d, +d);
    ui->doubleSpinBoxMinX->setRange(-d, +d);
    ui->doubleSpinBoxMinY->setRange(-d, +d);
    ui->doubleSpinBoxMinZ->setRange(-d, +d);
}

void DefineSampleBoxDialog::DecimalsChanged(int i)
{
    ui->doubleSpinBoxMaxX->setDecimals(i);
    ui->doubleSpinBoxMaxY->setDecimals(i);
    ui->doubleSpinBoxMaxZ->setDecimals(i);
    ui->doubleSpinBoxMinX->setDecimals(i);
    ui->doubleSpinBoxMinY->setDecimals(i);
    ui->doubleSpinBoxMinZ->setDecimals(i);
}

void DefineSampleBoxDialog::MaxXChanged(double d)
{
    if (FuzzyTestLessThanOrEqual(d, ui->doubleSpinBoxMinX->value())) ui->doubleSpinBoxMinX->setValue(ui->doubleSpinBoxMinX->value() - ui->doubleSpinBoxMinX->singleStep());
    else emit DefineSampleBoxDialogDataChanged();
}

void DefineSampleBoxDialog::MaxYChanged(double d)
{
    if (FuzzyTestLessThanOrEqual(d, ui->doubleSpinBoxMinY->value())) ui->doubleSpinBoxMinY->setValue(ui->doubleSpinBoxMinY->value() - ui->doubleSpinBoxMinY->singleStep());
    else emit DefineSampleBoxDialogDataChanged();
}

void DefineSampleBoxDialog::MaxZChanged(double d)
{
    if (FuzzyTestLessThanOrEqual(d, ui->doubleSpinBoxMinZ->value())) ui->doubleSpinBoxMinZ->setValue(ui->doubleSpinBoxMinZ->value() - ui->doubleSpinBoxMinZ->singleStep());
    else emit DefineSampleBoxDialogDataChanged();
}

void DefineSampleBoxDialog::MinXChanged(double d)
{
    if (FuzzyTestGreaterThanOrEqual(d, ui->doubleSpinBoxMaxX->value())) ui->doubleSpinBoxMaxX->setValue(ui->doubleSpinBoxMaxX->value() + ui->doubleSpinBoxMaxX->singleStep());
    else emit DefineSampleBoxDialogDataChanged();
}

void DefineSampleBoxDialog::MinYChanged(double d)
{
    if (FuzzyTestGreaterThanOrEqual(d, ui->doubleSpinBoxMaxY->value())) ui->doubleSpinBoxMaxY->setValue(ui->doubleSpinBoxMaxY->value() + ui->doubleSpinBoxMaxY->singleStep());
    else emit DefineSampleBoxDialogDataChanged();
}

void DefineSampleBoxDialog::MinZChanged(double d)
{
    if (FuzzyTestGreaterThanOrEqual(d, ui->doubleSpinBoxMaxZ->value())) ui->doubleSpinBoxMaxZ->setValue(ui->doubleSpinBoxMaxZ->value() + ui->doubleSpinBoxMaxZ->singleStep());
    else emit DefineSampleBoxDialogDataChanged();
}

bool DefineSampleBoxDialog::FuzzyTestLessThanOrEqual(double d1, double d2)
{
    if (d1 < d2) return true;
    double eps = std::pow(10, -(ui->doubleSpinBoxMinX->decimals() + 1));
    if (std::fabs(d1 - d2) < eps) return true;
    return false;
}

bool DefineSampleBoxDialog::FuzzyTestGreaterThanOrEqual(double d1, double d2)
{
    if (d1 > d2) return true;
    double eps = std::pow(10, -(ui->doubleSpinBoxMinX->decimals() + 1));
    if (std::fabs(d1 - d2) < eps) return true;
    return false;
}

