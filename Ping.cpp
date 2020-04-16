#include <cassert>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <sys/types.h> 
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#define PORT 80
#define SIZE_INT sizeof(int)
using namespace std;
//http://man7.org/linux/man-pages/man7/icmp.7.html
//sudo sysctl -w net.ipv4.ping_group_range="0 65535"
//-f: flood
//
void ping_addr(char* ip_addr){
    struct sockaddr_in sock_addr;
    struct icmphdr icmp;
    char send_buf[sizeof(icmp) + SIZE_INT];
    char recieve_buf[sizeof(icmp) + SIZE_INT];
    int amt_sent = 0;
    int amt_recieved = 0;
    int sock;
    int pton_result;

    memset(&icmp, 0, sizeof(icmp));
    memset(&sock_addr, 0, sizeof(sock_addr));

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP); //CHANGE TO IPPRONT_ICMPV6 for ipv6
    //Use SOCK_DGRAM for UDP to determine quality of network
    if(sock < 0) { 
        cout<<"Error: "<<errno<<endl;
        assert(false);
    }

    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_family = AF_INET; //CHANGE TO AF_INET6 for ipv6

    pton_result = inet_pton(AF_INET, ip_addr, &sock_addr.sin_addr);//CHANGE TO AF_INET6 for ipv6
    assert(pton_result != 0);
    if(pton_result<0){
        cout<<"Error: "<<errno<<endl;
        assert(false);
    }

    icmp.type = ICMP_ECHO;

    memcpy(send_buf, &icmp, sizeof(icmp));
    /*
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); //CODE TO SET TIEMOUTS
    */
    while(true){
        struct icmphdr recieved;
        int recieved_num = 0;
        socklen_t trash = 0;
        int rec;
        int send_res;

        ++amt_sent;
        memcpy(send_buf+sizeof(icmp), &amt_sent, SIZE_INT);

        //Send packet with icmp struct and amt_sent 
        send_res = sendto(sock, send_buf, sizeof(icmphdr) + SIZE_INT,0, (struct sockaddr*)&sock_addr,sizeof(sock_addr));
        if(send_res<0){
            cout<<"Error: "<<errno<<endl; assert(false);
        }
        
        rec = recvfrom(sock, recieve_buf, sizeof(recieve_buf), 0, NULL, &trash);
        if(rec<0){
            cout<<"Error: "<<errno<<endl;
            if(errno == EAGAIN){
                cout<<"Packet Lost!";
            }
            else{assert(false);}
        }
        else{
            memcpy(&recieved, recieve_buf, sizeof(recieved));
            assert(recieved.type == ICMP_ECHOREPLY);        //validate that packet is correct
            memcpy(&recieved_num, &recieve_buf[sizeof(icmphdr)],4);
            cout<<"REPLY: "<<recieved_num<<endl;
        }
    }
}

void ping_ipv6(char* ip6_addr){
    //struct ip6_hdr ip6;
    struct sockaddr_in6 sock_addr;
    struct icmp6_hdr icmp;
    char send_buf[sizeof(icmp) + SIZE_INT];
    char recieve_buf[sizeof(icmp) + SIZE_INT];
    int amt_sent = 0;
    int amt_recieved = 0;
    int sock;
    int pton_result;

    memset(&icmp, 0, sizeof(icmp));
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);//IPPROTO_ICMPV6); //CHANGE TO IPPRONT_ICMPV6 for ipv6
    //Use SOCK_DGRAM for UDP to determine quality of network
    if(sock < 0) { 
        cout<<"Error: "<<errno<<endl;
        assert(false);
    }

    sock_addr.sin6_port = htons(PORT);
    sock_addr.sin6_family = AF_INET6; //CHANGE TO AF_INET6 for ipv6

    pton_result = inet_pton(AF_INET6, ip6_addr, &sock_addr.sin6_addr);//CHANGE TO AF_INET6 for ipv6
    assert(pton_result != 0);
    if(pton_result<0){
        cout<<"Error: "<<errno<<endl;
        assert(false);
    }

    icmp.icmp6_type = ICMP6_ECHO_REQUEST;

    memcpy(send_buf, &icmp, sizeof(icmp));
    cout<<sizeof(icmp)<<" : "<<sizeof(int)<<endl;
    
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); //CODE TO SET TIEMOUTS
    
    while(true){
        struct icmphdr recieved;
        int recieved_num = 0;
        socklen_t trash = 0;
        int rec;
        int send_res;

        ++amt_sent;
        memcpy(send_buf+sizeof(icmp), &amt_sent, SIZE_INT);

        //Send packet with icmp struct and amt_sent 
        send_res = sendto(sock, send_buf, sizeof(icmp6_hdr) + SIZE_INT,0, (struct sockaddr*)&sock_addr,sizeof(sock_addr));
        if(send_res<0){
            cout<<"Error: "<<errno<<endl; assert(false);
        }
        cout<<"Sent and waititng"<<endl;
        rec = recvfrom(sock, recieve_buf, sizeof(recieve_buf), 0, NULL, &trash);
        if(rec<0){
            cout<<"Error: "<<errno<<endl;
            if(errno == EAGAIN){
                cout<<"Packet Lost!";
            }
            else{assert(false);}
        }
        else{
            memcpy(&recieved, recieve_buf, sizeof(recieved));
            assert(recieved.type == ICMP6_ECHO_REPLY);        //validate that packet is correct
            memcpy(&recieved_num, &recieve_buf[sizeof(icmphdr)],4);
            cout<<"REPLY: "<<recieved_num<<endl;
        }
    }
}

int main(int argc, char *argv[]){
    struct addrinfo address;
    struct addrinfo * result, * p;
    void *h;
    struct in6_addr help;
    struct hostent *help2electricboogaloo;
    memset(&address, 0, sizeof(addrinfo));
    memset(&result, 0, sizeof(addrinfo));

    address.ai_family = AF_UNSPEC;
    if(getaddrinfo(argv[1], NULL, &address, &result)){
        assert(false);
    }
    char* addr = (char*) malloc(1000);
    strcpy(addr, argv[1]);
    if(inet_pton(AF_INET, addr, &help)> 0){//Valid ipv4 addresss
        cout<<"HERE"<<endl;
        ping_addr(argv[1]);
    } 
    if(inet_pton(AF_INET6, addr, &help)> 0){//Valid ipv6 addresss
        cout<<"HERE2"<<endl;
        ping_ipv6(argv[1]);
    } 

    
    char host[512];
    char addstr[512];
    for(p = result; p!=NULL; p = p->ai_next){
        if(getnameinfo(p->ai_addr, p->ai_addrlen, host, 512, NULL, 0, 0)){
            assert(false);
        }
        h = &((struct sockaddr_in*)p->ai_addr )->sin_addr;
        cout<<host<<endl;
        cout<<inet_ntop(AF_INET,h,addstr,100)<<endl;
        cout<<"STR"<<addstr<<" "<<p->ai_family<<endl;
        ping_addr(addstr);

    }

}
