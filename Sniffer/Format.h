#ifndef FORMAT_H
#define FORMAT_H

typedef unsigned char u_char;//1
typedef unsigned short u_short;//2
typedef unsigned int u_int;//4
typedef unsigned long u_long;//4

typedef struct ether_header{ // MAC
    u_char ethernet_des_host[6];
    u_char ethernet_src_host[6];
    u_short type;
}ETHER_HEADER;

typedef struct ip_header{ // IPv4
    u_char version_length;
    u_char TOS;
    u_short total_length;
    u_short identification;
    u_short offset;
    u_char ttl;
    u_char protocol;
    u_short checksum;
    u_int src_address;
    u_int des_address;
}IP_HEADER;

typedef struct tcp_header{ // TCP
    u_short src_port;
    u_short des_port;
    u_int sequence;
    u_int ack;
    u_char header_length;
    u_char flags;
    u_short window_size;
    u_short checksum;
    u_short urgent;
}TCP_HEADER;

typedef struct udp_header{ // UDP
    u_short src_port;
    u_short des_port;
    u_short data_length;
    u_short checksum;
}UDP_HEADER;

typedef struct arp_header{ // ARP
    u_short type;
    u_short protocol;
    u_char mac_length;
    u_char ip_length;
    u_short op_code;
    u_char src_eth_addr[6];
    u_char src_ip_addr[4];
    u_char des_eth_addr[6];
    u_char des_ip_addr[4];
}ARP_HEADER;

typedef struct icmp_header{ // ICMP
    u_char type;
    u_char code;
    u_short checksum;
    u_short identification;
    u_short sequence;

}ICMP_HEADER;

typedef struct dns_header{  // DNS
    u_short identification;
    u_short flags;
    u_short question;
    u_short answer;
    u_short authority;
    u_short additional;
}DNS_HEADER;

typedef struct dns_question{ // DNS请求
    // char* name;
    u_short query_type;
    u_short query_class;
}DNS_QUESITON;

typedef struct dns_answer{ // DNS响应
    // char* name
    u_short answer_type;
    u_short answer_class;
    u_int TTL;
    u_short dataLength;
    //char* name
}DNS_ANSWER;

#endif // FORMAT_H
