#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QColor>

#include "Preferences.h"

struct SettingsWidget
{
    QWidget *widget;
    SettingsItem item;
};

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    void setPreferences(const Preferences &preferences);
    Preferences preferences();

    QColor getIdealTextColour(const QColor& rBackgroundColour);
    QColor getAlphaColourHint(const QColor& colour);

public slots:
    void colourButtonClicked();

private:
    QList<SettingsWidget> m_SettingsWidgetList;
    Preferences m_Preferences;
};

#endif // PREFERENCESDIALOG_H
