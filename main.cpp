#include <QCoreApplication>
#include "GiST/gist.h"
#include "GiST/gist_compat.h"



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    gist aa;
    std::cout<<(1U << ((8 * (int) sizeof(int)) - 1))<<" "<< HIBITL << " "<<MAXINT<<" "<<8*sizeof(int)<<std::endl;
    //return a.exec();
}
