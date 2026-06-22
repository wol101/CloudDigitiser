#include <QFileInfo>
#include <QRegularExpression>
#include <QString>

#include "Misc.h"

Misc::Misc()
{
}

bool Misc::numberInFileNameLessThan(const QFileInfo &s1, const QFileInfo &s2)
{
    double v1 = 0, v2 = 0;
    QRegularExpression rx("[-+]?\\d*\\.\\d+|\\d+"); // matches a decimal number
    QRegularExpressionMatch match1 = rx.match(s1.fileName());
    if (match1.hasMatch()) v1 = match1.captured(0).toDouble();
    QRegularExpressionMatch match2 = rx.match(s2.fileName());
    if (match2.hasMatch()) v2 = match2.captured(0).toDouble();
    if (v1 == v2) return (s1.fileName() < s2.fileName());
    return (v1 < v2);
}

bool Misc::numberInStringAsFileNameLessThan(const QString &str1, const QString &str2)
{
    double v1 = 0, v2 = 0;
    QFileInfo s1(str1);
    QFileInfo s2(str2);
    QRegularExpression rx("[-+]?\\d*\\.\\d+|\\d+"); // matches a decimal number
    QRegularExpressionMatch match1 = rx.match(s1.fileName());
    if (match1.hasMatch()) v1 = match1.captured(0).toDouble();
    QRegularExpressionMatch match2 = rx.match(s2.fileName());
    if (match2.hasMatch()) v2 = match2.captured(0).toDouble();
    if (v1 == v2) return (s1.fileName() < s2.fileName());
    return (v1 < v2);
}


bool Misc::numberInStringLessThan(const QString &s1, const QString &s2)
{
    double v1 = 0, v2 = 0;
    QRegularExpression rx("[-+]?\\d*\\.\\d+|\\d+"); // matches a decimal number
    QRegularExpressionMatch match1 = rx.match(s1);
    if (match1.hasMatch()) v1 = match1.captured(0).toDouble();
    QRegularExpressionMatch match2 = rx.match(s2);
    if (match2.hasMatch()) v2 = match2.captured(0).toDouble();
    if (v1 == v2) return (s1 < s2);
    return (v1 < v2);
}

