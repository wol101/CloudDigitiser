#ifndef DEFINESAMPLEBOXDIALOG_H
#define DEFINESAMPLEBOXDIALOG_H

#include <QDialog>
#include <QMap>
#include <QVariant>

namespace Ui {
class DefineSampleBoxDialog;
}

class DefineSampleBoxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DefineSampleBoxDialog(QWidget *parent = 0);
    ~DefineSampleBoxDialog();

    void SetValues(const QMap<QString, QVariant> &values);
    void GetValues(QMap<QString, QVariant> *values);

public slots:
    void IncrementChanged(double d);
    void RangeChanged(double d);
    void DecimalsChanged(int i);
    void MaxXChanged(double d);
    void MaxYChanged(double d);
    void MaxZChanged(double d);
    void MinXChanged(double d);
    void MinYChanged(double d);
    void MinZChanged(double d);

signals:
    void DefineSampleBoxDialogDataChanged();

private:
    Ui::DefineSampleBoxDialog *ui;

    bool FuzzyTestLessThanOrEqual(double d1, double d2);
    bool FuzzyTestGreaterThanOrEqual(double d1, double d2);
};

#endif // DEFINESAMPLEBOXDIALOG_H
