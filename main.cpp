#include <QCoreApplication>
#include "GiST/gist.h"
#include "BTree/gist_btree.h"
#include "GiST/gist_cursor.h"

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


void intTest()
{
    gist myGist;
    //ایجاد پایگاه داده با افزونه BTree
    myGist.create("Data.db", &bt_int_ext);
    int key1 = 5;
    int data1 = 1;
    int key2 = 3;
    int data2 = 2;
    int key3 = 10;
    int data3 = 3;
    myGist.insert((void *) &key1, sizeof(int), (void *) &data1, sizeof(int));
    myGist.insert((void *) &key2, sizeof(int), (void *) &data2, sizeof(int));
    myGist.insert((void *) &key3, sizeof(int), (void *) &data3, sizeof(int));
    bt_query_t q(bt_query_t::bt_betw, new int(4), new int(100));
    gist_cursor_t cursor;
    myGist.fetch_init(cursor, &q);
    bool eof = false;
    int key, data;
    smsize_t keysz=2, datasz=2;
    while (!eof)
    {
        (void) myGist.fetch(cursor, &key, keysz, &data, datasz, eof);
        if (!eof)
            std::cout<<key<<"->"<<data<<endl;
        // process key and data...
    }
}

void stringTest()
{
    gist myGist;
    //ایجاد پایگاه داده با افزونه BTree
    myGist.create("Data.db", &bt_str_ext);
    char key1[10] = "Ali";
    int data1 = 1;
    char key2[10] = "Hasan";
    int data2 = 2;
    char key3[10] = "Amir";
    int data3 = 3;
    myGist.insert((void *) &key1, 10, (void *) &data1, sizeof(int));
    myGist.insert((void *) &key2, 10, (void *) &data2, sizeof(int));
    myGist.insert((void *) &key3, 10, (void *) &data3, sizeof(int));

    bt_query_t q(bt_query_t::bt_betw, key1, key3);
    gist_cursor_t cursor;
    myGist.fetch_init(cursor, &q);
    bool eof = false;
    char* key = "----------";
    int data;
    smsize_t keysz, datasz;
    while (!eof)
    {
        (void) myGist.fetch(cursor, key, keysz, &data, datasz, eof);
        if (!eof)
            std::cout<<key<<"->"<<data<<endl;
        // process key and data...
    }
}
int main(int argc, char *argv[])
{
    //intTest();
    stringTest();
}
