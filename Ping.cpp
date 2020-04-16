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
#include <chrono>
#include <sys/types.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <thread>
#include <cstdlib>
#include <signal.h>
#include <mutex>

#include "Ping.hpp"
#define PORT 80
#define WAIT_BETWEEN_RUNS_MS 1000
#define SIZE_INT sizeof(int)
using namespace std;
//http://man7.org/linux/man-pages/man7/icmp.7.html
//sudo sysctl -w net.ipv4.ping_group_range="0 65535"
//-f: flood

struct sockaddr_in sock_addr;
struct icmphdr icmp;
char send_buf[sizeof(icmp) + SIZE_INT];
char recieve_buf[sizeof(icmp) + SIZE_INT];
int amt_sent = 0;
int sock;
int pton_result;

//For IPV6 pings only
struct sockaddr_in6 sock_addr_6;
struct icmp6_hdr icmp_6;

mutex time_lock;

uint amt_recieved;
uint amt_lost;
uint amt_attempts;
long total_times_ms;



int ping_addr(char* ip_addr) {


    struct icmphdr recieved;
    int recieved_num = 0;
    socklen_t trash = 0;
    int rec;
    int send_res;

    ++amt_sent;
    memcpy(send_buf+sizeof(icmp), &amt_sent, SIZE_INT);

    //Send packet with icmp struct and amt_sent
    send_res = sendto(sock, send_buf, sizeof(icmphdr) + SIZE_INT,0, (struct sockaddr*)&sock_addr,sizeof(sock_addr));
    if(send_res<0) {
        if(errno == ENETUNREACH) {
            return -1;
        }
        error_handler((char*)"IPv4 sendto");//cout<<"Error: "<<errno<<endl; assert(false);
    }

    rec = recvfrom(sock, recieve_buf, sizeof(recieve_buf), 0, NULL, &trash);
    if(rec<0) {
        if(errno == EAGAIN) {
            return -1;
        }
        else {
            error_handler((char*)"IPv4 Recieve");
        }
    }
    else {
        memcpy(&recieved, recieve_buf, sizeof(recieved));
        assert(recieved.type == ICMP_ECHOREPLY);        //validate that packet is correct
        memcpy(&recieved_num, &recieve_buf[sizeof(icmphdr)],4);
        cout<<"REPLY: "<<recieved_num<<endl;
    }
    return 1;
}



int ping_ipv6(char* ip6_addr) {


    struct icmphdr recieved;
    int recieved_num = 0;
    socklen_t trash = 0;
    int rec;
    int send_res;

    ++amt_sent;
    memcpy(send_buf+sizeof(icmp), &amt_sent, SIZE_INT);

    //Send packet with icmp struct and amt_sent
    send_res = sendto(sock, send_buf, sizeof(icmp6_hdr) + SIZE_INT,0, (struct sockaddr*)&sock_addr_6,sizeof(sock_addr_6));
    if(send_res<0) {
        if(errno == ENETUNREACH) {
            return -1;
        }
        error_handler((char*)"IPv6 sendto");
    }
    cout<<"Sent and waititng"<<endl;
    rec = recvfrom(sock, recieve_buf, sizeof(recieve_buf), 0, NULL, &trash);
    if(rec<0) {
        if(errno == EAGAIN) {
            return -1;
        }
        else {
            error_handler((char*)"IPv6 recieve");
        }
    }
    else {
        memcpy(&recieved, recieve_buf, sizeof(recieved));
        assert(recieved.type == ICMP6_ECHO_REPLY);        //validate that packet is correct
        memcpy(&recieved_num, &recieve_buf[sizeof(icmphdr)],4);
        cout<<"REPLY: "<<recieved_num<<endl;
    }
    return 1;
}

void setup_ipv6(char* ip6_addr) {

    memset(&icmp_6, 0, sizeof(icmp_6));
    memset(&sock_addr_6, 0, sizeof(sock_addr_6));
    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);//IPPROTO_ICMPV6); //CHANGE TO IPPRONT_ICMPV6 for ipv6
    //Use SOCK_DGRAM for UDP to determine quality of network
    if(sock < 0) {
        error_handler((char*)"IPv6 Socket");
    }

    sock_addr_6.sin6_port = htons(PORT);
    sock_addr_6.sin6_family = AF_INET6; //CHANGE TO AF_INET6 for ipv6

    pton_result = inet_pton(AF_INET6, ip6_addr, &sock_addr_6.sin6_addr);//CHANGE TO AF_INET6 for ipv6
    assert(pton_result != 0);
    if(pton_result<0) {
        error_handler((char*)"IPv6 inet_pton(Ip address decoder)");
    }

    icmp_6.icmp6_type = ICMP6_ECHO_REQUEST;

    memcpy(send_buf, &icmp_6, sizeof(icmp_6));

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); //CODE TO SET TIEMOUTS

}

void setup_ipv4(char* ip_addr) {

    memset(&icmp, 0, sizeof(icmp));
    memset(&sock_addr, 0, sizeof(sock_addr));

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP); //CHANGE TO IPPRONT_ICMPV6 for ipv6
    //Use SOCK_DGRAM for UDP to determine quality of network
    if(sock < 0) {
        error_handler((char*)"IPv4 Socket");
    }

    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_family = AF_INET; //CHANGE TO AF_INET6 for ipv6

    pton_result = inet_pton(AF_INET, ip_addr, &sock_addr.sin_addr);//CHANGE TO AF_INET6 for ipv6
    assert(pton_result != 0);
    if(pton_result<0) {
        error_handler((char*)"IPv4 inet_pton(Ip address decoder)");
    }

    icmp.type = ICMP_ECHO;

    memcpy(send_buf, &icmp, sizeof(icmp));


    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); //CODE TO SET TIEMOUTS


}

int main(int argc, char *argv[]) {
    signal(SIGINT, exit_handler);

    struct addrinfo address;
    struct addrinfo * result, * p;
    void *h;
    struct in6_addr help;
    struct hostent *help2electricboogaloo;
    memset(&address, 0, sizeof(addrinfo));
    memset(&result, 0, sizeof(addrinfo));

    address.ai_family = AF_UNSPEC;
    if(getaddrinfo(argv[1], NULL, &address, &result)) {
        error_handler((char*)"Get address info");
    }
    char* addr = (char*) malloc(1000);
    strcpy(addr, argv[1]);
    if(inet_pton(AF_INET, addr, &help)> 0) { //Valid ipv4 addresss
        cout<<"HERE"<<endl;
        setup_ipv4(argv[1]);
        time_function(ping_addr, 30, argv[1]);
    }
    if(inet_pton(AF_INET6, addr, &help)> 0) { //Valid ipv6 addresss
        cout<<"HERE2"<<endl;
        setup_ipv6(argv[1]);
        time_function(ping_ipv6, 30, argv[1]);
    }


    char host[512];
    char addstr[512];
    for(p = result; p!=NULL; p = p->ai_next) {
        if(getnameinfo(p->ai_addr, p->ai_addrlen, host, 512, NULL, 0, 0)) {
            assert(false);
        }
        if(p->ai_family == AF_INET) {
            h = &((struct sockaddr_in*)p->ai_addr )->sin_addr;
            cout<<host<<endl;
            cout<<inet_ntop(AF_INET,h,addstr,100)<<endl;
            cout<<"STR"<<addstr<<" "<<p->ai_family<<endl;
            setup_ipv4(addstr);
            //ping_addr(addstr);
            time_function(ping_addr, 30, addstr);
            exit(1);
        }
        else if(p->ai_family == AF_INET6) {
            h = &((struct sockaddr_in6*)p->ai_addr )->sin6_addr;
            cout<<host<<endl;
            cout<<inet_ntop(AF_INET6,h,addstr,100)<<endl;
            cout<<"STR"<<addstr<<" "<<p->ai_family<<endl;
            setup_ipv6(addstr);
            ping_ipv6(addstr);
        }
        else {
            assert(false);
        }

    }
}

//This function prints timing data after every run and after amt_times have elapsed
//If amt_times == 0, will run until program is stopped
void time_function(int (*ping)(char*), int amt_times, char* ping_addr) {
    amt_recieved = 0;
    amt_lost = 0;
    total_times_ms = 0;

    for(amt_attempts = 0; amt_times == 0 || amt_attempts < amt_times; ) {
        auto time_start = chrono::system_clock::now();
        int run_result = ping(ping_addr);
        auto time_end = chrono::system_clock::now();
        chrono::milliseconds time_taken = chrono::duration_cast<chrono::milliseconds>(time_end-time_start);

        time_lock.lock();
        ++amt_attempts;
        if(run_result < 0) {
            amt_lost++;
            cout<<"Ping: Packet Lost! Packet loss percentage: "<<amt_lost/(float)amt_attempts<<endl;
        }
        else {
            cout<<"Ping: "<<time_taken.count()<<" Packet loss percentage: "<<amt_lost/(float)amt_attempts<<endl;
            amt_recieved++;
            total_times_ms += time_taken.count();
        }
        time_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BETWEEN_RUNS_MS));
    }
}



void error_handler(char * method) {
    cout<<"Error in "<<method<<"Code: "<<errno<<": "<<strerror(errno)<<endl;
    exit(-1);
}

void exit_handler(int code) {
    time_lock.lock();
    cout<<" Packet loss percentage: "<<amt_lost/(float)amt_attempts<<endl;
    time_lock.unlock();
    exit(0);
}
