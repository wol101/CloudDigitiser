#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QColor>
#include <QString>
#include <QByteArray>
#include <QVariantMap>
#include <QVariant>
#include <QMap>

struct SettingsItem
{
    SettingsItem() { order = -1; display = false; }
    QString key;
    QString type;
    bool display;
    QString label;
    QVariant defaultValue;
    QVariant value;
    int order;
};

class Preferences
{
public:
    Preferences();
    ~Preferences();

    void Read();
    void Write();
    void LoadDefaults();

    const SettingsItem settingsItem(const QString &key) const;
    void getValue(const QString &key, QVariant *value) const;
    void getValue(const QString &key, QString *value) const;
    void getValue(const QString &key, QColor *value) const;
    void getValue(const QString &key, double *value) const;
    void getValue(const QString &key, float *value) const;
    void getValue(const QString &key, int *value) const;
    void getValue(const QString &key, bool *value) const;
    void insert(const QString &key, const SettingsItem &item);
    void insert(const QString &key, const QVariant &value);
    bool contains(const QString &key) const;
    QStringList keys() const;

protected:

    QMap<QString, SettingsItem> m_settings;
};

#endif // PREFERENCES_H
