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

void myPrintDataFct(
            std::ostream& s, // what to print to
            const vec_t& data)
{

}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    gist myGist;
    myGist.create("Data.db", &bt_int_ext);
    int key1 = 5;
    int data1 = 1;
    int key2 = 3;
    int data2 = 2;
    (void) myGist.insert((void *) &key1, sizeof(int), (void *) &data1, sizeof(int));
    (void) myGist.insert((void *) &key2, sizeof(int), (void *) &data2, sizeof(int));
    bt_query_t q(bt_query_t::bt_betw, new int(1), new int(6));
    gist_cursor_t cursor;
    myGist.fetch_init(cursor, &q);
    bool eof = false;
    int key, data;
    smsize_t keysz, datasz;
    while (!eof)
    {
        (void) myGist.fetch(cursor, (void *)&key, keysz, (void *)&data, datasz, eof);
        if (!eof)
            std::cout<<key<<"->"<<data<<endl;
            // process key and data...
    }
}
