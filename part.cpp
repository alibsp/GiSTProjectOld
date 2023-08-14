#include "part.h"

#include <QDir>
#include <QFileInfoList>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define ID_LEN 16
#define DATA_LEN 164 //4+16*10
#define KEY_LEN 101
  struct stat st = {0};
Part::Part(QObject *parent) : QObject(parent)
{
    // testInserts();
    //dropGists();
    //loadGists();
}

void Part::importCSV(QString filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        //cout << file.errorString();
        return ;
    }

    int i=0;
    while (!file.atEnd())
    {
        QByteArray line = file.readLine();
        if(i)
        {
            QList<QByteArray> columns = line.split(';');
            QString id=QString(columns[0]);
            QString keys=QString(columns[2]).replace("\"","");
            cout<<"Record "<<i<<endl;
            if(i==520)
                i=520;
            insertRecord(columns[0].data(), keys.toUtf8().data());
        }
        i++;
    }
    file.close();
}

void Part::loadGists()
{

#ifdef __linux__
    QDir dir("data/", "*.db");
#elif _WIN32
    QDir dir("data\\", "*.db");
#endif

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        QString keyName = fileInfo.fileName().split(".")[0];

        gist *myGist  =gists[keyName];
        if(myGist==nullptr)
        {
            myGist = new gist();
            if(myGist->create(fileInfo.filePath().toLocal8Bit().data() , &bt_str_key_binary_data_ext)!=RCOK)
            {
                if(myGist->open(fileInfo.filePath().toLocal8Bit().data())==RCOK)
                    gists[keyName] = myGist;
            }
        }
    }
}
void Part::dropGists()
{

    #ifdef __linux__
    QDir dir("data/", "*.db");
#elif _WIN32
    QDir dir("data\\", "*.db");
#endif

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i)
    {
        QFile file (list[i].filePath());
        file.remove();
    }
}
void Part::insertRecord(const char *id, const char *keys)
{
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    char myId[37]={0};
    strcpy(myId, id);
    //if(insertId(myId))
    {
        int i=0;
        char term[KEY_LEN]={0};
        for (int j=0; ; i++)
        {
            if(keys[i]==',' || (keys[i]=='}' &&  keys[i+1]==0) || keys[i]==0 || j>=KEY_LEN)
            {
                term[j] = 0;
                if(j)
                {
                    if(strcmp(myId, "f29519f2-0ab7-4bd3-baac-c0f21055cd78")==0 && strcmp(term, "changeType_updated_by_id"))
                        printAllKeys("changeType");
                    insertTerm(myId, term);
                }
                j=0;
            }
            else if((i==0&&keys[i]!='{') || i)
                term[j++]=keys[i];
            if( keys[i]==0 || j>=KEY_LEN)
                break;
        }
    }
    cout<<"insertRecord finish at "<<elapsedTimer.nsecsElapsed()<<endl;
}

QList<UUID> Part::findKey(const char * key_value)
{
    //QElapsedTimer timer;
    //timer.start();
    char key[KEY_LEN]={0};
    char value[KEY_LEN]={0};
    extractKeyValue(key_value, key, value);
    QList<UUID> results;
    gist *myGist=gists[key];

    bt_query_t q(bt_query_t::bt_eq, (void*)value);
    gist_cursor_t cursor;
    if(myGist->fetch_init(cursor, &q)!=RCOK)
    {
        cerr << "Can't initialize cursor." << endl;
        return results;
    }
    bool eof = false;
    smsize_t keysz=KEY_LEN, datasz=DATA_LEN;

    char keyFound[KEY_LEN]={0};
    char data[DATA_LEN]={0};
    while (!eof)
    {
        if(myGist->fetch(cursor, (void *)&keyFound, keysz, (void *)&data, datasz, eof)!=RCOK)
        {
            cerr << "Can't fetch from cursor." << endl;
            return results;
        }
        if (!eof)
        {
            unsigned int count=0;
            memcpy(&count, data, 4);
            for(size_t i=0;i<count && i< 10;i++)
            {
                unsigned char bin_id[ID_LEN]={0};
                memcpy(bin_id, data+(4+i*ID_LEN), ID_LEN);
                //binToHexStr(uuid, id);
                UUID uuid(bin_id);
                results.append(uuid);
            }
            if(count>10)
            {
                //go to posting tree
                // can use printAllTree method pattern
                //Then concat results
            }
        }
    }
    //cout<<"Execute Time: "<<timer.nsecsElapsed()<<" ns, record count:"<<results.count()<<endl;
    return results;
}

//Start Mr. MahmoudiNik
//Continue by Aldaghi(Add unsigned char *data to reuslt)
bool Part::isKeyExist(const char *key_value, void *data)
{
    char key[KEY_LEN]={0};
    char value[KEY_LEN]={0};
    extractKeyValue(key_value, key, value);
    isKeyExist(key, value, data);
}

bool Part::isKeyExist(const char *key, const char *value, void *data)
{
    //This method checks if a "key-value" exists in tree or not

    gist *myGist=gists[key];
    bt_query_t q(bt_query_t::bt_eq, (void*)value);
    gist_cursor_t cursor;
    if(myGist->fetch_init(cursor, &q)!=RCOK)
    {
        cerr << "Can't initialize cursor." << endl;
        return false;
    }
    bool eof = false;
    smsize_t keysz=KEY_LEN, datasz=DATA_LEN;
    char keyFound[KEY_LEN]={0};
    while (!eof)
    {
        if(myGist->fetch(cursor, (void *)&keyFound, keysz, data, datasz, eof)!=RCOK)
        {
            cerr << "Can't fetch from cursor." << endl;
            return false;
        }
        if (!eof)
            return true;
    }
    return false;
}

//term = "source_gitlab\0"
void Part::extractKeyValue(const char *term, char *key, char *value)
{
    for (int i=0, sw=0, j=0;;i++)
    {
        if(sw==0)
            key[j++]=term[i];
        if(sw==1)
            value[j++]=term[i];
        if(term[i]=='_')
        {
            if(sw==0)
            {
                key[j-1]=0;
                j=0;
            }
            sw=1;
        }
        if(!term[i])
            break;
    }
}


//void Part::hexStrToBin(const char* uuid, unsigned char** out)   //shahab
void Part::hexStrToBin(const char* uuid, unsigned char* bins)   //aldaghi
{
    unsigned char i=0, byte=0, res=0;
    //unsigned char* bins = (unsigned char*)malloc( 16*sizeof(char) );    //Always assign a new, free memory to this pointer! As the reference will be copied to the *out parameter of the very first call to this function and thus, never will be deleted after going out of scope!
    while(uuid[i]!=0)
    {
        if(uuid[i]=='-') {i++; continue;}

        if(uuid[i] > 47 && uuid[i] < 58) {res = (uuid[i]-48)*16;}
        else if(uuid[i] > 96 && uuid[i] < 103) {res = (uuid[i]-87)*16;}

        if(uuid[i+1] > 47 && uuid[i+1] < 58) {res += (uuid[i+1]-48);}
        else if(uuid[i+1] > 96 && uuid[i+1] < 103) {res += (uuid[i+1]-87);}

        bins[byte] = res;
        byte++; i+=2;
    }

    //*out = bins;
    //We cannot free bins here as the memory address now being used by *out !
}


void Part::binToHexStr(const unsigned char* bins, char* out) //shahab
{
    //char output[37];
    char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    for( int i = 0, j = 0; i < 16; i++ )
    {
        char const byte = bins[i];

        (out)[j] = hex_chars[ ( byte & 0xF0 ) >> 4 ];
        (out)[j+1] = hex_chars[ ( byte & 0x0F ) ];
        j+=2;
        if( i==3 || i==5 || i==7 || i==9 ) {(out)[j] = '-'; j++;}
    }
    (out)[36] = 0;
    //printf("%s\n\n", (*out));
}


//*id = 43bf9957-e944-408a-a17d-ea7ac40ffea7 <-> term = "source_gitlab\0"
void Part::insertTerm(const char *id, const char *term)
{
    unsigned char binaryUUID[ID_LEN];  //shahab: I think we don't need to worry about allocating explict memory range for this, as it'll be assigned to another memory location later;
    hexStrToBin(id, binaryUUID);

    unsigned char data[DATA_LEN]={0};



    char treeName[KEY_LEN]={0};
    char treeKey[KEY_LEN]={0};
    extractKeyValue(term, treeName, treeKey);

    //strcat(value, ":"); //shahab:duplicate error fix
    //strcat(value, id);  //shahab:duplicate error fix

    if(treeName[0]==0)
        strcpy(treeName, "nonekey");
#ifdef __linux__
    char path[200]="data/";
    char path_data_folder[200]="data/";
#elif _WIN32
    char path[200]="data\\";
#endif

    strcat(path, treeName);
    strcat(path_data_folder, treeName);

    strcat(path, ".db");
    //ایجاد پایگاه داده با افزونه BTree
    gist *myGist  =gists[treeName];
    if(myGist==nullptr)
    {
        myGist = new gist();
        gists[treeName] = myGist;

        if(myGist->create(path, &bt_str_key_binary_data_ext)!=RCOK)
            if(myGist->open(path)!=RCOK)
            {
                cerr << "Can't Open File." << endl;
                return ;
            }
    }
    //qDebug()<< id << key << value;
    //std::cout <<id <<" "<< key  <<" "<< value<<endl;
    //myGist
    //if -> bloom check (term)  -->0 --1>
    //TODO: Consider adding bloom filter
    unsigned int count=0;
    if(strcmp(term, "source_gitlab")==0)
        count=1;


    isKeyExist(treeName, treeKey, (void *)data);

    memcpy(&count, data, 4);
    count++;
    if(count<=10)
    {
        memcpy(data, &count, 4);
        memcpy(data+(4+(count-1)*ID_LEN), binaryUUID, ID_LEN);
        if(count>1)
        {
            bt_query_t q(bt_query_t::bt_eq, (void *)treeKey);
            myGist->remove(&q);
        }
        myGist->insert((void *) &treeKey, KEY_LEN, (void *) data, DATA_LEN);
        myGist->flush();
    }
    else
    {
        count=10;
        //Directory name: treeName
        //File name     : treeKey
        //key in tree   : binaryUUID
        //type of tree  : bt_binary_key_ext


        //mahmoud

        //1.Check if directory not exists, then create directory
        if (stat(path_data_folder, &st) == -1) {
            mkdir(path_data_folder, 0700);
        }
        //2.Create sub Tree of binaryUUID's:
        //2.1.make name of tree file
         strcat(path_data_folder, "/");
         strcat(path_data_folder, treeKey);
         strcat(path_data_folder, ".db");

        //std::cout << "path_data_folder" << path_data_folder << endl ;
          gist *myGistSubTree  =gists[treeKey];
          if(myGistSubTree==nullptr)
          {
              myGistSubTree = new gist();
              gists[treeName] = myGistSubTree;

              if(myGistSubTree->create(path_data_folder, &bt_binary_key_ext)!=RCOK)
                  if(myGistSubTree->open(path_data_folder)!=RCOK)
                  {
                      cerr << "Can't Open File." << endl;
                      return ;
                  }
          }
          //MAHMOUD:CODE WORKS BUT ERROR IN record 27

    }
    //TODO: add code to create new posting-tree if key exsists!
    //myGist->close();
}
//1402-05-22 Aldaghi: Store id in id tree as binary
bool Part::insertId(const char *id)
{
    unsigned char binaryUUID[ID_LEN];  //shahab: I think we don't need to worry about allocating explict memory range for this, as it'll be assigned to another memory location later;
    hexStrToBin(id, binaryUUID);

#ifdef __linux__
    char path[]="data/id.db";
#elif _WIN32
    char path[]="data\\id.db";
#endif

    gist *myGist  =gists["id"];
    if(myGist==nullptr)
    {
        myGist = new gist();
        gists["id"] = myGist;
        if(myGist->create(path, &bt_binary_key_ext)!=RCOK)
        {
            if(myGist->open(path)!=RCOK)
            {
                cerr << "Can't Oepn File." << endl;
                return false;
            }
        }
    }
    bt_query_t q(bt_query_t::bt_eq, &binaryUUID);
    gist_cursor_t cursor;
    if(myGist->fetch_init(cursor, &q)!=RCOK)
    {
        cerr << "Can't initialize cursor." << endl;
        return false;   //shahab
    }
    bool eof = false;

    char data[2]="A";
    smsize_t keysz=ID_LEN, datasz=2;
    char key[ID_LEN]={0};
    while (!eof)
    {
        if(myGist->fetch(cursor, (void *)&key, keysz, (void *)&data, datasz, eof)!=RCOK)
        {
            cerr << "Can't fetch from cursor." << endl;
            return false;   //shahab
        }
        if (!eof)
        {
            cout<<"id:"<<(char*)&key<<"->"<<data<<endl;
            //myGist->close();
            return false;
        }// process key and data...
    }
    strcpy(data, "A");
    myGist->insert((void *) &binaryUUID, ID_LEN, (void *) &data, datasz);
    myGist->flush();

    //myGist->close();
    return true;
}

void Part::printAllKeys(QString treeName)
{
    QStringList results;
    gist *myGist=gists[treeName];

    bt_query_t q(bt_query_t::bt_nooper, &treeName);
    gist_cursor_t cursor;
    if(myGist->fetch_init(cursor, &q)!=RCOK)
    {
        cerr << "Can't initialize cursor." << endl;
        return ;
    }
    bool eof = false;
    smsize_t keysz=KEY_LEN, datasz=ID_LEN;

    char keyFound[KEY_LEN]={0};
    char id[ID_LEN]={0};
    while (!eof)
    {
        if(myGist->fetch(cursor, (void *)&keyFound, keysz, (void *)&id, datasz, eof)!=RCOK)
        {
            cerr << "Can't fetch from cursor." << endl;
            return ;
        }
        if (!eof)
        {
            cout<<"find:"<<(char*)&keyFound<<", id:"<<id<<endl;
            results.append(id);
        }
    }
    //cout<<"Execute Time: "<<timer.nsecsElapsed()<<" ns, record count:"<<results.count()<<endl;
}

void Part::testInserts()
{
    insertRecord("43bf9957-e944-408a-a17d-ea7ac40ffea7", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510134033000,updatedAt_14010510141304000,userId_1119,username_mojtaba.heidari,updatedById_1119,projectId_4414,projectPathWithNamespace_danabot/backlog,action_update,state_opened,assigneeId_farshad.abdi,issueId_247276,issueIid_111,labels_Backend,labels_NLP,labels_SIZE - XL,labels_اولویت: عادی,changeType_labels,dataId_86c11be6-e37e-4a65-b486-c314bc11da53,groupName_DanaBot,groupId_5272,participantId_971,participantId_1031,participantId_1119,participantId_750,participantUsername_farshad.abdi,participantUsername_maryam.najafi,participantUsername_mojtaba.heidari,participantUsername_amir.ghasemirad,milestoneId_2999}");
    insertRecord("fe4bb7f6-c44d-45e9-bae2-abba4d3ae343", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510122821000,updatedAt_14010510122857000,userId_539,username_fatemeh.salehiyeh,updatedById_539,projectId_1398,projectPathWithNamespace_Sabtyar/server/sabtyar_dotnetcore,action_update,state_opened,assigneeId_fatemeh.salehiyeh,issueId_247239,issueIid_1394,labels_تخته: دات نت,labels_نوع: باگ,labels_وضعیت: در حال انجام,changeType_description,changeType_last_edited_at,changeType_last_edited_by_id,changeType_time_estimate,changeType_updated_at,dataId_1209bc37-4eaf-4057-983a-2e3909c8ba66,groupName_Server,groupName_Sabtyar,groupId_1205,groupId_225,participantId_539,participantUsername_fatemeh.salehiyeh,milestoneId_2984}");
    insertRecord("8df2490c-20e6-4002-8b6e-411cdcb3f9ff", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010508113326000,updatedAt_14010510121506000,userId_1034,username_samangarbot,updatedById_1034,projectId_4007,projectPathWithNamespace_tahlilgaran/backlog,action_update,state_opened,assigneeId_reza.davodian,issueId_246616,issueIid_4487,labels_شرکت,labels_فوریت: غیرفوری/مهم,labels_وضعیت: آماده انجام,changeType_total_time_spent,changeType_time_change,dataId_153f11b8-8355-4537-bcb7-e798421d3d77,groupName_tahlilgaran,groupId_1154,participantId_733,participantId_1034,participantUsername_reza.davodian,participantUsername_samangarbot,milestoneId_2990}");
    insertRecord("5430cbd8-bcdd-4347-9ddf-1652954fba07", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010422174641000,updatedAt_14010510120932000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_yaser.fathi,issueId_243737,issueIid_195,labels_محیط: عملیات,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_90713fc0-9953-4e9d-a48a-8400edc80c3d,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_869,participantId_1034,participantUsername_yaser.fathi,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("4e53d239-11b4-44d3-be87-fafe55cc020d", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14001122175951000,updatedAt_14010510173034000,userId_478,username_anahita.mortazavi,updatedById_478,projectId_1327,projectPathWithNamespace_automation/modules/partServiceRocketEvents,action_update,state_opened,assigneeId_anahita.mortazavi,issueId_215531,issueIid_45,labels_اولویت: عادی,labels_نوع: بهبود,changeType_time_estimate,changeType_updated_at,dataId_e23f554b-d714-4ff2-ae86-e7268fdb76ec,groupName_modules,groupName_automation,groupId_660,groupId_91,participantId_478,participantId_516,participantUsername_anahita.mortazavi,participantUsername_azadeh.ghafourian,milestoneId_2580}");
    insertRecord("dbcb0eea-a007-4f9d-a5dd-8694ea967fd5", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510132845000,updatedAt_14010510140255000,userId_1119,username_mojtaba.heidari,updatedById_1119,projectId_4414,projectPathWithNamespace_danabot/backlog,action_update,state_opened,assigneeId_eghbal.mirzaei,issueId_247270,issueIid_110,labels_Frontend,labels_اولویت: بحرانی,changeType_milestone_id,changeType_updated_at,changeType_updated_by_id,dataId_6d25c0d7-addb-4805-b038-581973b517ff,groupName_DanaBot,groupId_5272,participantId_1153,participantId_750,participantUsername_eghbal.mirzaei,participantUsername_amir.ghasemirad,milestoneId_2999}");
    insertRecord("84c49e72-0735-4943-8e40-380951b40638", "{source_gitlab,type_mergeRequest,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010330160757000,updatedAt_14010510131936000,userId_940,username_reza.moradi,projectId_3846,projectPathWithNamespace_naturallanguageprocessing/chatbot/qa/kgqa,mergeRequestId_66956,mergeRequestIid_4,authorId_1013,state_opened,action_update,sourceBranch_nightly-SimpleQA,targetBranch_master,assigneeId_niloufar.beyranvand,labels_اولویت: بالا,labels_نوع: بهبود,changeType_updated_at,dataId_08ceeea5-ff71-4dc5-8b63-e161130bca65,groupName_QA,groupName_Chatbot,groupName_NaturalLanguageProcessing,groupId_1803,groupId_1741,groupId_1714,participantId_940,participantId_1013,participantUsername_reza.moradi,participantUsername_niloufar.beyranvand,reviewerId_940,reviewerUsername_reza.moradi,milestoneId_2661}");
    insertRecord("ab6c6cb2-64d7-44c8-85b5-73afe1a590c8", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010407171235000,updatedAt_14010510154746000,userId_349,username_mostafa.shahcheraghian,updatedById_349,projectId_2087,projectPathWithNamespace_network/hardware-team-tasks,action_close,state_closed,assigneeId_mostafa.shahcheraghian,issueId_240843,issueIid_593,labels_Doing,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_00b7321d-c40e-4ae8-a928-96a15184f72a,groupName_network,groupId_152,participantId_349,participantUsername_mostafa.shahcheraghian,milestoneId_2890}");
    insertRecord("e9ec3500-7bf2-4896-ada8-ab8f6c7a8664", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510161857000,updatedAt_14010510161932000,userId_811,username_mahdi.madanchi,updatedById_811,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247318,issueIid_5234,labels_Farashenasa,labels_PartPay,labels_Saha,labels_Signal,changeType_milestone_id,changeType_updated_at,dataId_208802c9-df35-4148-b123-648e0838d003,groupName_Support,groupId_352,participantId_811,participantId_240,participantId_735,participantId_166,participantId_62,participantId_57,participantUsername_mahdi.madanchi,participantUsername_reza.shabani,participantUsername_amirhossein.fattahi,participantUsername_mohammad.sherafat,participantUsername_saeed.shidkalaee,participantUsername_mahmoud.rezaee,milestoneId_2971}");
    insertRecord("b05e9c9b-06b9-40ec-ac60-cb56fdfd762b", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14000930101844000,updatedAt_14010510183157000,userId_1182,username_zahra.aramkhah,updatedById_1182,projectId_362,projectPathWithNamespace_automation/modules/partServiceGitEvents,action_update,state_opened,assigneeId_zahra.aramkhah,issueId_205494,issueIid_700,labels_اولویت: عادی,labels_نوع: بهبود,labels_وضعیت: در انتظار مرج,changeType_total_time_spent,changeType_time_change,dataId_07f6695f-f45a-40c9-9446-f188098ba2fe,groupName_modules,groupName_automation,groupId_660,groupId_91,participantId_1182,participantId_516,participantId_1034,participantUsername_zahra.aramkhah,participantUsername_azadeh.ghafourian,participantUsername_samangarbot,milestoneId_2580}");
    insertRecord("6483bd9b-7fee-44ea-8851-136484fce70d", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510091921000,updatedAt_14010510104849000,userId_1034,username_samangarbot,updatedById_1034,projectId_4394,projectPathWithNamespace_data-layer/cookey,action_update,state_opened,assigneeId_somayeh.nikkhou,issueId_247166,issueIid_69,labels_M,labels_اولویت: بالا,labels_تخته: پایگاه داده,labels_نوع: وظیفه,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_total_time_spent,changeType_time_change,dataId_d7ac11cb-000a-4014-821b-ab808dee6693,groupName_زیرساخت لایه داده,groupId_89,participantId_865,participantId_1034,participantUsername_somayeh.nikkhou,participantUsername_samangarbot,milestoneId_2986}");
    insertRecord("846898a7-8a9c-4298-95cb-fb563b6cec70", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510100246000,updatedAt_14010510100301000,userId_1164,username_mohammadamin.hashemi,updatedById_1164,projectId_818,projectPathWithNamespace_devops/tasks-board,action_update,state_opened,assigneeId_mohammadamin.hashemi,issueId_247178,issueIid_771,changeType_milestone_id,changeType_updated_at,changeType_updated_by_id,dataId_26487b59-caa4-434c-b8fe-99af4d59c3fd,groupName_DevOps,groupId_353,participantId_1164,participantId_277,participantUsername_mohammadamin.hashemi,participantUsername_mohammadhossein.ghaderi,milestoneId_2989}");
    insertRecord("4d2a1afd-8a67-4cf0-9032-b401e4709678", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010422174641000,updatedAt_14010510093609000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_yaser.fathi,issueId_243737,issueIid_195,labels_محیط: عملیات,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_eff3567d-cbbc-4eae-b31e-bfc1a9c2a2b5,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_869,participantId_1034,participantUsername_yaser.fathi,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("f4f5dc45-e682-438e-a683-bc87ddfb4dfb", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510165015000,updatedAt_14010510165055000,userId_1034,username_samangarbot,updatedById_1034,projectId_4007,projectPathWithNamespace_tahlilgaran/backlog,action_update,state_opened,assigneeId_masoud.zaker,issueId_247333,issueIid_4507,labels_شرکت,labels_فوریت: غیرفوری/مهم,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_total_time_spent,changeType_time_change,dataId_05bafb0c-cd6e-4e82-bc51-b75aeca5dcfb,groupName_tahlilgaran,groupId_1154,participantId_958,participantId_1034,participantUsername_masoud.zaker,participantUsername_samangarbot,milestoneId_2990}");
    insertRecord("9161e32a-8070-4a1d-89aa-6ae9857e7611", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010422122955000,updatedAt_14010510182001000,userId_1034,username_samangarbot,updatedById_1034,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_hamed.babaei,issueId_243610,issueIid_360,labels_مطالعه و بررسی,labels_نوع: بررسی و ارزیابی,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_98b9f3cc-7b81-43ad-bd33-6c62057804e3,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1034,participantUsername_hamed.babaei,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("e58f1b6a-1ab1-4bf8-b6ee-7c1d0f0d2e5a", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510091943000,updatedAt_14010510092020000,userId_57,username_mahmoud.rezaee,updatedById_57,projectId_132,projectPathWithNamespace_data-layer/infraStructure,action_update,state_opened,assigneeId_mahmoud.rezaee,issueId_247167,issueIid_1487,labels_اولویت: بالا,labels_تخته: پایگاه داده,labels_نوع: وظیفه,labels_وضعیت: آماده انجام,changeType_description,changeType_last_edited_at,changeType_last_edited_by_id,changeType_updated_at,changeType_updated_by_id,dataId_28077d9f-d3ff-4955-9369-d34d09e133c3,groupName_زیرساخت لایه داده,groupId_89,participantId_57,participantId_1034,participantUsername_mahmoud.rezaee,participantUsername_samangarbot,milestoneId_2986}");
    insertRecord("5ceec3ec-93e1-4a66-ba74-a4a9b0aa7d55", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510164814000,updatedAt_14010510182722000,userId_811,username_mahdi.madanchi,updatedById_811,projectId_1797,projectPathWithNamespace_support/24-7,action_close,state_closed,issueId_247331,issueIid_5235,labels_Other,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_40472fbf-45c2-4cd8-9a0b-8e35d496f9c9,groupName_Support,groupId_352,participantId_811,participantId_198,participantId_145,participantUsername_mahdi.madanchi,participantUsername_hojjat.ataee,participantUsername_reza.mahmoudi,milestoneId_2971}");
    insertRecord("0342c324-9697-4fad-8151-fe35a881f546", "{source_gitlab,type_mergeRequest,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510103838000,updatedAt_14010510113342000,userId_848,username_ehsan.tavan,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,mergeRequestId_69384,mergeRequestIid_62,authorId_848,state_merged,action_merge,sourceBranch_add_tsdae,targetBranch_dev,assigneeId_ehsan.tavan,changeType_state_id,changeType_updated_at,dataId_53780ce7-1359-4299-a14d-cb5aab954129,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_998,participantId_848,participantUsername_milad.molazadeh,participantUsername_ehsan.tavan,reviewerId_998,reviewerUsername_milad.molazadeh,milestoneId_2891}");
    insertRecord("07b051b1-f232-4c8f-b4e8-25597ca68505", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010505113001000,updatedAt_14010510094508000,userId_474,username_fakhri.hassanzadeh,updatedById_474,projectId_2046,projectPathWithNamespace_quality-assurance/backlog/test,action_update,state_opened,assigneeId_fakhri.hassanzadeh,issueId_246213,issueIid_928,labels_اولویت : بالا,labels_وضعیت: در حال انجام,changeType_last_edited_at,changeType_title,changeType_updated_at,dataId_5da88ca0-8bb8-486a-962e-aebf9af2ef5b,groupName_backlog,groupName_کنترل کیفیت,groupId_1174,groupId_407,participantId_474,participantUsername_fakhri.hassanzadeh,milestoneId_2970}");
    insertRecord("14f49b83-3f99-41e9-b856-a2ba8b28e0a7", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010505094319000,updatedAt_14010510160432000,userId_475,username_mahdi.aslami,updatedById_475,projectId_4736,projectPathWithNamespace_ios/server-projects/blogger/blog,action_update,state_opened,assigneeId_mahdi.aslami,issueId_246157,issueIid_162,changeType_updated_at,changeType_updated_by_id,changeType_assignees,dataId_7ad03392-8d55-4f4f-96c3-e183d4fb27cb,groupName_blogger,groupName_Server Projects,groupName_ios,groupId_2083,groupId_590,groupId_271,participantId_475,participantUsername_mahdi.aslami,milestoneId_2987}");
    insertRecord("e9729013-b1ad-4590-a128-1c81f3903fdf", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010509111254000,updatedAt_14010510170950000,userId_811,username_mahdi.madanchi,updatedById_790,projectId_1797,projectPathWithNamespace_support/24-7,action_close,state_closed,issueId_246945,issueIid_5220,labels_Signal,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_6871f44d-86c6-44df-9f57-04f3892fe036,groupName_Support,groupId_352,participantId_811,participantId_790,participantId_145,participantUsername_mahdi.madanchi,participantUsername_amin.ziaei,participantUsername_reza.mahmoudi,mentioner_devops-pr#8185,milestoneId_2971}");
    insertRecord("d05a28ed-ac2c-4bbe-b465-75579469f8d4", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010509161223000,updatedAt_14010510125930000,userId_1034,username_samangarbot,updatedById_1034,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_ali.rahmati,issueId_247076,issueIid_456,labels_نوع: تحقیق و توسعه,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_563280c1-4cd2-4da2-a7c7-c2723cee9b17,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_1029,participantId_1034,participantUsername_ali.rahmati,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("dd33b9ee-fff0-4e7e-b370-54e0e4b3f1d3", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010426135716000,updatedAt_14010510172627000,userId_1034,username_samangarbot,updatedById_1034,projectId_4175,projectPathWithNamespace_datamining/recommenderengine/recommenderengine-ai,action_update,state_opened,assigneeId_sadegh.arefizadeh,issueId_244242,issueIid_83,labels_حوزه: داده کاوی,labels_نوع: ویژگی,labels_وضعیت: آماده انجام,changeType_total_time_spent,changeType_time_change,dataId_53e09356-2f6b-4283-9966-3bb99c195965,groupName_RecommenderEngine,groupName_DataMining,groupId_1801,groupId_1710,participantId_694,participantId_1034,participantId_877,participantUsername_sadegh.arefizadeh,participantUsername_samangarbot,participantUsername_fatemeh.baghkhani,milestoneId_2966}");
    insertRecord("14575235-9879-4cdb-bd35-c8e71367c0b4", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510161857000,updatedAt_14010510173305000,userId_811,username_mahdi.madanchi,updatedById_811,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247318,issueIid_5234,labels_Farashenasa,labels_PartPay,labels_Saha,labels_Signal,changeType_description,changeType_last_edited_at,changeType_updated_at,dataId_dd41ed0b-470e-45ff-99c1-c4facf4a6a80,groupName_Support,groupId_352,participantId_811,participantId_240,participantId_735,participantId_166,participantId_62,participantId_57,participantUsername_mahdi.madanchi,participantUsername_reza.shabani,participantUsername_amirhossein.fattahi,participantUsername_mohammad.sherafat,participantUsername_saeed.shidkalaee,participantUsername_mahmoud.rezaee,milestoneId_2971}");
    insertRecord("c01b96e7-5357-4346-8a0b-047c137eab6c", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14000930101844000,updatedAt_14010510094108000,userId_337,username_infrastructreBot,updatedById_337,projectId_362,projectPathWithNamespace_automation/modules/partServiceGitEvents,action_update,state_opened,assigneeId_zahra.aramkhah,issueId_205494,issueIid_700,labels_اولویت: عادی,labels_نوع: بهبود,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_bbc969b7-73d4-4edd-b07a-58360c4f5c13,groupName_modules,groupName_automation,groupId_660,groupId_91,participantId_1182,participantId_516,participantId_1034,participantUsername_zahra.aramkhah,participantUsername_azadeh.ghafourian,participantUsername_samangarbot,milestoneId_2580}");
    insertRecord("12fe33a4-7b82-4aea-819b-04a211c63b87", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010502120707000,updatedAt_14010510164047000,userId_1128,username_fereshteh.gholami,updatedById_1128,projectId_2046,projectPathWithNamespace_quality-assurance/backlog/test,action_update,state_opened,issueId_245372,issueIid_918,labels_پیشرفت حاصل‌شده,labels_نوع: جلسه,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_4a594c62-399e-4ec3-9ff0-5613d6745d77,groupName_backlog,groupName_کنترل کیفیت,groupId_1174,groupId_407,participantId_1128,participantUsername_fereshteh.gholami,milestoneId_2970}");
    insertRecord("4ce519eb-22b9-4d9e-b030-4b3e7a2492d1", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010428171919000,updatedAt_14010510173759000,userId_1142,username_mohsen.mahmoudzadeh,updatedById_1142,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_closed,assigneeId_mohsen.mahmoudzadeh,issueId_244577,issueIid_205,labels_وضعیت: در انتظار تایید,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_2a8bd171-ee00-43f1-9637-7214436fcb0f,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_1142,participantId_1034,participantUsername_mohsen.mahmoudzadeh,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("d1a6f046-6579-48a4-b06b-be3413378b1a", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510155817000,updatedAt_14010510155817000,userId_1029,username_ali.rahmati,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_open,state_opened,assigneeId_ali.rahmati,issueId_247312,issueIid_460,labels_وضعیت: در حال انجام,labels_نوع: تحقیق و توسعه,changeType_author_id,changeType_created_at,changeType_description,changeType_id,changeType_iid,changeType_milestone_id,changeType_project_id,changeType_title,changeType_updated_at,dataId_1a77d5a8-a389-40c6-9443-f36814f96e30,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_1029,participantUsername_ali.rahmati,milestoneId_2891}");
    insertRecord("d442065c-0ee2-423d-9dd1-21a73bed9880", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010421175029000,updatedAt_14010510130216000,userId_1016,username_fateme.sheykhi,updatedById_1016,projectId_3853,projectPathWithNamespace_Sabtyar/sabtyar_ai_group/identification,action_update,state_closed,assigneeId_fateme.sheykhi,issueId_243473,issueIid_184,labels_حوزه: سرور,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_9e759e6b-b178-4f2d-ae56-840d64b3a329,groupName_Sabtyar_ai_group,groupName_Sabtyar,groupId_1726,groupId_225,participantId_1016,participantId_1034,participantUsername_fateme.sheykhi,participantUsername_samangarbot,milestoneId_1978}");
    insertRecord("be3760fd-2449-47db-ad64-b8879c2723d4", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010508121105000,updatedAt_14010510114502000,userId_1087,username_maryam.vatanparast,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_maryam.vatanparast,issueId_246639,issueIid_217,labels_وضعیت: تعلیق شده,changeType_relative_position,changeType_labels,dataId_27223367-5b5e-4c6b-abaf-73051755e8b7,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_1087,participantId_1034,participantUsername_maryam.vatanparast,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("0d65bf9a-0558-4180-8a17-2150186049b2", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510090809000,updatedAt_14010510090847000,userId_349,username_mostafa.shahcheraghian,updatedById_349,projectId_2087,projectPathWithNamespace_network/hardware-team-tasks,action_update,state_opened,assigneeId_omid.nekouei,issueId_247161,issueIid_623,labels_Doing,labels_ghv,labels_Hardware-Error,labels_HV,labels_اولویت: بالا,labels_اولویت: بحرانی,labels_پارت 3,changeType_description,changeType_last_edited_at,changeType_last_edited_by_id,changeType_updated_at,changeType_updated_by_id,dataId_55b16fd6-bdbb-4bfc-b212-97948b911dd2,groupName_network,groupId_152,participantId_854,participantId_349,participantId_58,participantId_340,participantId_890,participantUsername_omid.nekouei,participantUsername_mostafa.shahcheraghian,participantUsername_alireza.tajalli,participantUsername_ali.nourani,participantUsername_mohaddeseh.rezaee,milestoneId_2985}");
    insertRecord("a5960738-a58f-4dc5-844c-bf0c042a192b", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510155506000,updatedAt_14010510162946000,userId_1034,username_samangarbot,updatedById_1034,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_ali.rahmati,issueId_247309,issueIid_459,labels_نوع: بررسی و ارزیابی,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_d5f98dcc-f633-4169-82b3-3a21a0339642,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_1029,participantId_1034,participantUsername_ali.rahmati,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("056922a0-2c1f-4df6-b992-aa2a66b4ed70", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010508101053000,updatedAt_14010510103421000,userId_337,username_infrastructreBot,updatedById_337,projectId_1398,projectPathWithNamespace_Sabtyar/server/sabtyar_dotnetcore,action_update,state_closed,assigneeId_fatemeh.salehiyeh,issueId_246555,issueIid_1390,labels_تخته: دات نت,labels_نوع: ویژگی,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_d8da000d-d017-489b-8282-0cdcebec9af3,groupName_Server,groupName_Sabtyar,groupId_1205,groupId_225,participantId_539,participantUsername_fatemeh.salehiyeh,milestoneId_2984}");
    insertRecord("7dd6e88a-141b-4ca2-862e-413434806611", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510123944000,updatedAt_14010510141107000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_yaser.fathi,issueId_247243,issueIid_222,labels_نوع: مستندسازی,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_497fc332-3ae2-4a52-a271-eacf67652ade,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_869,participantId_1034,participantUsername_yaser.fathi,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("6f474278-5798-44d6-a368-a31adbdaf2ad", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510165015000,updatedAt_14010510165015000,userId_958,username_masoud.zaker,projectId_4007,projectPathWithNamespace_tahlilgaran/backlog,action_open,state_opened,assigneeId_masoud.zaker,issueId_247333,issueIid_4507,labels_وضعیت: در حال انجام,labels_فوریت: غیرفوری/مهم,labels_شرکت,changeType_author_id,changeType_created_at,changeType_description,changeType_id,changeType_iid,changeType_milestone_id,changeType_project_id,changeType_time_estimate,changeType_title,changeType_updated_at,dataId_906a98a6-e669-49e6-ab99-6d183a75ce2c,groupName_tahlilgaran,groupId_1154,participantId_958,participantId_1034,participantUsername_masoud.zaker,participantUsername_samangarbot,milestoneId_2990}");
    insertRecord("8876564f-e82d-42ab-bc15-47ffb8912ee5", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010502154443000,updatedAt_14010510155755000,userId_325,username_sara.kazemzadeh,updatedById_325,projectId_2047,projectPathWithNamespace_quality-assurance/backlog/support,action_update,state_opened,assigneeId_sara.kazemzadeh,issueId_245485,issueIid_446,labels_نوع: پیگیری,changeType_total_time_spent,changeType_time_change,dataId_35311979-19b9-4f89-9b7a-68b8c5053cbf,groupName_backlog,groupName_کنترل کیفیت,groupId_1174,groupId_407,participantId_325,participantUsername_sara.kazemzadeh,milestoneId_2970}");
    insertRecord("69380962-ea41-46c2-8e12-391ac7badfd0", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010303112342000,updatedAt_14010510200815000,userId_976,username_hamed.babaei,updatedById_976,projectId_4746,projectPathWithNamespace_danabot/rahtak_ai_group/nlp-backlog,action_update,state_opened,assigneeId_hamed.babaei,issueId_233392,issueIid_27,labels_نوع: بهبود,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_total_time_spent,changeType_time_change,dataId_89b97ae5-5871-468f-8381-0f97dc51da60,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1029,participantId_1034,participantId_1031,participantId_967,participantUsername_hamed.babaei,participantUsername_ali.rahmati,participantUsername_samangarbot,participantUsername_maryam.najafi,participantUsername_marzieh.zargari,mentioner_#44,milestoneId_2891}");
    insertRecord("23601e13-e92b-4046-b890-51edec501168", "{source_gitlab,type_mergeRequest,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010504142923000,updatedAt_14010510093947000,userId_1182,username_zahra.aramkhah,updatedById_516,projectId_362,projectPathWithNamespace_automation/modules/partServiceGitEvents,mergeRequestId_69080,mergeRequestIid_757,authorId_1182,state_opened,action_update,sourceBranch_700-fixed-gitbeaker-node5,targetBranch_develop,assigneeId_azadeh.ghafourian,labels_اولویت: عادی,labels_نوع: بهبود,labels_وضعیت: آماده انجام,dataId_f066d835-0e12-4e11-95fe-cab5b94183f5,groupName_modules,groupName_automation,groupId_660,groupId_91,participantId_952,participantId_516,participantId_1182,participantId_337,participantUsername_farzaneh.hallaji,participantUsername_azadeh.ghafourian,participantUsername_zahra.aramkhah,participantUsername_infrastructreBot,reviewerId_952,reviewerUsername_farzaneh.hallaji,milestoneId_2580}");
    insertRecord("154544f2-e315-4f14-bcf0-9074ee21712e", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010503124913000,updatedAt_14010510170935000,userId_1167,username_sara.safaee,updatedById_1167,projectId_2047,projectPathWithNamespace_quality-assurance/backlog/support,action_update,state_opened,assigneeId_sara.safaee,issueId_245683,issueIid_449,labels_نوع: بررسی و ارزیابی,changeType_due_date,changeType_updated_at,dataId_3f400e69-c8a8-4ced-a720-c5a260de97d7,groupName_backlog,groupName_کنترل کیفیت,groupId_1174,groupId_407,participantId_1167,participantId_325,participantId_290,participantUsername_sara.safaee,participantUsername_sara.kazemzadeh,participantUsername_niloufar.karimi,milestoneId_2970}");
    insertRecord("c73a1023-e8a7-4502-8c08-49fa6d970acd", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010503124913000,updatedAt_14010510170642000,userId_1167,username_sara.safaee,updatedById_1167,projectId_2047,projectPathWithNamespace_quality-assurance/backlog/support,action_update,state_opened,assigneeId_sara.safaee,issueId_245683,issueIid_449,labels_نوع: بررسی و ارزیابی,changeType_time_estimate,changeType_updated_at,dataId_98d87212-44e8-43dd-adf5-ebfdbf855c24,groupName_backlog,groupName_کنترل کیفیت,groupId_1174,groupId_407,participantId_1167,participantId_325,participantId_290,participantUsername_sara.safaee,participantUsername_sara.kazemzadeh,participantUsername_niloufar.karimi,milestoneId_2970}");
    insertRecord("20975f48-4940-47bb-81f1-c22ac21a73b7", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010505092029000,updatedAt_14010510162925000,userId_1200,username_mahdieh.tarikhchi,updatedById_1200,projectId_4966,projectPathWithNamespace_network/internship/mahdie.tarikhchi,action_update,state_opened,assigneeId_mahdieh.tarikhchi,issueId_246146,issueIid_14,labels_Doing,labels_Programming,changeType_total_time_spent,changeType_time_change,dataId_8d246571-456c-4401-8ad9-ac8db343e45d,groupName_internship,groupName_network,groupId_2074,groupId_152,participantId_1200,participantUsername_mahdieh.tarikhchi,milestoneId_2890}");
    insertRecord("dda62c7b-8bc5-4c0b-98e8-c533b9d64b9d", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010505092903000,updatedAt_14010510113427000,userId_1034,username_samangarbot,updatedById_1034,projectId_132,projectPathWithNamespace_data-layer/infraStructure,action_update,state_opened,assigneeId_somayeh.nikkhou,issueId_246148,issueIid_1480,labels_S,labels_اولویت: بالا,labels_تخته: پایگاه داده,labels_نوع: پشتیبانی,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_d896c37f-e7a7-4564-a812-f6c3b923529b,groupName_زیرساخت لایه داده,groupId_89,participantId_865,participantId_1034,participantUsername_somayeh.nikkhou,participantUsername_samangarbot,milestoneId_2986}");
    insertRecord("11e11bfd-cbdb-4668-9f14-9f906cc06dbe", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510195252000,updatedAt_14010510195318000,userId_811,username_mahdi.madanchi,updatedById_811,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247364,issueIid_5237,labels_Network-Enterprise,labels_Other,changeType_milestone_id,changeType_updated_at,dataId_b4b26fab-117b-414b-abcc-69d8a40a1fe6,groupName_Support,groupId_352,participantId_811,participantId_145,participantUsername_mahdi.madanchi,participantUsername_reza.mahmoudi,mentioner_network_support#9574,milestoneId_2971}");
    insertRecord("6f8033c8-061d-4e6a-82ca-c649b78b2d9c", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010430202517000,updatedAt_14010511175728000,userId_1072,username_shahrzad.zourkians,updatedById_1072,projectId_1796,projectPathWithNamespace_7-24_team/7-24,action_update,state_opened,assigneeId_shahrzad.zourkians,issueId_244909,issueIid_315,changeType_total_time_spent,changeType_time_change,dataId_6a8e309d-a24b-4faa-af22-ef2930496bbc,groupName_7-24_team,groupId_1517,participantId_1072,participantUsername_shahrzad.zourkians,milestoneId_2969}");
    insertRecord("e53c3676-0127-4d84-a35a-b62a009df4ce", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510155506000,updatedAt_14010510161812000,userId_1034,username_samangarbot,updatedById_1034,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_ali.rahmati,issueId_247309,issueIid_459,labels_نوع: بررسی و ارزیابی,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_total_time_spent,changeType_time_change,dataId_40a41c2c-b37d-4d1a-9c1a-797e2dd8bdc4,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_1029,participantId_1034,participantUsername_ali.rahmati,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("9b915594-067d-4af4-9837-a285e49c7e81", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14000401131623000,updatedAt_14010510094849000,userId_516,username_azadeh.ghafourian,updatedById_516,projectId_2272,projectPathWithNamespace_automation/backlog,action_update,state_opened,issueId_118345,issueIid_4,labels_اولویت: عادی,labels_وضعیت: تعلیق شده,changeType_labels,dataId_580ba9b9-cc7a-411b-99d8-da621df2abdc,groupName_automation,groupId_91,participantId_516,participantId_662,participantId_337,participantUsername_azadeh.ghafourian,participantUsername_mohammadhossein.kazemi,participantUsername_infrastructreBot,milestoneId_2581}");
    insertRecord("1a3dc4ed-5576-4d6b-8173-d73351f184ac", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510154904000,updatedAt_14010510154904000,userId_349,username_mostafa.shahcheraghian,projectId_2087,projectPathWithNamespace_network/hardware-team-tasks,action_open,state_opened,assigneeId_mostafa.shahcheraghian,issueId_247306,issueIid_625,labels_Doing,changeType_author_id,changeType_created_at,changeType_description,changeType_due_date,changeType_id,changeType_iid,changeType_milestone_id,changeType_project_id,changeType_title,changeType_updated_at,dataId_537fdece-22b7-408d-993b-5affb36e8fe9,groupName_network,groupId_152,participantId_349,participantUsername_mostafa.shahcheraghian,milestoneId_2985}");
    insertRecord("465ad041-1b76-4515-b996-d905fd1319b9", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010508101053000,updatedAt_14010510103420000,userId_539,username_fatemeh.salehiyeh,updatedById_539,projectId_1398,projectPathWithNamespace_Sabtyar/server/sabtyar_dotnetcore,action_close,state_closed,assigneeId_fatemeh.salehiyeh,issueId_246555,issueIid_1390,labels_تخته: دات نت,labels_نوع: ویژگی,labels_وضعیت: در حال انجام,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_c2546219-6f0f-47ce-bf27-79b37c6e5f23,groupName_Server,groupName_Sabtyar,groupId_1205,groupId_225,participantId_539,participantUsername_fatemeh.salehiyeh,milestoneId_2984}");
    insertRecord("255b547c-8ffa-49a9-8a3f-3250ffb5e320", "{source_gitlab,type_issue,year_1400,yearMonth_140007,yearMonthDay_14000712,createdAt_14000712135926000,username_kiana.jalali,userId_642,projectId_132,projectPathWithNamespace_data-layer/infraStructure,action_update,state_opened,issueId_133201,issueIid_1009,changeType_milestone_id,changeType_updated_at,changeType_updated_by_id,dataId_45f43473-6906-4b8f-8b7b-f0fc3d9626e5,groupName_زیرساخت لایه داده,groupId_89,participantId_642,participantId_526,participantId_62,participantId_653,participantId_681,participantId_865,participantId_866,participantId_57,participantId_166,participantId_387,participantUsername_kiana.jalali,participantUsername_farzaneh.mohtasham,participantUsername_saeed.shidkalaee,participantUsername_sahar.refaee,participantUsername_somayeh.abdoljavadi,participantUsername_somayeh.nikkhou,participantUsername_negin.haddad,participantUsername_mahmoud.rezaee,participantUsername_mohammad.sherafat,participantUsername_zahra.amirmahani,milestoneId_1169}");
    insertRecord("cf43208f-8129-4967-a125-37a31cf72801", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010509124207000,updatedAt_14010510095236000,userId_337,username_infrastructreBot,updatedById_337,projectId_478,projectPathWithNamespace_signal/android/signal_android,action_update,state_closed,assigneeId_sadegh.bashi,issueId_246999,issueIid_1512,labels_اولویت: بالا,labels_تخته: اندروید,labels_محیط: توسعه,labels_نوع: بهبود,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_ef89cb74-fbb2-4b33-ab5e-2312dbf9965c,groupName_Android,groupName_signal,groupId_1196,groupId_85,participantId_208,participantId_113,participantUsername_sadegh.bashi,participantUsername_hossein.kelidari,milestoneId_2972}");
    insertRecord("9145658d-5be6-411d-8cd1-54fa6aa4251c", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510012902000,updatedAt_14010510090305000,userId_145,username_reza.mahmoudi,projectId_1797,projectPathWithNamespace_support/24-7,action_close,state_closed,issueId_247143,issueIid_5226,labels_Netflow,labels_SOC,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_2bc3d343-3164-4442-9018-0a7f77d977f4,groupName_Support,groupId_352,participantId_792,participantId_1208,participantId_145,participantId_50,participantUsername_razieh.ghavami,participantUsername_mohammad.askari,participantUsername_reza.mahmoudi,participantUsername_alireza.abedini,milestoneId_2971}");
    insertRecord("d018103b-8b55-4a92-92e6-1b3d608ead47", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510002946000,updatedAt_14010510095029000,userId_1072,username_shahrzad.zourkians,updatedById_1208,projectId_1797,projectPathWithNamespace_support/24-7,action_close,state_closed,issueId_247140,issueIid_5224,labels_Signal,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_38b0f8da-eb51-4821-9169-bdcfb904a8b8,groupName_Support,groupId_352,participantId_108,participantId_145,participantId_265,participantId_1072,participantId_1208,participantId_565,participantId_313,participantId_735,participantUsername_kamyar.nourbakhsh,participantUsername_reza.mahmoudi,participantUsername_hossein.akbarian,participantUsername_shahrzad.zourkians,participantUsername_mohammad.askari,participantUsername_iman.safarzadeh,participantUsername_hamidreza.akbarnezhad,participantUsername_amirhossein.fattahi,milestoneId_2971}");
    insertRecord("521236bb-6895-492a-8f84-bc4b6d4aaa60", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510123944000,updatedAt_14010510145721000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_yaser.fathi,issueId_247243,issueIid_222,labels_نوع: مستندسازی,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_c302e0cf-af81-4d18-901e-dcbd016e88e2,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_869,participantId_1034,participantUsername_yaser.fathi,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("bd05411e-5688-430c-94b9-ad5e9bb5ff01", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510171810000,updatedAt_14010510172904000,userId_405,username_mohsen.sameti,updatedById_405,projectId_478,projectPathWithNamespace_signal/android/signal_android,action_update,state_opened,assigneeId_mohsen.sameti,issueId_247336,issueIid_1514,labels_تخته: اندروید,labels_محیط: عملیات,labels_نوع: باگ,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_total_time_spent,changeType_time_change,dataId_31b66749-dc31-40b8-9358-adcd4ad3c0d1,groupName_Android,groupName_signal,groupId_1196,groupId_85,participantId_405,participantUsername_mohsen.sameti,milestoneId_2972}");
    insertRecord("bcfd3318-94e3-48c7-8f4d-8b96a01add31", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14001122175951000,updatedAt_14010510173017000,userId_478,username_anahita.mortazavi,updatedById_478,projectId_1327,projectPathWithNamespace_automation/modules/partServiceRocketEvents,action_update,state_opened,assigneeId_anahita.mortazavi,issueId_215531,issueIid_45,labels_اولویت: عادی,labels_نوع: بهبود,changeType_time_estimate,changeType_updated_at,dataId_e5de5464-4890-4410-ad80-74153fea23d7,groupName_modules,groupName_automation,groupId_660,groupId_91,participantId_478,participantId_516,participantUsername_anahita.mortazavi,participantUsername_azadeh.ghafourian,milestoneId_2580}");
    insertRecord("64267f4b-bf32-4d21-a939-adf005c2b8af", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010422174641000,updatedAt_14010510120932000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_yaser.fathi,issueId_243737,issueIid_195,labels_محیط: عملیات,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_3525e8c4-b381-4397-8d24-6d31d181dc0d,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_869,participantId_1034,participantUsername_yaser.fathi,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("62145602-ee8a-4931-b75b-256d265a7594", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510161857000,updatedAt_14010510163048000,userId_240,username_reza.shabani,updatedById_240,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247318,issueIid_5234,labels_Farashenasa,labels_PartPay,labels_Saha,labels_Signal,changeType_description,changeType_last_edited_at,changeType_updated_at,dataId_cab67d75-4e75-4957-a98c-e8353dde9cc1,groupName_Support,groupId_352,participantId_811,participantId_240,participantId_735,participantId_166,participantId_62,participantId_57,participantUsername_mahdi.madanchi,participantUsername_reza.shabani,participantUsername_amirhossein.fattahi,participantUsername_mohammad.sherafat,participantUsername_saeed.shidkalaee,participantUsername_mahmoud.rezaee,milestoneId_2971}");
    insertRecord("8225a811-8865-4be4-b03c-7fcffd1c019f", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010422174641000,updatedAt_14010510120932000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_yaser.fathi,issueId_243737,issueIid_195,labels_محیط: عملیات,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_c7dbe494-d089-4f9b-a5d6-8de967aa4d7f,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_869,participantId_1034,participantUsername_yaser.fathi,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("d600c687-4b29-4714-aeac-d0e90ab89aa0", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010507221747000,updatedAt_14010510193631000,userId_337,username_infrastructreBot,updatedById_337,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_closed,issueId_246466,issueIid_5197,labels_iSahab,labels_Netflow,labels_SOC,changeType_labels,dataId_074eaf9f-1792-401a-a27f-52b6a788a557,groupName_Support,groupId_352,participantId_792,participantId_811,participantId_1072,participantId_1208,participantId_145,participantId_50,participantUsername_razieh.ghavami,participantUsername_mahdi.madanchi,participantUsername_shahrzad.zourkians,participantUsername_mohammad.askari,participantUsername_reza.mahmoudi,participantUsername_alireza.abedini,mentioner_network/network-security-bridge#59,milestoneId_2971}");
    insertRecord("d586aa76-63f6-455d-b3f1-77953d5b5eef", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010504173221000,updatedAt_14010510170716000,userId_1181,username_fatemeh.namjou,updatedById_1181,projectId_2046,projectPathWithNamespace_quality-assurance/backlog/test,action_update,state_opened,assigneeId_fatemeh.namjou,issueId_246106,issueIid_927,labels_اولویت : عادی,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_0c814c94-b9e1-471e-837e-b2e772e22419,groupName_backlog,groupName_کنترل کیفیت,groupId_1174,groupId_407,participantId_1181,participantUsername_fatemeh.namjou,milestoneId_2970}");
    insertRecord("519deb80-e1c9-43f0-97cd-5645e0b94b8f", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010504104308000,updatedAt_14010510143703000,userId_1222,username_nima.mahpour,updatedById_1222,projectId_4969,projectPathWithNamespace_network/internship/nima.mahpour,action_update,state_opened,assigneeId_nima.mahpour,issueId_245893,issueIid_4,labels_Doing,labels_Education,changeType_description,changeType_last_edited_at,changeType_updated_at,dataId_bc7fb76f-8d12-426f-956a-691a80d4eba3,groupName_internship,groupName_network,groupId_2074,groupId_152,participantId_1222,participantId_1110,participantUsername_nima.mahpour,participantUsername_sina.samadzad,mentioner_#1,milestoneId_2890}");
    insertRecord("7e8a620c-d26e-4918-a85a-85542afa434b", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010404175255000,updatedAt_14010510130212000,userId_1016,username_fateme.sheykhi,updatedById_1034,projectId_3853,projectPathWithNamespace_Sabtyar/sabtyar_ai_group/identification,action_close,state_closed,assigneeId_fateme.sheykhi,issueId_239992,issueIid_180,labels_حوزه: سرور,labels_وضعیت: در انتظار تایید,changeType_closed_at,changeType_relative_position,changeType_state_id,changeType_updated_at,dataId_2e4d25d7-8d6d-475c-816d-ab78d4ae9db2,groupName_Sabtyar_ai_group,groupName_Sabtyar,groupId_1726,groupId_225,participantId_1016,participantId_1034,participantUsername_fateme.sheykhi,participantUsername_samangarbot,milestoneId_1978}");
    insertRecord("beffe992-16c1-48a2-b3d5-1499977b3035", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510130003000,updatedAt_14010510130104000,userId_1016,username_fateme.sheykhi,updatedById_1016,projectId_3853,projectPathWithNamespace_Sabtyar/sabtyar_ai_group/identification,action_update,state_opened,assigneeId_fateme.sheykhi,issueId_247255,issueIid_187,labels_حوزه: سرور,changeType_description,changeType_last_edited_at,changeType_last_edited_by_id,changeType_updated_at,dataId_4b976aae-b08f-43a3-b11b-19e8f910bda5,groupName_Sabtyar_ai_group,groupName_Sabtyar,groupId_1726,groupId_225,participantId_1016,participantId_1034,participantUsername_fateme.sheykhi,participantUsername_samangarbot,milestoneId_1978}");
    insertRecord("411dda2a-f748-4adb-9fef-6e94c0b33cdb", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510091943000,updatedAt_14010510125832000,userId_1034,username_samangarbot,updatedById_1034,projectId_132,projectPathWithNamespace_data-layer/infraStructure,action_update,state_opened,assigneeId_mahmoud.rezaee,issueId_247167,issueIid_1487,labels_اولویت: بالا,labels_تخته: پایگاه داده,labels_نوع: وظیفه,labels_وضعیت: آماده انجام,changeType_total_time_spent,changeType_time_change,dataId_72c7a2cb-0266-4897-893f-24a95832fb1d,groupName_زیرساخت لایه داده,groupId_89,participantId_57,participantId_1034,participantUsername_mahmoud.rezaee,participantUsername_samangarbot,milestoneId_2986}");
    insertRecord("a2cecb51-b5c9-4bf6-89c6-f09c05cbd018", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010508170507000,updatedAt_14010510161239000,userId_1034,username_samangarbot,updatedById_1034,projectId_4007,projectPathWithNamespace_tahlilgaran/backlog,action_update,state_opened,assigneeId_masoud.zaker,issueId_246795,issueIid_4497,labels_شرکت,labels_فوریت: غیرفوری/مهم,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_a986b7e0-9e89-4838-b7f4-55c30b135420,groupName_tahlilgaran,groupId_1154,participantId_958,participantId_1034,participantUsername_masoud.zaker,participantUsername_samangarbot,milestoneId_2990}");
    insertRecord("de291553-c10f-45db-9b2f-d26bc0b09b6d", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010511095113000,updatedAt_14010511100716000,userId_790,username_amin.ziaei,updatedById_790,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247414,issueIid_5248,changeType_milestone_id,changeType_updated_at,changeType_updated_by_id,dataId_c8913fef-6025-4386-9794-42a71d72b7d2,groupName_Support,groupId_352,participantId_507,participantId_790,participantId_1069,participantId_223,participantId_240,participantId_484,participantId_936,participantId_381,participantUsername_khadijeh.shokati,participantUsername_amin.ziaei,participantUsername_moeen.tavakoli,participantUsername_ali.jafari,participantUsername_reza.shabani,participantUsername_ali.nourollahi,participantUsername_mojtaba.hosseinalinezhad,participantUsername_saeed.kazemi,mentioner_#5247,milestoneId_2971}");
    insertRecord("d7a9db95-513a-4c59-95bd-fa26937e538f", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510220325000,updatedAt_14010510220429000,userId_811,username_mahdi.madanchi,updatedById_811,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247368,issueIid_5239,labels_Network-LMS,changeType_description,changeType_last_edited_at,changeType_last_edited_by_id,changeType_updated_at,dataId_c0500e4d-9235-4ff8-bb34-c2a6efa312fa,groupName_Support,groupId_352,participantId_811,participantId_145,participantUsername_mahdi.madanchi,participantUsername_reza.mahmoudi,mentioner_network_support#9575,milestoneId_2971}");
    insertRecord("567bb71f-f440-41df-afe9-767b31c8fa95", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510200946000,updatedAt_14010510200954000,userId_976,username_hamed.babaei,updatedById_976,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_hamed.babaei,issueId_247366,issueIid_461,labels_وضعیت: در حال انجام,changeType_milestone_id,changeType_updated_at,dataId_80f59d5f-edd9-4aab-9743-25459df550b6,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1034,participantUsername_hamed.babaei,participantUsername_samangarbot,milestoneId_2999}");
    insertRecord("7daae342-f4a5-4261-ad62-edc1680d5dd8", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510200946000,updatedAt_14010510201130000,userId_1034,username_samangarbot,updatedById_1034,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_hamed.babaei,issueId_247366,issueIid_461,labels_مطالعه و بررسی,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_total_time_spent,changeType_time_change,dataId_116aa8f5-9284-452d-8294-4613d5d27c98,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1034,participantUsername_hamed.babaei,participantUsername_samangarbot,milestoneId_2999}");
    insertRecord("a0eedbac-2bfd-497b-aa9b-b110ddc04de5", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010422122955000,updatedAt_14010510200621000,userId_976,username_hamed.babaei,updatedById_976,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_hamed.babaei,issueId_243610,issueIid_360,labels_مطالعه و بررسی,labels_نوع: بررسی و ارزیابی,labels_وضعیت: در انتظار تایید,changeType_relative_position,changeType_labels,dataId_8641b341-bf4d-44e9-9f43-be7e786392f2,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1034,participantUsername_hamed.babaei,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("f541db3e-2539-43ab-aa02-8c4dd77051a7", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010510220325000,updatedAt_14010510220356000,userId_811,username_mahdi.madanchi,updatedById_811,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247368,issueIid_5239,labels_Network-LMS,changeType_milestone_id,changeType_updated_at,dataId_bb3979f4-88fe-4589-8681-ea3287ebb344,groupName_Support,groupId_352,participantId_811,participantId_145,participantUsername_mahdi.madanchi,participantUsername_reza.mahmoudi,mentioner_network_support#9575,milestoneId_2971}");
    insertRecord("bbd9ef40-3a52-4484-976b-fce9c4b0fce6", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010509161604000,updatedAt_14010510222457000,userId_1034,username_samangarbot,updatedById_1034,projectId_4007,projectPathWithNamespace_tahlilgaran/backlog,action_update,state_opened,assigneeId_behnam.akbari,issueId_247079,issueIid_4499,labels_پروژه: ساتراپ/ملی,labels_حوزه: پشتیبانی,labels_فوریت: غیرفوری/مهم,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_total_time_spent,changeType_time_change,dataId_02c623c7-7b2e-41ec-9d44-a52511122427,groupName_tahlilgaran,groupId_1154,participantId_737,participantId_1034,participantUsername_behnam.akbari,participantUsername_samangarbot,milestoneId_2990}");
    insertRecord("7abc8ff3-c5d8-47cb-835b-41816839271b", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010404193615000,updatedAt_14010510200905000,userId_1034,username_samangarbot,updatedById_1034,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_opened,assigneeId_hamed.babaei,issueId_240000,issueIid_281,labels_نوع: بهبود,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_7f3de435-3266-420c-9b90-0d1b46f86c61,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1034,participantUsername_hamed.babaei,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("f17acc67-3a4f-4961-91d5-c0bec673aaa2", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010505021223000,updatedAt_14010510224314000,userId_811,username_mahdi.madanchi,projectId_1797,projectPathWithNamespace_support/24-7,action_close,state_closed,issueId_246117,issueIid_5178,labels_Signal,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_28b64151-5eb6-4875-acc7-32aef92244ae,groupName_Support,groupId_352,participantId_811,participantId_1208,participantId_48,participantId_735,participantId_44,participantId_145,participantId_108,participantUsername_mahdi.madanchi,participantUsername_mohammad.askari,participantUsername_koosha.foroughi,participantUsername_amirhossein.fattahi,participantUsername_mohammad.rahmati,participantUsername_reza.mahmoudi,participantUsername_kamyar.nourbakhsh,milestoneId_2971}");
    insertRecord("a3678e34-3d05-4611-a1dd-36f9ca35eaa4", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010501143923000,updatedAt_14010510230950000,userId_1208,username_mohammad.askari,updatedById_1208,projectId_1796,projectPathWithNamespace_7-24_team/7-24,action_update,state_opened,issueId_245129,issueIid_318,changeType_time_estimate,changeType_updated_at,dataId_98f46b0c-aa98-4f91-8a4b-0f32db71b696,groupName_7-24_team,groupId_1517,participantId_1208,participantUsername_mohammad.askari,milestoneId_2969}");
    insertRecord("20063b53-c75f-43f2-9f30-34780636364b", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010406144745000,updatedAt_14010511150557000,userId_866,username_negin.haddad,updatedById_1034,projectId_4394,projectPathWithNamespace_data-layer/cookey,action_close,state_closed,assigneeId_negin.haddad,issueId_240495,issueIid_48,labels_L,labels_اولویت: بحرانی,labels_تخته: پایگاه داده,labels_نوع: باگ,labels_وضعیت: در حال انجام,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_509a4071-1fa1-47c9-9499-314c4975aede,groupName_زیرساخت لایه داده,groupId_89,participantId_866,participantId_1034,participantId_681,participantId_387,participantUsername_negin.haddad,participantUsername_samangarbot,participantUsername_somayeh.abdoljavadi,participantUsername_zahra.amirmahani,milestoneId_2986}");
    insertRecord("b3699d77-7fcd-4880-9e24-81a83cdfc02c", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010508211843000,updatedAt_14010510224602000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_farnaz.babalou,issueId_246815,issueIid_220,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_c118de93-88c1-4084-9481-4897ded1f061,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_1058,participantId_1034,participantUsername_farnaz.babalou,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("e1241930-c875-4fe5-95c9-26de0e057687", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010506062916000,updatedAt_14010510223801000,userId_1208,username_mohammad.askari,updatedById_1208,projectId_1797,projectPathWithNamespace_support/24-7,action_close,state_closed,issueId_246437,issueIid_5188,labels_Ai-Services,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_a4661de3-b76c-4702-8ef7-76c4764fc12b,groupName_Support,groupId_352,participantId_1208,participantId_796,participantId_145,participantUsername_mohammad.askari,participantUsername_emad.hadian,participantUsername_reza.mahmoudi,milestoneId_2971}");
    insertRecord("c544abcf-fd39-47a7-8cb6-c1b6c26c2ca6", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010510,createdAt_14010508211843000,updatedAt_14010510225516000,userId_1034,username_samangarbot,updatedById_1034,projectId_4331,projectPathWithNamespace_naturallanguageprocessing/g2p/g2p,action_update,state_opened,assigneeId_farnaz.babalou,issueId_246815,issueIid_220,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_f280af4e-e849-48d4-b525-6ab5df72b767,groupName_G2P,groupName_NaturalLanguageProcessing,groupId_1763,groupId_1714,participantId_1058,participantId_1034,participantUsername_farnaz.babalou,participantUsername_samangarbot,milestoneId_2897}");
    insertRecord("587b1506-7487-44da-8bb0-e2b0c92d1483", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010511005944000,updatedAt_14010511005944000,userId_1208,username_mohammad.askari,projectId_1797,projectPathWithNamespace_support/24-7,action_open,state_opened,issueId_247371,issueIid_5241,labels_Network,labels_Other,changeType_author_id,changeType_created_at,changeType_description,changeType_id,changeType_iid,changeType_milestone_id,changeType_project_id,changeType_title,changeType_updated_at,dataId_92a2a18f-e35c-4909-9a85-d9084b64fa5f,groupName_Support,groupId_352,participantId_1208,participantId_145,participantUsername_mohammad.askari,participantUsername_reza.mahmoudi,milestoneId_2971}");
    insertRecord("f8880f1a-8be9-4b5c-bdc5-cabf16f45a21", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010511013030000,updatedAt_14010511013030000,userId_1208,username_mohammad.askari,projectId_1797,projectPathWithNamespace_support/24-7,action_open,state_opened,issueId_247375,issueIid_5243,labels_Network-LMS,changeType_author_id,changeType_created_at,changeType_description,changeType_id,changeType_iid,changeType_milestone_id,changeType_project_id,changeType_title,changeType_updated_at,dataId_5ad75a30-1c06-4057-919a-fa7414a54526,groupName_Support,groupId_352,participantId_1208,participantId_145,participantUsername_mohammad.askari,participantUsername_reza.mahmoudi,milestoneId_2971}");
    insertRecord("baa0e88f-8750-4af3-9b7b-b159829d56d2", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010508151629000,updatedAt_14010511173406000,userId_526,username_farzaneh.mohtasham,updatedById_526,projectId_1587,projectPathWithNamespace_data-layer/atlas/servers/atlas-service,action_update,state_opened,assigneeId_farzaneh.mohtasham,issueId_246727,issueIid_1146,labels_L,labels_اولویت: بالا,labels_تخته: پایگاه داده,labels_نوع: باگ,labels_وضعیت: در حال انجام,changeType_updated_at,changeType_updated_by_id,changeType_total_time_spent,changeType_time_change,dataId_d6cb9bea-c2fe-432d-9d53-ad1b7dd3dda5,groupName_servers,groupName_Atlas,groupName_زیرساخت لایه داده,groupId_1059,groupId_1039,groupId_89,participantId_526,participantId_1034,participantId_866,participantUsername_farzaneh.mohtasham,participantUsername_samangarbot,participantUsername_negin.haddad,milestoneId_2986}");
    insertRecord("3f90284f-b89d-44f0-85ad-be49f1b16e4e", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010511011720000,updatedAt_14010511011720000,userId_1208,username_mohammad.askari,projectId_1797,projectPathWithNamespace_support/24-7,action_open,state_opened,issueId_247373,issueIid_5242,labels_Network-LMS,changeType_author_id,changeType_created_at,changeType_description,changeType_id,changeType_iid,changeType_milestone_id,changeType_project_id,changeType_title,changeType_updated_at,dataId_d2ffe075-e433-4de3-9a88-474394eba501,groupName_Support,groupId_352,participantId_1208,participantId_145,participantUsername_mohammad.askari,participantUsername_reza.mahmoudi,milestoneId_2971}");
    insertRecord("51bc070f-118b-41f0-bd62-0b1b8581159c", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010511005944000,updatedAt_14010511010637000,userId_1208,username_mohammad.askari,updatedById_1208,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_247371,issueIid_5241,labels_Network,labels_Network-LMS,labels_Other,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_8c92af5b-6133-47a9-96d8-613bde8ab000,groupName_Support,groupId_352,participantId_1208,participantId_145,participantUsername_mohammad.askari,participantUsername_reza.mahmoudi,milestoneId_2971}");
    insertRecord("06c1440d-67d8-4716-920a-e5c3ef7b2b51", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010501131559000,updatedAt_14010511143804000,userId_1031,username_maryam.najafi,updatedById_1029,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_close,state_closed,assigneeId_ali.rahmati,issueId_245082,issueIid_398,labels_نوع: تحقیق و توسعه,labels_وضعیت: در انتظار تایید,changeType_closed_at,changeType_relative_position,changeType_state_id,changeType_updated_at,dataId_7f49e035-0855-4590-8975-61f39ba6b09c,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_1029,participantId_1034,participantUsername_ali.rahmati,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("069687e8-0832-4005-bb60-8fa8f31742d9", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010506060212000,updatedAt_14010511013423000,userId_337,username_infrastructreBot,updatedById_337,projectId_1797,projectPathWithNamespace_support/24-7,action_update,state_opened,issueId_246435,issueIid_5186,labels_Signal,labels_وضعیت: آماده انجام,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_0f739809-ae15-482e-b7f4-5d02038ebb16,groupName_Support,groupId_352,participantId_1208,participantId_811,participantId_790,participantId_145,participantId_1073,participantUsername_mohammad.askari,participantUsername_mahdi.madanchi,participantUsername_amin.ziaei,participantUsername_reza.mahmoudi,participantUsername_kosar.soltani,milestoneId_2971}");
    insertRecord("a7b9729f-2b65-45c2-8e55-e71c1b3d56c1", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010506060212000,updatedAt_14010511013422000,userId_1208,username_mohammad.askari,updatedById_790,projectId_1797,projectPathWithNamespace_support/24-7,action_reopen,state_opened,issueId_246435,issueIid_5186,labels_Signal,changeType_closed_at,changeType_state_id,changeType_updated_at,dataId_946d61f4-3b1b-4cfb-b983-15c9c202bf71,groupName_Support,groupId_352,participantId_1208,participantId_811,participantId_790,participantId_145,participantId_1073,participantUsername_mohammad.askari,participantUsername_mahdi.madanchi,participantUsername_amin.ziaei,participantUsername_reza.mahmoudi,participantUsername_kosar.soltani,milestoneId_2971}");
    insertRecord("4da2f9d8-1748-4199-863d-00a0bf04a246", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010511053141000,updatedAt_14010511053141000,userId_1208,username_mohammad.askari,projectId_1797,projectPathWithNamespace_support/24-7,action_open,state_opened,issueId_247377,issueIid_5244,labels_Other,changeType_author_id,changeType_created_at,changeType_description,changeType_id,changeType_iid,changeType_milestone_id,changeType_project_id,changeType_title,changeType_updated_at,dataId_7a771623-9767-48ec-8c2b-21bb8c1f0234,groupName_Support,groupId_352,participantId_1208,participantId_145,participantId_768,participantUsername_mohammad.askari,participantUsername_reza.mahmoudi,participantUsername_saeed.khanehgir,milestoneId_2971}");
    insertRecord("10bb245a-0c9c-498e-b6ca-f1ede67419da", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010504085808000,updatedAt_14010511140139000,userId_1034,username_samangarbot,updatedById_1034,projectId_132,projectPathWithNamespace_data-layer/infraStructure,action_update,state_opened,assigneeId_mahmoud.rezaee,issueId_245839,issueIid_1475,labels_اولویت: بالا,labels_تخته: پایگاه داده,labels_نوع: پشتیبانی,labels_وضعیت: در حال انجام,changeType_total_time_spent,changeType_time_change,dataId_04942bf0-6d8a-4ce9-8308-656547c9e277,groupName_زیرساخت لایه داده,groupId_89,participantId_57,participantId_1034,participantUsername_mahmoud.rezaee,participantUsername_samangarbot,milestoneId_2986}");
    insertRecord("83514934-8216-448d-bc61-cc912c8d3f60", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010413174700000,updatedAt_14010511143741000,userId_1031,username_maryam.najafi,updatedById_1031,projectId_4382,projectPathWithNamespace_danabot/rahtak_ai_group/faq,action_update,state_closed,assigneeId_hamed.babaei,issueId_242073,issueIid_324,labels_مطالعه و بررسی,changeType_updated_at,changeType_updated_by_id,changeType_labels,dataId_c74c313b-68f5-4d44-b33d-1a8a6042ffda,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1034,participantUsername_hamed.babaei,participantUsername_samangarbot,milestoneId_2891}");
    insertRecord("6bf2c28e-42c9-493f-9099-3fb13d307440", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010428121114000,updatedAt_14010511123520000,userId_57,username_mahmoud.rezaee,updatedById_57,projectId_29,projectPathWithNamespace_merat/imerat,action_update,state_opened,assigneeId_mahmoud.rezaee,issueId_244427,issueIid_1775,labels_نسخه ۲.۲۹,labels_نوع: ویژگی,labels_وضعیت: در حال انجام,changeType_assignees,dataId_724cde6e-818d-4d47-9439-411d187720be,groupName_merat,groupId_56,participantId_57,participantId_5,participantId_62,participantId_42,participantId_936,participantId_49,participantUsername_mahmoud.rezaee,participantUsername_alireza.hokmabadi,participantUsername_saeed.shidkalaee,participantUsername_mohammadjavad.ahmadi,participantUsername_mojtaba.hosseinalinezhad,participantUsername_pouya.azarpour,milestoneId_2968}");
    insertRecord("cd0caeb3-7273-4e49-b956-31b3b4d96f37", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010303112342000,updatedAt_14010511145257000,userId_1031,username_maryam.najafi,updatedById_1031,projectId_4746,projectPathWithNamespace_danabot/rahtak_ai_group/nlp-backlog,action_update,state_opened,assigneeId_hamed.babaei,issueId_233392,issueIid_27,labels_نوع: بهبود,labels_وضعیت: در حال انجام,changeType_milestone_id,changeType_updated_at,changeType_updated_by_id,dataId_3fac0dc2-895c-4358-ae06-1cd065d0475b,groupName_DanaBot,groupName_Danabot_ai_group,groupId_5272,groupId_1903,participantId_976,participantId_1029,participantId_1034,participantId_1031,participantId_967,participantUsername_hamed.babaei,participantUsername_ali.rahmati,participantUsername_samangarbot,participantUsername_maryam.najafi,participantUsername_marzieh.zargari,mentioner_#44,milestoneId_3004}");
    insertRecord("273d5f42-53e2-47cc-b71c-bea9fb694c50", "{source_gitlab,type_issue,year_1401,yearMonth_140105,yearMonthDay_14010511,createdAt_14010511112425000,updatedAt_14010511112433000,userId_55,username_kousha.foroughi,updatedById_55,projectId_702,projectPathWithNamespace_signal/infrastructure/modules/partServiceSignalData,action_update,state_opened,issueId_247476,issueIid_424,labels_تخته: اندروید,labels_تخته: زیر ساخت,labels_نوع: بهبود,labels_وضعیت: آماده انجام,changeType_milestone_id,changeType_updated_at,changeType_updated_by_id,dataId_57577658-adb9-407b-8b7c-38e3f84cfc2c,groupName_Infrastructure,groupName_Modules,groupName_signal,groupId_1581,groupId_398,groupId_85,participantId_55,participantUsername_kousha.foroughi,milestoneId_2972}");
}

