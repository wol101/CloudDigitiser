#ifndef MISC_H
#define MISC_H

class QFileInfo;
class QString;

class Misc
{
public:
    Misc();

    static bool numberInFileNameLessThan(const QFileInfo &s1, const QFileInfo &s2);
    static bool numberInStringAsFileNameLessThan(const QString &str1, const QString &str2);
    static bool numberInStringLessThan(const QString &s1, const QString &s2);

};

#endif // MISC_H
