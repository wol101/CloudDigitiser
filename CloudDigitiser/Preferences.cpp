#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QColor>

#include "Preferences.h"

Preferences::Preferences()
{
    Read();
}

Preferences::~Preferences()
{
    Write();
}

void Preferences::Write()
{
    QSettings settings(m_settings["Organisation"].value.toString(), m_settings["Application"].value.toString());
    settings.clear();
    for (QMap<QString, SettingsItem>::const_iterator i = m_settings.constBegin(); i != m_settings.constEnd(); i++)
    {
        qDebug("%s: %s", qUtf8Printable(i.key()), qUtf8Printable(i.value().value.toString()));
        settings.setValue(i.key(), i.value().value);
    }
    settings.sync();
}

void Preferences::Read()
{
    LoadDefaults();
    SettingsItem item;
    QSettings settings(m_settings["Organisation"].value.toString(), m_settings["Application"].value.toString());
    // check whether the settings are the right ones
    if (settings.value("SettingsCode") != m_settings["SettingsCode"].value)
    {
        settings.clear();
        settings.sync();
    }
    else
    {
        QStringList keys = settings.allKeys();
        for (int i = 0; i < keys.size(); i++) insert(keys[i], settings.value(keys[i]));
    }
}

void Preferences::LoadDefaults()
{
    m_settings.clear();
    SettingsItem item;
    m_settings.clear();
    QFile file(":/DefaultSettings");
    bool ok = file.open(QIODevice::ReadOnly);
    Q_ASSERT(ok);
    QTextStream textStream(&file);
    int order = 0;
    while (true)
    {
        QString line = textStream.readLine();
        if (line.isNull())
            break;
        QStringList tokens = line.split("\t");
        // format is:
        // setting_name \t type \t value
        if (tokens.size() < 5) continue;

        item.key = tokens[0];
        item.type = tokens[1];
        item.order = order;
        order++;
        if (tokens[2].compare("true", Qt::CaseInsensitive) == 0 || tokens[2].toInt() != 0) item.display = true;
        else item.display = false;
        item.label = tokens[3];
        if (item.type.compare("string") == 0 || item.type.compare("path") == 0)
        {
            item.defaultValue = tokens[4];
            item.value = item.defaultValue;
            m_settings[tokens[0]] = item;
            continue;
        }
        if (item.type.compare("int") == 0)
        {
            item.defaultValue = tokens[4].toInt();
            item.value = item.defaultValue;
            m_settings[tokens[0]] = item;
            continue;
        }
        if (item.type.compare("double") == 0)
        {
            item.defaultValue = tokens[4].toDouble();
            item.value = item.defaultValue;
            m_settings[tokens[0]] = item;
            continue;
        }
        if (item.type.compare("bool") == 0)
        {
            if (tokens[4].compare("true", Qt::CaseInsensitive) == 0 || tokens[4].toInt() != 0) item.defaultValue = true;
            else item.defaultValue = false;
            item.value = item.defaultValue;
            m_settings[tokens[0]] = item;
            continue;
        }
        if (item.type.compare("colour") == 0)
        {
            if (tokens.size() == 7) item.defaultValue = QColor(tokens[4].toInt(), tokens[5].toInt(), tokens[6].toInt(), 255);
            if (tokens.size() == 8) item.defaultValue = QColor(tokens[4].toInt(), tokens[5].toInt(), tokens[6].toInt(), tokens[7].toInt());
            item.value = item.defaultValue;
            m_settings[tokens[0]] = item;
            continue;
        }
    }
}

const SettingsItem Preferences::settingsItem(const QString &key) const
{
    return m_settings.value(key);
}

void Preferences::getValue(const QString &key, QVariant *value) const
{
    if (m_settings.contains(key)) *value = m_settings.value(key).value;
}

void Preferences::getValue(const QString &key, QString *value) const
{
    if (m_settings.contains(key)) *value = m_settings.value(key).value.toString();
}

void Preferences::getValue(const QString &key, QColor *value) const
{
    if (m_settings.contains(key)) *value = qvariant_cast<QColor>(m_settings.value(key).value);
}

void Preferences::getValue(const QString &key, double *value) const
{
    if (m_settings.contains(key)) *value = m_settings.value(key).value.toDouble();
}

void Preferences::getValue(const QString &key, float *value) const
{
    if (m_settings.contains(key)) *value = static_cast<float>(m_settings.value(key).value.toDouble());
}

void Preferences::getValue(const QString &key, int *value) const
{
    if (m_settings.contains(key)) *value = m_settings.value(key).value.toInt();
}

void Preferences::getValue(const QString &key, bool *value) const
{
    if (m_settings.contains(key)) *value = m_settings.value(key).value.toBool();
}

void Preferences::insert(const QString &key, const SettingsItem &item)
{
    m_settings.insert(key, item);
}

bool Preferences::contains(const QString &key) const
{
    return m_settings.contains(key);
}

QStringList Preferences::keys() const
{
    return m_settings.keys();
}

void Preferences::insert(const QString &key, const QVariant &value)
{
    SettingsItem item;
    if (m_settings.contains(key))
    {
        item = m_settings.value(key);
        item.value = value;
        m_settings.insert(key, item);
    }
    else
    {
        item.key = key;
        item.display = false;
        item.label = "";
        item.order = -1;
        switch (value.type())
        {
        case QMetaType::Int:
            item.type = "int";
            item.value = value;
            item.defaultValue = item.value;
            m_settings.insert(key, item);

        case QMetaType::Double:
            item.type = "double";
            item.value = value;
            item.defaultValue = item.value;
            m_settings.insert(key, item);

        case QMetaType::QString:
            item.type = "string";
            item.value = value;
            item.defaultValue = item.value;
            m_settings.insert(key, item);

        case QMetaType::QColor:
            item.type = "colour";
            item.value = value;
            item.defaultValue = item.value;
            m_settings.insert(key, item);

        case QMetaType::Bool:
            item.type = "bool";
            item.value = value;
            item.defaultValue = item.value;
            m_settings.insert(key, item);

        default:
            item.type = value.typeName();
            item.value = value;
            item.defaultValue = item.value;
            m_settings.insert(key, item);
        }
    }
}


