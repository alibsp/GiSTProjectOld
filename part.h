#ifndef PART_H
#define PART_H

#include <QObject>

class Part : public QObject
{
    Q_OBJECT
public:
    explicit Part(QObject *parent = nullptr);

signals:

};

#endif // PART_H
