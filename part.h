#ifndef PART_H
#define PART_H

//----------------------|MACROS|----------------------
#define _GNU_SOURCE 1   //For NOATIME in open(2) //Must be very first line!
#define PAGING_COUNT 10 //Shahab    //For readPostingFile()
#define RECORD_SIZE 16  //Shahab    //For readPostingFile
#define ID_LEN 16       //Mr. Aladaghi
#define DATA_LEN 164    //4+16*10   //Mr. Aladaghi
#define KEY_LEN 101     //Mr. Aladaghi
#define BUCKET_SIZE 10 //Mr.mahmoudi??    //Used in findkey() if(count == 10) section
//--------------------|END MACROS|--------------------

#include <fcntl.h>      //for open(2)   //Shahab
#include <unistd.h>     //for lseek(2)  //Shahab   //Unnecessary
#include <sys/stat.h>   //For stat(2)   //Shahab
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

//Mahmoud
struct FileStateManager
{
    //FileStateManager(const char* _path){
        //memcpy(path, _path, sizeof(_path));
    FileStateManager(){
        accessTime=0;
        accessCount=0;
    }
    char* path;
    long accessTime;
    int accessCount;
};

class Part : public QObject
{
    Q_OBJECT
public:
    explicit Part(QObject *parent = nullptr);
    void insertRecord(const char * id, const char *keys);
    QList<UUID> findKey(const char *key_value);
    void printAllKeys(QString treeName);    //Mr. Aladaghi + shahab
    bool isKeyExist(const char *key, const char *value, void * data);   //Mr. Mahmoudi
    bool isKeyExist(const char *key_value, void *data); //Mr. Aladaghi + Mr. Mahmoudi
    void importCSV(QString filePath);
    void loadGists();
    void dropGists();
    void testInserts();
    FILE *openFile(const char *fileName, const char *mode);
    void checkAndRemoveIlligalChar(char* str, char c);//mahmoud
    void appropriateFileEliminate(); //Mahmoud

    //-------------------------------------------------------------------------------|Shahab|--------------------------------------------------------------------------------------------
    void hexStrToBin(const char* uuid, unsigned char *bins);   //shahab
    void binToHexStr(const unsigned char* bins, char* out); //shahab
    char compareBins(unsigned char* first, unsigned char* second);  //Shahab
    int openFileV2(const char* pathname, int flags);   //shahab
    int readPostingFile(int fd, int countOfDataInside, unsigned char* readBuffer, char* isEof);    //shahab
    int closePostingFd(int fd);  //Shahab
    //-----------------------------------------------------------------------------|END Shahab|------------------------------------------------------------------------------------------

signals:
private:
    QMap<QString, gist*> gists;
    //QMap<QString, FILE*> files;
    QList<QPair<QString, FILE*>> dupValuefiles;
    QList<QPair<FileStateManager, FILE*>> dupValueOrderFiles;
    void insertTerm(const char *id, const char *term);
    bool insertId(const char *id);
    void extractKeyValue(const char *term, char *key, char *value);
    QStringList findFiles(const QString &startDir, const QStringList &filters);
};

#endif // PART_H
