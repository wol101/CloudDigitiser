#include <QColorDialog>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QMultiMap>

#include "Preferences.h"

#include "PreferencesDialog.h"

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences Dialog"));
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::setPreferences(const Preferences &preferences)
{
    const QString COLOUR_STYLE("QPushButton { background-color : %1; color : %2; border: 4px solid %3; }");
    m_Preferences = preferences;

    QVBoxLayout *topLayout = new QVBoxLayout;
    setLayout(topLayout);

    QGridLayout *gridLayout = new QGridLayout();

    QStringList keys = m_Preferences.keys();
    QMultiMap<int, SettingsItem> sortedItems;
    for (int i = 0; i < keys.size(); i++)
    {
        SettingsItem item = m_Preferences.settingsItem(keys[i]);
        if (item.display) sortedItems.insert(item.order, item);
    }

    int row = 0;
    SettingsWidget settingsWidget;
    for (QMultiMap<int, SettingsItem>::const_iterator i = sortedItems.constBegin(); i != sortedItems.constEnd(); i++)
    {
        SettingsItem item = i.value();

        QLabel *label = new QLabel();
        label->setText(item.label);
        gridLayout->addWidget(label, row, 0);

        settingsWidget.item = item;
        QLineEdit *lineEdit;
        if (item.type == "int")
        {
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(item.value.toInt()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
        }

        if (item.type == "double")
        {
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(item.value.toDouble()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
        }

        if (item.type == "string")
        {
            lineEdit = new QLineEdit();
            lineEdit->setMinimumWidth(200);
            lineEdit->setText(QString("%1").arg(item.value.toString()));
            gridLayout->addWidget(lineEdit, row, 1);
            settingsWidget.widget = lineEdit;
            m_SettingsWidgetList.append(settingsWidget);
        }

        if (item.type == "bool")
        {
            QCheckBox *checkBox = new QCheckBox();
            checkBox->setChecked(item.value.toBool());
            gridLayout->addWidget(checkBox, row, 1);
            settingsWidget.widget = checkBox;
            m_SettingsWidgetList.append(settingsWidget);
        }

        if (item.type == "colour")
        {
            QPushButton *pushButton = new QPushButton();
            pushButton->setText("Colour");
            QColor color = qvariant_cast<QColor>(item.value);
            pushButton->setStyleSheet(COLOUR_STYLE.arg(color.name()).arg(getIdealTextColour(color).name()).arg(getAlphaColourHint(color).name()));
            connect(pushButton, SIGNAL(clicked()), this, SLOT(colourButtonClicked()));
            gridLayout->addWidget(pushButton, row, 1);
            settingsWidget.widget = pushButton;
            m_SettingsWidgetList.append(settingsWidget);
        }

        row++;

    }

    topLayout->addLayout(gridLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    topLayout->addWidget(buttonBox);

}

Preferences PreferencesDialog::preferences()
{
    for (int i = 0; i < m_SettingsWidgetList.size(); i++)
    {
        SettingsItem item = m_SettingsWidgetList[i].item;
        QWidget *widget = m_SettingsWidgetList[i].widget;

        if (item.type == "int") item.value = dynamic_cast<QLineEdit *>(widget)->text().toInt();
        if (item.type == "double") item.value = dynamic_cast<QLineEdit *>(widget)->text().toDouble();
        if (item.type == "string") item.value = dynamic_cast<QLineEdit *>(widget)->text();
        if (item.type == "bool") item.value = dynamic_cast<QCheckBox *>(widget)->isChecked();
        if (item.type == "int") item.value = dynamic_cast<QLineEdit *>(widget)->text().toInt();

        m_Preferences.insert(item.key, item);
    }
    return m_Preferences;
}

void PreferencesDialog::colourButtonClicked()
{
    const QString COLOUR_STYLE("QPushButton { background-color : %1; color : %2; border: 4px solid %3; }");
    int i;
    QPushButton *pushButton = dynamic_cast<QPushButton *>(QObject::sender());
    for (i = 0; i < m_SettingsWidgetList.size(); i++)
        if (m_SettingsWidgetList[i].widget == pushButton) break;
    if (i >= m_SettingsWidgetList.size()) return;

    QColor colour = QColorDialog::getColor(qvariant_cast<QColor>(m_SettingsWidgetList[i].item.value), this, "Select Color", QColorDialog::ShowAlphaChannel /*| QColorDialog::DontUseNativeDialog*/);
    if (colour.isValid())
    {
        pushButton->setStyleSheet(COLOUR_STYLE.arg(colour.name()).arg(getIdealTextColour(colour).name()).arg(getAlphaColourHint(colour).name()));
        m_SettingsWidgetList[i].item.value = colour;
    }
}


// return an ideal label colour, based on the given background colour.
// Based on http://www.codeproject.com/cs/media/IdealTextColor.asp
QColor PreferencesDialog::getIdealTextColour(const QColor& rBackgroundColour)
{
    const int THRESHOLD = 105;
    int BackgroundDelta = (rBackgroundColour.red() * 0.299) + (rBackgroundColour.green() * 0.587) + (rBackgroundColour.blue() * 0.114);
    return QColor((255- BackgroundDelta < THRESHOLD) ? Qt::black : Qt::white);
}

QColor PreferencesDialog::getAlphaColourHint(const QColor& colour)
{
    // (source × Blend.SourceAlpha) + (background × Blend.InvSourceAlpha)
    QColor background;
    background.setRgbF(1.0, 1.0, 1.0);
    QColor hint;
    hint.setRedF((colour.redF() * colour.alphaF()) + (background.redF() * (1 - colour.alphaF())));
    hint.setGreenF((colour.greenF() * colour.alphaF()) + (background.greenF() * (1 - colour.alphaF())));
    hint.setBlueF((colour.blueF() * colour.alphaF()) + (background.blueF() * (1 - colour.alphaF())));
    return hint;
}


