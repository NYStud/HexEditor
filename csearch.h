/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CSEARCH_H
#define CSEARCH_H

#include "cmemorymappedfile.h"

#include <QFile>
#include <QObject>
#include <QProgressBar>
#include <QStandardItemModel>
#include <QTreeView>

typedef struct FoundedResult{
    FoundedResult(int64_t inPos, int64_t inLen)
    {
        pos = inPos;
        len = inLen;
    }
    int64_t pos;
    int64_t len;
}FoundedResult;

class CSearch : public QObject
{
    Q_OBJECT
public:
    explicit CSearch(QObject *parent = nullptr);
    void Search(const char *searchBuffer, int32_t searchBufferLength, const QString &searchFile,
                QStandardItemModel *resultControl, QProgressBar *progressBar);
    void Abort(){m_abort = true;}

private:
    void ParseFile(const char *searchBuffer, int32_t searchBufferLength);
    void AddResult(int64_t pos);
    CMemoryMappedFile m_mappedFile;
    QFile m_file;
    QStandardItemModel* m_resultControl = nullptr;
    QProgressBar *m_progressBar = nullptr;
    bool m_abort = false;
};



#endif // CSEARCH_H