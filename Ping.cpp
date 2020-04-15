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
#define PORT 80
#define SIZE_INT sizeof(int)
using namespace std;
//http://man7.org/linux/man-pages/man7/icmp.7.html
//sudo sysctl -w net.ipv4.ping_group_range="0 65535"
void ping_addr(char* argv[]){
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

    pton_result = inet_pton(AF_INET, argv[1], &sock_addr.sin_addr);//CHANGE TO AF_INET6 for ipv6
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

    

int main(int argc, char *argv[]){
    assert(argc>1);
    ping_addr(argv);
}
