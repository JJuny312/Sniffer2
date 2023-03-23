#include "datapackage.h"
#include <QMetaType>
#include <winsock2.h>

DataPackage::DataPackage()
{
    qRegisterMetaType<DataPackage>("DataPackage");
    this->timeStamp = "";
    this->data_length = 0;
    this->package_type = 0;
}

void DataPackage::setInfo(QString info){
    this->info = info;
}

void DataPackage::setPointer(const u_char *pkt_content, int size){
    this->pkt_content = (u_char*)malloc(size);
    memcpy((char*)(this->pkt_content), pkt_content, size);
}

void DataPackage::setTimeStamp(QString timeStamp){
    this->timeStamp = timeStamp;
}

void DataPackage::setDataLength(u_int data_length){
    this->data_length = data_length;
}

QString DataPackage::getInfo(){
    return this->info;
}

QString DataPackage::getTimeStamp(){
    return this->timeStamp;
}

QString DataPackage::getDataLength(){
    return QString::number(this->data_length);
}

QString DataPackage::getPackageType(){
    switch (this->package_type){
    case 1: return "ARP";
    case 2: return "ICMP";
    case 3: return "TCP";
    case 4: return "UDP";
    case 5: return "DNS";
    case 6: return "TLS";
    case 7: return "SSL";
    default: return "";
    }
}

QString DataPackage::ByteToString(u_char *str, int size){
    QString res = "";
    for(int i = 0; i < size; i++){
        char one = str[i] >> 4;
        if(one >= 0x0A)
            one += 0x41 - 0x0A;
        else one += 0x30;
        char two = str[i] & 0xF;
        if(two >= 0x0A)
            two += 0x41 - 0x0A;
        else two += 0x30;
        res.append(one);
        res.append(two);
    }
    return res;
}

void DataPackage::setPackageType(int type){
    this->package_type = type;
}

// MAC
QString DataPackage::getDesMacAddr(){
    ETHER_HEADER *eth;
    eth = (ETHER_HEADER*)(pkt_content);
    u_char *addr = eth->ethernet_des_host;
    if(addr){
        QString res = ByteToString(addr, 1) + ":"
                + ByteToString((addr + 1), 1) + ":"
                + ByteToString((addr + 2), 1) + ":"
                + ByteToString((addr + 3), 1) + ":"
                + ByteToString((addr + 4), 1) + ":"
                + ByteToString((addr + 5), 1);
        if(res == "FF:FF:FF:FF:FF:FF") return "FF:FF:FF:FF:FF:FF(Broadcast)";
        else return res;
    }
}

QString DataPackage::getSrcMacAddr(){
    ETHER_HEADER *eth;
    eth = (ETHER_HEADER*)(pkt_content);
    u_char *addr = eth->ethernet_src_host;
    if(addr){
        QString res = ByteToString(addr, 1) + ":"
                + ByteToString((addr + 1), 1) + ":"
                + ByteToString((addr + 2), 1) + ":"
                + ByteToString((addr + 3), 1) + ":"
                + ByteToString((addr + 4), 1) + ":"
                + ByteToString((addr + 5), 1);
        if(res == "FF:FF:FF:FF:FF:FF") return "FF:FF:FF:FF:FF:FF(Broadcast)";
        else return res;
    }
}

// IP
QString DataPackage::getDesIpAddr(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    sockaddr_in desAddr;
    desAddr.sin_addr.s_addr = ip->des_address;
    return QString(inet_ntoa(desAddr.sin_addr));
}

QString DataPackage::getSrcIpAddr(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    sockaddr_in srcAddr;
    srcAddr.sin_addr.s_addr = ip->src_address;
    return QString(inet_ntoa(srcAddr.sin_addr));
}

QString DataPackage::getSource(){
    if(this->package_type == 1){
        return this->getSrcMacAddr();
    }
    else return this->getSrcIpAddr();
}

QString DataPackage::getDestination(){
    if(this->package_type == 1){
        return this->getDesMacAddr();
    }
    else return this->getDesIpAddr();
}

QString DataPackage::getMacType(){
    ETHER_HEADER *eth;
    eth = (ETHER_HEADER*)(pkt_content);
    u_short type = ntohs(eth->type);
    if(type = 0x0800) return "IPv4(0x0800)";
    else if(type == 0x806) return "ARP(0x0806)";
    else return "";
}

QString DataPackage::getIpVersion(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number(ip->version_length >> 4);
}

QString DataPackage::getIpHeaderLength(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    QString res = "";
    int length = ip->version_length & 0x0F;
    if(length == 5) res = "20 bytes (5)";
    else res = QString::number(length*5) + "bytes (" + QString::number(length) + ")";
    return res;
}

QString DataPackage::getIpTos(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number(ntohs(ip->TOS));
}

QString DataPackage::getIpTotalLength(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number(ntohs(ip->total_length));
}

QString DataPackage::getIpIdentification(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number(ntohs(ip->identification),16);
}

QString DataPackage::getIpFlag(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number((ntohs(ip->offset)& 0xe000) >> 8,16);
}

QString DataPackage::getIpReservedBit(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    int bit = (ntohs(ip->offset) & 0x8000) >> 15;
    return QString::number(bit);
}

QString DataPackage::getIpDF(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number((ntohs(ip->offset) & 0x4000) >> 14);
}

QString DataPackage::getIpMF(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number((ntohs(ip->offset) & 0x2000) >> 13);
}

QString DataPackage::getIpFragmentOffset(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number(ntohs(ip->offset) & 0x1FFF);
}

QString DataPackage::getIpTTL(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number(ip->ttl);
}

QString DataPackage::getIpProtocol(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    int protocol = ip->protocol;
    switch (protocol) {
    case 1:return "ICMP (1)";
    case 6:return "TCP (6)";
    case 17:return "UDP (17)";
    default:{
        return "";
    }
    }
}

QString DataPackage::getIpCheckSum(){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    return QString::number(ntohs(ip->checksum),16);
}

// ICMP
QString DataPackage::getIcmpType(){
    ICMP_HEADER *icmp;
    icmp = (ICMP_HEADER*)(pkt_content + 14 + 20);
    return QString::number(ntohs(icmp->type));
}

QString DataPackage::getIcmpCode(){
    ICMP_HEADER *icmp;
    icmp = (ICMP_HEADER*)(pkt_content + 14 + 20);
    return QString::number(ntohs(icmp->code));

}

QString DataPackage::getIcmpCheckSum(){
    ICMP_HEADER *icmp;
    icmp = (ICMP_HEADER*)(pkt_content + 14 + 20);
    return QString::number(ntohs(icmp->checksum),16);
}

QString DataPackage::getIcmpIdentification(){
    ICMP_HEADER *icmp;
    icmp = (ICMP_HEADER*)(pkt_content + 14 + 20);
    return QString::number(ntohs(icmp->identification));
}

QString DataPackage::getIcmpSequeue(){
    ICMP_HEADER *icmp;
    icmp = (ICMP_HEADER*)(pkt_content + 14 + 20);
    return QString::number(ntohs(icmp->sequence));
}

QString DataPackage::getIcmpData(int size){
    char *icmp;
    icmp = (char*)(pkt_content + 14 + 20 + 8);
    QString res= "";
    for(int i = 0; i < size; i++){
        res += (*icmp);
        icmp++;
    }
    return res;
}

// ARP
QString DataPackage::getArpHardwareType(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    int type = ntohs(arp->type);
    QString res = "";
    if(type == 0x0001) res = "Ethernet(1)";
    else res = QString::number(type);
    return res;
}

QString DataPackage::getArpProtocolType(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    int type = ntohs(arp->protocol);
    QString res = "";
    if(type == 0x0800) res = "IPv4(0x0800)";
    else res = QString::number(type);
    return res;
}

QString DataPackage::getArpHardwareLength(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    return QString::number(arp->mac_length);
}

QString DataPackage::getArpProtocolLength(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    return QString::number(arp->ip_length);
}

QString DataPackage::getArpOperationCode(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    int code = ntohs(arp->op_code);
    QString res = "";
    if(code == 1) res  = "request(1)";
    else if(code == 2) res = "reply(2)";
    return res;
}

QString DataPackage::getArpSourceEtherAddr(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    u_char*addr;
    if(arp){
        addr = arp->src_eth_addr;
        if(addr){
            QString res = ByteToString(addr, 1) + ":"
                    + ByteToString((addr + 1), 1) + ":"
                    + ByteToString((addr + 2), 1) + ":"
                    + ByteToString((addr + 3), 1) + ":"
                    + ByteToString((addr + 4), 1) + ":"
                    + ByteToString((addr + 5), 1);
            return res;
        }
    }
    return "";
}

QString DataPackage::getArpDestinationEtherAddr(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    u_char *addr;
    if(arp){
        addr = arp->des_eth_addr;
        if(addr){
            QString res = ByteToString(addr, 1) + ":"
                    + ByteToString((addr + 1), 1) + ":"
                    + ByteToString((addr + 2), 1) + ":"
                    + ByteToString((addr + 3), 1) + ":"
                    + ByteToString((addr + 4), 1) + ":"
                    + ByteToString((addr + 5), 1);
            return res;
        }
    }
    return "";
}

QString DataPackage::getArpSourceIpAddr(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    if(arp){
        u_char *addr = arp->src_ip_addr;
        QString srcIp = QString::number(*addr) + "."
                + QString::number(*(addr + 1)) + "."
                + QString::number(*(addr + 2)) + "."
                + QString::number(*(addr + 3));
        return srcIp;
    }
    return "";
}

QString DataPackage::getArpDestinationIpAddr(){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);
    if(arp){
        u_char *addr = arp->des_ip_addr;
        QString desIp = QString::number(*addr) + "."
                + QString::number(*(addr + 1)) + "."
                + QString::number(*(addr + 2)) + "."
                + QString::number(*(addr + 3));
        return desIp;
    }
    return "";
}














