#include "multhread.h"
#include <QDebug>
#include "Format.h"
#include "datapackage.h"

multhread::multhread()
{
    this->isDone = true;
}

bool multhread::setPointer(pcap_t *pointer){
    this->pointer = pointer;
    if(pointer)
        return true;
    else return false;
}

void multhread::setFlag(){
    this->isDone = false;
}

void multhread::resetFlag(){
    this->isDone = true;
}

void multhread::run(){
    while(true){
        if(isDone)
            break;
        else{
            int res = pcap_next_ex(pointer, &header, &pkt_data);
            if(res == 0)
                continue;
            local_time_sec = header->ts.tv_sec;
            localtime_s(&local_time, &local_time_sec);
            strftime(timeString, sizeof(timeString), "%H:%M:%S", &local_time);
            //qDebug()<<timeString;
            QString info = "";
            int type = ethernetPackageHandle(pkt_data, info);
            if(type){
                DataPackage data;
                int len = header->len;
                data.setInfo(info);
                data.setDataLength(len);
                data.setTimeStamp(timeString);
                data.setPackageType(type);
                data.setPointer(pkt_data, len);
                emit send(data);
            }
        }
    }
}

int multhread::ethernetPackageHandle(const u_char *pkt_content, QString &info){
    ETHER_HEADER *ethenet;
    u_short content_type;
    ethenet = (ETHER_HEADER*)(pkt_content);
    content_type = ntohs(ethenet->type);
    switch(content_type){
    case 0x0800:{ // ip
        int ipPackage = 0;
        int res = ipPackageHandle(pkt_content, ipPackage);
        switch(res){
        case 1:{ // icmp
            info = icmpPackageHandle(pkt_content);
            return 2;
        }
        case 6:{ // tcp
            return tcpPackageHandle(pkt_content, info, ipPackage);
        }
        case 17:{ // udp
            return udpPackageHandle(pkt_content, info);
        }
        default: break;
        }
        break;
    }
    case 0x0806:{ // arp
        info = arpPackageHandle(pkt_content);
        return 1;
    }
    default: break;
    }
    return 0;
}

int multhread::ipPackageHandle(const u_char *pkt_content, int &ipPackage){
    IP_HEADER *ip;
    ip = (IP_HEADER*)(pkt_content + 14);
    int protocol = ip->protocol;
    ipPackage = (ntohs(ip->total_length) - ((ip->version_length)& 0x0F) * 4);
    return protocol;
}

int multhread::tcpPackageHandle(const u_char *pkt_content, QString &info, int ipPackage){
    TCP_HEADER *tcp;
    tcp = (TCP_HEADER*)(pkt_content + 14 + 20);
    u_short src = ntohs(tcp->src_port);
    u_short des = ntohs(tcp->des_port);

    QString proSend = "";
    QString proRecv = "";

    int type = 3;
    int delta = (tcp->header_length >> 4) * 4;
    int tcpLoader = ipPackage - delta;

    if((src == 443 || des == 443) && (tcpLoader > 0)){
        if(src == 443)
            proSend = "(https)";
        else proRecv = "(https)";
        u_char *ssl;
        ssl = (u_char*)(pkt_content + 14 + 20 + delta);
        u_char isTls = (*ssl);
        ssl++;
        u_short *pointer = (u_short*)ssl;
        u_short version = ntohs(*pointer);
        if(isTls >= 20 && isTls <= 23 && version >= 0x0301 && version <= 0x0304){
            type = 6;
            switch(isTls){
            case 20:{
                info = "Change Cipher Spec ";
                break;
            }
            case 21:{
                info = "Alert ";
                break;
            }
            case 22:{
                info = "HandShake ";
                ssl += 4;
                u_char type = (*ssl);
                switch(type){
                case 1:{
                    info += " Client Hello ";
                    break;
                }
                case 2:{
                    info += " Server Hello ";
                    break;
                }
                case 4: {
                    info += " New Session Ticket ";
                    break;
                }
                case 11:{
                    info += " Certificate ";
                    break;
                }
                case 16:{
                    info += " Client Key Exchange ";
                    break;
                }
                case 12:{
                    info += " Server Key Exchange ";
                    break;
                }
                case 14:{
                    info += " Server Hello Done ";
                    break;
                }
                default:break;
                }
            }
            case 23:{
                    info = "Application Data ";
                    break;
                }
            default:break;
            }
        }else type = 7;
    }
    if(type == 7)
        info = " Continuation Data ";
    else{
        info += QString::number(src) + proSend + "->" + QString::number(des) + proRecv;
        QString flag = "";
        if(tcp->flags & 0x08) flag += "PSH";
        if(tcp->flags & 0x10) flag += "ACK";
        if(tcp->flags & 0x02) flag += "SYN";
        if(tcp->flags & 0x20) flag += "URG";
        if(tcp->flags & 0x01) flag += "FIN";
        if(tcp->flags & 0x04) flag += "RST";
        if(flag != ""){
            flag = flag.left(flag.length() - 1);
            info += "[" + flag + "]";
        }

        u_int sequence = ntohl(tcp->sequence);
        u_int ack = ntohl(tcp->ack);
        u_short window = ntohs(tcp->window_size);

        info += " Seq=" + QString::number(sequence) + " Ack=" + QString::number(ack) + " win=" + QString::number(window) + " len=" + QString::number(tcpLoader);

    }
    return type;
}

int multhread::udpPackageHandle(const u_char *pkt_content, QString &info){
    UDP_HEADER *udp;
    udp = (UDP_HEADER*)(pkt_content + 14 + 20);
    u_short des = ntohs(udp->des_port);
    u_short src = ntohs(udp->src_port);
    if(des == 53 || src == 53){
        info = dnsPackageHandle(pkt_content);
        return 5;
    }
    else {
        QString res = QString::number(src) + "->" + QString::number(des);
        u_short data_len = ntohs(udp->data_length);
        res += "len=" + QString::number(data_len);
        info = res;
        return 4;
    }
}

QString multhread::arpPackageHandle(const u_char *pkt_content){
    ARP_HEADER *arp;
    arp = (ARP_HEADER*)(pkt_content + 14);

    u_short op = ntohs(arp->op_code);
    QString res = "";

    u_char *des_addr = arp->des_ip_addr;
    QString desIp = QString::number(*des_addr) + "."
            + QString::number(*(des_addr + 1)) + "."
            + QString::number(*(des_addr + 2)) + "."
            + QString::number(*(des_addr + 3));

    u_char *src_ip = arp->src_ip_addr;
    QString srcIp = QString::number(*src_ip) + "."
            + QString::number(*(src_ip + 1)) + "."
            + QString::number(*(src_ip + 2)) + "."
            + QString::number(*(src_ip + 3));

    u_char *src_eth_addr = arp->src_eth_addr;
    QString srcEth = ByteToString(src_eth_addr, 1) + ":"
            + ByteToString((src_eth_addr + 1), 1) + ":"
            + ByteToString((src_eth_addr + 2), 1) + ":"
            + ByteToString((src_eth_addr + 3), 1) + ":"
            + ByteToString((src_eth_addr + 4), 1) + ":"
            + ByteToString((src_eth_addr + 5), 1);

    if(op == 1){
        res = "Who has " + desIp + "? Tell " + srcIp;
    }
    else if(op == 2){
        res = srcIp + " is at " + srcEth;
    }
    return res;

}

QString multhread::ByteToString(u_char *str, int size){
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

QString multhread::dnsPackageHandle(const u_char *pkt_content){
    DNS_HEADER *dns;
    dns = (DNS_HEADER*)(pkt_content + 14 + 20 + 8);
    u_short identification = ntohs(dns->identification);
    u_short type = dns->flags;
    QString info = "";
    if((type & 0xf800) == 0x0000){
        info = "Standard query";
    }else if((type & 0xf800) == 0x8000){
        info = "Standard query response";
    }
    QString name = "";
    char *domain = (char*)(pkt_content + 14 + 20 + 8 + 12);
    while(domain != 0x00){
        if(domain && (*domain) <= 64){
            int length = *domain;
            domain++;
            for(int k = 0; k < length; k++){
                name += (*domain);
                domain++;
            }
            name += ".";
        }else break;
    }
    if(name != "")
        name = name.left(name.length() - 1);
    return info + " 0x" + QString::number(identification, 16) + " " + name;
}

QString multhread::icmpPackageHandle(const u_char *pkt_content){
    ICMP_HEADER *icmp;
    icmp = (ICMP_HEADER*)(pkt_content + 14 + 20);
    u_char type = icmp->type;
    u_char code = icmp->code;
    QString res = "";
    switch(type){
    case 0:{
        if(!code)
            res = "Echo response(ping)";
        break;
    }
    case 3:{
        switch (code) {
        case 0:{
            res = "Network unreachable";
            break;
        }
        case 1:{
            res = "Host unreachable";
            break;
        }
        case 2:{
            res = "Protocol unreachable";
            break;
        }
        case 3:{
            res = "Port unreachable";
            break;
        }
        case 4:{
            res = "Fragmentation is required, but DF is set";
            break;
        }
        case 5:{
            res = "Source route selection failed";
            break;
        }
        case 6:{
            res = "Unknown target network";
            break;
        }
        default:break;
        }
        break;
    }
    case 4:{
        res = "Source station suppression [congestion control]";
        break;
    }
    case 5:{
        res = "Relocation";
        break;
    }
    case 8:{
        if(!code)
            res = "Echo request(ping)";
        break;
    }
    default:break;
    }
    return res;
}
