#ifndef PART_H
#define PART_H

#include <QObject>
#include <QElapsedTimer>
#include <QMap>
#include "GiST/gist.h"
#include "BTree/gist_btree.h"
#include "GiST/gist_cursor.h"


class Part : public QObject
{
    Q_OBJECT
public:
    explicit Part(QObject *parent = nullptr);
    void insertRecord(const char * id, const char *keys);
    QStringList findKey(const char *key_value);
    bool isKeyExist(const char *key_value);


    void importCSV(QString filePath);
    void loadGists();
    void dropGists();
    void testInserts();
    void printAllKeys(QString treeName);    //shahab
    void hexStrToBin(const char* uuid, unsigned char** out);   //shahab
    void binToHexStr(const unsigned char* bins, char** out); //shahab
signals:
private:
    QMap<QString, gist*> gists;
    void insertTerm(const char *id, const char *term);
    bool insertId(const char *id);
    void extractKeyValue(const char *term, char *key, char *value);
};

#endif // PART_H
