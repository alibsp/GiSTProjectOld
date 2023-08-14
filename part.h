#ifndef PART_H
#define PART_H

#include <QObject>
#include <QElapsedTimer>
#include <QMap>
#include "GiST/gist.h"
#include "BTree/gist_btree.h"
#include "GiST/gist_cursor.h"

struct UUID
{
    UUID(const unsigned char * _val)
    {
        memcpy(val, _val, 16);
    }
    unsigned char val[16];
};

class Part : public QObject
{
    Q_OBJECT
public:
    explicit Part(QObject *parent = nullptr);
    void insertRecord(const char * id, const char *keys);
    QList<UUID> findKey(const char *key_value);
    bool isKeyExist(const char *key, const char *value, void * data);
    bool isKeyExist(const char *key_value, void *data);


    void importCSV(QString filePath);
    void loadGists();
    void dropGists();
    void testInserts();
    void printAllKeys(QString treeName);    //shahab
    void hexStrToBin(const char* uuid, unsigned char *bins);   //shahab
    void binToHexStr(const unsigned char* bins, char* out); //shahab
    FILE *openFile(const char *fileName, const char *mode);
signals:
private:
    QMap<QString, gist*> gists;
    //QMap<QString, FILE*> files;
    QList<QPair<QString, FILE*>> dupValuefiles;
    void insertTerm(const char *id, const char *term);
    bool insertId(const char *id);
    void extractKeyValue(const char *term, char *key, char *value);
    QStringList findFiles(const QString &startDir, const QStringList &filters);
};

#endif // PART_H
