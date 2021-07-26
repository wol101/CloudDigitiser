#ifndef CALIBRATEDIALOG_H
#define CALIBRATEDIALOG_H

#include <QDialog>

namespace Ui {
class CalibrateDialog;
}

class CalibrateDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit CalibrateDialog(QWidget *parent = 0);
    ~CalibrateDialog();
    
    void SetValues(const QMap<QString, QVariant> &values);

    void GetValues(QMap<QString, QVariant> *values);

public slots:
    void StepChanged(double d);
    void DecimalsChanged(int i);
    void ModelXChanged(double d);
    void ModelYChanged(double d);
    void ModelZChanged(double d);
    void WorldXChanged(double d);
    void WorldYChanged(double d);
    void WorldZChanged(double d);
    void XScaleChanged(double d);
    void YScaleChanged(double d);
    void ZScaleChanged(double d);
    void UniformScaleChanged(int i);

private:
    Ui::CalibrateDialog *ui;
};

#endif // CALIBRATEDIALOG_H
