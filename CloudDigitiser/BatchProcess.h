#ifndef BATCHPROCESS_H
#define BATCHPROCESS_H

#include <QString>

class BatchProcess
{
public:
    BatchProcess();

    void SetInputs(QString sourceFolder, QString destinationFolder, QString transformationFile);
    void Process();

private:
    QString m_sourceFolder;
    QString m_destinationFolder;
    QString m_transformationFile;
};

#endif // BATCHPROCESS_H
