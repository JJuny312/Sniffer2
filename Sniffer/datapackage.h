#ifndef DATAPACKAGE_H
#define DATAPACKAGE_H

#include "Format.h"
#include <QString>

class DataPackage
{
private:
    u_int data_length;
    QString timeStamp;
    QString info;
    int package_type;
protected:
    static QString ByteToString(char *str, int size);
public:
    const u_char *pkt_content;
public:
    DataPackage();
    void setDataLength(u_int data_length);
    void setTimeStamp(QString timeStamp);
    void setPackageType(int type);
    void setPointer(const u_char *pkt_content, int size);
    void setInfo(QString info);

    QString getDataLength();
    QString getTimeStamp();
    QString getPackageType();
    QString getInfo();
};

#endif // DATAPACKAGE_H
