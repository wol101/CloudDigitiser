#include <QFileInfo>
#include <QRegExp>
#include <QString>

#include "Misc.h"

Misc::Misc()
{
}

bool Misc::numberInFileNameLessThan(const QFileInfo &s1, const QFileInfo &s2)
{
    double v1 = 0, v2 = 0;
    QRegExp rx("[-+]?\\d*\\.\\d+|\\d+"); // matches a decimal number
    int in1 = rx.indexIn(s1.fileName());
    if (in1 >= 0) v1 = rx.cap(0).toDouble();
    int in2 = rx.indexIn(s2.fileName());
    if (in2 >= 0) v2 = rx.cap(0).toDouble();
    if (v1 == v2) return (s1.fileName() < s2.fileName());
    return (v1 < v2);
}

bool Misc::numberInStringAsFileNameLessThan(const QString &str1, const QString &str2)
{
    double v1 = 0, v2 = 0;
    QFileInfo s1(str1);
    QFileInfo s2(str2);
    QRegExp rx("[-+]?\\d*\\.\\d+|\\d+"); // matches a decimal number
    int in1 = rx.indexIn(s1.fileName());
    if (in1 >= 0) v1 = rx.cap(0).toDouble();
    int in2 = rx.indexIn(s2.fileName());
    if (in2 >= 0) v2 = rx.cap(0).toDouble();
    if (v1 == v2) return (s1.fileName() < s2.fileName());
    return (v1 < v2);
}


bool Misc::numberInStringLessThan(const QString &s1, const QString &s2)
{
    double v1 = 0, v2 = 0;
    QRegExp rx("[-+]?\\d*\\.\\d+|\\d+"); // matches a decimal number
    int in1 = rx.indexIn(s1);
    if (in1 >= 0) v1 = rx.cap(0).toDouble();
    int in2 = rx.indexIn(s2);
    if (in2 >= 0) v2 = rx.cap(0).toDouble();
    if (v1 == v2) return (s1 < s2);
    return (v1 < v2);
}

