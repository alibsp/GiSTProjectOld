#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDebug>
#include "GiST/gist.h"
#include "BTree/gist_btree.h"
#include "GiST/gist_cursor.h"
#include "part.h"

void myPrintPredFct(
        std::ostream& s, // what to print to
        const vec_t& pred, // pred.ptr(0) contains pointer to 8-byte aligned predicate
        int level)
{

}

void myPrintDataFct(std::ostream& s, // what to print to
                    const vec_t& data)
{

}


int intTest()
{
    gist myGist;
    //ایجاد پایگاه داده با افزونه BTree
    if(myGist.create("DataInt.db", &bt_int_ext)==RCOK)
    {

        //تولید اعداد تصادفی برای درج در پایگاه داده
        srand(time(0));
        int lb = -10000000, ub = 10000000;
        for (int i = 0; i < 1000; i++)
        {
            for (int j = 0; j < 10000; j++)
            {
                int key = (rand() % (ub - lb + 1)) + lb;
                int data = (rand() % (ub - lb + 1)) + lb;
                myGist.insert((void *) &key, sizeof(int), (void *) &data, sizeof(int));
            }
            myGist.flush();
        }
    }
    else // File maybe Exist
    {
        if(myGist.open("DataInt.db")!=RCOK)
        {
            cerr << "Can't Open File." << endl;
            return 0;
        }
    }

    bt_query_t q(bt_query_t::bt_betw, new int(1200), new int(1500));
    gist_cursor_t cursor;

    if(myGist.fetch_init(cursor, &q)!=RCOK)
    {
        cerr << "Can't initialize cursor." << endl;
        return(eERROR);
    }

    bool eof = false;
    int key, data;
    smsize_t keysz=sizeof(int), datasz=sizeof(int);
    while (!eof)
    {
        if(myGist.fetch(cursor, (void *)&key, keysz, (void *)&data, datasz, eof)!=RCOK)
        {
            cerr << "Can't fetch from cursor." << endl;
            return(eERROR);
        }
        if (!eof)
        {
            std::cout<<key<<"->"<<data<<endl;
        }
        // process key and data...
    }
    return 1;
}

static char *rand_string(char *str, size_t size)
{
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-";

    if (size)
    {
        --size;
        for (size_t n = 0; n < size; n++)
        {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

int stringTest()
{
    gist myGist;
    //ایجاد پایگاه داده با افزونه BTree
    if(myGist.create("DataString.db", &bt_str_ext)==RCOK)
    {

        //تولید رشته های تصادفی برای درج در پایگاه داده
        srand(time(0));
        int lb = 3, ub = 100;

        for (int i = 0, k=1; i < 10000; i++)
        {
            for (int j = 0; j < 1000; j++)
            {
                char key[100]={0};
                int len=(rand() % (ub - lb + 1)) + lb;
                rand_string(key, len);
                int data = k++;
                myGist.insert((void *) &key, 100, (void *) &data, sizeof(int));
            }
            //12532000 + 131000 + 118000 + 166000 + 1000 + 100000
            myGist.flush();
            cout << "Insert "<<k-1<< " Record ok" << endl;
        }
    }
    else // File maybe Exist
    {
        if(myGist.open("DataString.db")!=RCOK)
        {
            cerr << "Can't Open File." << endl;
            return 0;
        }
    }
    //char my_key[100]="Ali Aldaghi";
//    int my_data = 1000;
//    myGist.insert((void *) &my_key, 100, (void *) &my_data, sizeof(int));

    //bt_query_t qRemove(bt_query_t::bt_eq, my_key, my_key);

    //myGist.remove(&qRemove);

    char key1[100] = "Ali";
    char key2[100] = "Alizzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";

    QElapsedTimer timer;
    timer.start();

    bt_query_t q(bt_query_t::bt_betw, key1, key2);
    //bt_query_t q(bt_query_t::bt_eq, &key1, key2);
    gist_cursor_t cursor;
    if(myGist.fetch_init(cursor, &q)!=RCOK)
    {
        cerr << "Can't initialize cursor." << endl;
        return(eERROR);
    }
    bool eof = false;

    int data;
    smsize_t keysz=100, datasz=sizeof(int);
    char key[100]={0};
    while (!eof)
    {
        if(myGist.fetch(cursor, (void *)&key, keysz, (void *)&data, datasz, eof)!=RCOK)
        {
            cerr << "Can't fetch from cursor." << endl;
            return(eERROR);
        }
        if (!eof)
            cout<<(char*)&key<<"->"<<data<<endl;
        // process key and data...
    }
    cout<<endl<<"Time is:"<<timer.elapsed()<<endl;
    return 0;
}

int main(int argc, char *argv[])
{
    //intTest();
    //stringTest();
    qDebug()<<"start...";
    QString csvFile ;//= "/home/shahabseddigh/Desktop/GiSTProject/csv/minimal.csv"; //shahab
#ifdef __linux__
    csvFile = "/media/ali/Data/Programming/Projects/Part/Data/test1.csv";
#elif _WIN32
    csvFile = "D:\\Programming\\Projects\\Part\\Data\\data.csv";
#endif

    bool clean=true;
    Part part;
    if(clean)
    {
        part.dropGists();
        part.importCSV(csvFile);
    }
    else
        part.loadGists();

    QElapsedTimer timer;
    timer.start();

    QStringList results=part.findKey("milestoneId_2970");
    //results.append(part.findKey("updatedAt_14010510200905000"));
    //results.append(part.findKey("createdAt_14010511150209000"));

    uint64_t time1=timer.nsecsElapsed();
    QSet<QString> ids(results.begin(), results.end());
    uint64_t time=timer.nsecsElapsed();
    for (QString res:ids)
        qDebug()<<res;
    qDebug()<<"Execute Time: "<<time1<<time<<" ns, record count:"<<ids.count();
    qDebug()<<"finish.";
}
