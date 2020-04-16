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
#include <math.h>
#include "Ping.hpp"
#define PORT 80
#define WAIT_BETWEEN_RUNS_MS 1000
#define DEFAULT_TIMEOUT_MS 1000
#define SIZE_INT sizeof(int)
using namespace std;

//http://man7.org/linux/man-pages/man7/icmp.7.html
//sudo sysctl -w net.ipv4.ping_group_range="0 65535"
//-f: "Flood", prints "." for each lost packet in the command line
//-r (positive integer): Amount of pings to send. Default is 0, and will run until user quits program
//-t (positive integer): Timeout in milliseconds. Default is 0.

long timeout_us;
long timeout_ms;
long timeout_s;

struct sockaddr_in sock_addr;
struct icmphdr icmp;
socklen_t trash;
char send_buf[sizeof(icmp) + SIZE_INT];
char recieve_buf[sizeof(icmp) + SIZE_INT];
int amt_sent;
int sock;
int pton_result;

//For IPV6 pings only
struct sockaddr_in6 sock_addr_6;
struct icmp6_hdr icmp_6;

mutex time_lock;

uint amt_recieved;
uint amt_lost;
uint amt_attempts;
ulong total_times_ms;
vector<uint> times;

/*
 * Clears the recieve buffer, in case a ping gets echoed back after
 * the timeout has expired
 */
void clear_recv_buffer(){
    //set timeout to 1ms to clear all old ping requests quickly
    char trash_buf[sizeof(icmp) + SIZE_INT];
    struct timeval timeout2;
    timeout2.tv_sec = 0;
    timeout2.tv_usec = 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout2, sizeof(timeout2)); 
    while(recvfrom(sock, trash_buf, sizeof(trash_buf), 0, NULL, &trash)>= 0);


    //Revert timeout to default
    struct timeval timeout;
    timeout.tv_sec = timeout_s;
    timeout.tv_usec = timeout_us;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); 
}

/*
 * Sets up socket and timeout for ipv4 ping
 * @param ip4_addr: the ipv4 address to ping
 */
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
    timeout.tv_sec = timeout_s;
    timeout.tv_usec = timeout_us;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); //CODE TO SET TIEMOUTS


}

/*
 * Sets up socket and timeout for ipv6 ping
 * @param ip6_addr: the ipv6 address to ping
 */
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
    timeout.tv_sec = timeout_s;
    timeout.tv_usec = timeout_us;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); //CODE TO SET TIEMOUTS

}




/*
 * Pings IPv4 address ip_addr, setup_ipv4 MUST be called before this is run
 * @param char* ip_addr: The ipv4 address to ping
 * @return int: reuturns 1 on success and -1 on timeout
 */
int ping_addr(char* ip_addr) {
    struct icmphdr recieved;
    int rec;
    int send_res;
    int num_recieved;

    ++amt_sent;
    memcpy(send_buf+sizeof(icmp), &amt_sent, SIZE_INT);

    //Send packet with icmp struct and amt_sent
    send_res = sendto(sock, send_buf, sizeof(icmphdr) + SIZE_INT,0, (struct sockaddr*)&sock_addr,sizeof(sock_addr));
    if(send_res<0) {
        if(errno == ENETUNREACH) {
            //cout<<"Network unreachable!"<<endl;
            return -1 * errno;
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
        memcpy(&recieved, recieve_buf, sizeof(icmphdr));
        assert(recieved.type == ICMP_ECHOREPLY);        //validate that packet is correct
        memcpy(&num_recieved, &recieve_buf[sizeof(icmphdr)],4);
        if(num_recieved!= amt_sent){
            return -1;
        }
    }
    return 1;
}


/*
 * Pings IPv6 address ip_addr, setup_ipv6 MUST be called before this is run
 * @param char* ip_addr: The ipv6 address to ping
 * @return int: reuturns 1 on success and -1 on timeout
 */
int ping_ipv6(char* ip6_addr) {
    struct icmphdr recieved;
    socklen_t trash = 0;
    int rec;
    int send_res;
    int num_recieved;
    ++amt_sent;

    memcpy(send_buf+sizeof(icmp), &amt_sent, SIZE_INT);

    //Send packet with icmp struct and amt_sent
    send_res = sendto(sock, send_buf, sizeof(icmp6_hdr) + SIZE_INT,0, (struct sockaddr*)&sock_addr_6,sizeof(sock_addr_6));
    if(send_res<0) {
        if(errno == ENETUNREACH) {
            //cout<<"Network unreachable!"<<endl;
            return -1 * errno;
        }
        error_handler((char*)"IPv6 sendto");
    }
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
        memcpy(&recieved, recieve_buf, sizeof(icmphdr));
        assert(recieved.type == ICMP6_ECHO_REPLY);        //validate that packet is correct
        memcpy(&num_recieved, &recieve_buf[sizeof(icmphdr)],4);
        if(num_recieved!= amt_sent){
            return -1;
        }
    }
    return 1;
}


int main(int argc, char *argv[]) {
    signal(SIGINT, exit_handler);
    struct addrinfo address;
    struct addrinfo * result, * p;
    void *h;
    struct in6_addr pton_result;
    bool flood = false;
    int amt_runs = 0;
    amt_sent = 0;
    timeout_ms = DEFAULT_TIMEOUT_MS;

    memset(&address, 0, sizeof(addrinfo));
    memset(&result, 0, sizeof(addrinfo));

    //Parse Arguments
    for(int i = 2; i<argc; ++i){
        if(string(argv[i])== "-f") flood = true;
        if(string(argv[i])== "-t"){
            timeout_ms = atoi(argv[++i]);
            if(timeout_ms < 1){
                cout<<"Timeout must be positive!"<<endl;
                exit(-1);
            } 
            
        }
        if(string(argv[i])== "-r"){
            amt_runs = atoi(argv[++i]);
            if(amt_runs < 1){
                cout<<"Amount of runs must be positive!"<<endl;
                exit(-1);
            } 
        }
    }

    timeout_us = 1000 * (timeout_ms%1000);
    timeout_s = timeout_ms/1000;

    address.ai_family = AF_UNSPEC;
    if(getaddrinfo(argv[1], NULL, &address, &result)) {
        error_handler((char*)"Get address info");
    }

    char* addr = (char*) malloc(1000);
    strcpy(addr, argv[1]);

    //Checks if user entered valid IPv4 or IPv6 address
    if(inet_pton(AF_INET, addr, &pton_result)> 0) {
        cout<<"Pinging IPv4 address: "<<argv[1]<<endl;
        setup_ipv4(argv[1]);
        if(flood) flood_function(ping_addr, amt_runs, argv[1]);
        else time_function(ping_addr, amt_runs, argv[1]);
        exit_handler(1);
    }
    if(inet_pton(AF_INET6, addr, &pton_result)> 0) {
        cout<<"Pinging IPv6 address: "<<argv[1]<<endl;
        setup_ipv6(argv[1]);
        if(flood) flood_function(ping_ipv6, amt_runs, argv[1]);
        else time_function(ping_ipv6, amt_runs, argv[1]);
        exit_handler(1);
    }

    char host[512];
    char addstr[512];
    p = result;
    

    //If user entered ipv4 or ipv6 url, Gets host, then gets IPv4 or IPv6 address
    if(getnameinfo(p->ai_addr, p->ai_addrlen, host, 512, NULL, 0, 0)) {
        error_handler((char*)"Get name info");
    }
    if(p->ai_family == AF_INET) {
        h = &((struct sockaddr_in*)p->ai_addr )->sin_addr;
        inet_ntop(AF_INET,h,addstr,100);
        cout<<"Pinging host: "<<host<<"\nIpv4 address: "<<addstr<<endl;
        setup_ipv4(addstr);

        if(flood) flood_function(ping_addr, amt_runs, addstr);
        else time_function(ping_addr, amt_runs, addstr);
        exit_handler(1);
    }
    else if(p->ai_family == AF_INET6) {
        h = &((struct sockaddr_in6*)p->ai_addr )->sin6_addr;
        inet_ntop(AF_INET6,h,addstr,100);
        cout<<"Pinging host: "<<host<<"\nIpv6 address: "<<addstr<<endl;
        setup_ipv6(addstr);

        if(flood) flood_function(ping_ipv6, amt_runs, addstr);
        else time_function(ping_ipv6, amt_runs, addstr);
        exit_handler(1);
    }
    else {
        assert(false);
    }

}

/*
 *
 * This function prints periods in the terminal, and each period represents a lost packet
 * @param int ping(char*): the function to ping
 * @param int amt_times: the amount of times to run ping(), if this value is 0, runs until user exits
 * @param ping_addr: the IP address given to ping() as it's parameter
 * 
 */
void flood_function(int (*ping)(char*), int amt_times, char* ping_addr){
    amt_recieved = 0;
    amt_lost = 0;
    total_times_ms = 0;

    for(amt_attempts = 0; amt_times == 0 || amt_attempts < amt_times; ) {
        cout<<"."<<flush;
        clear_recv_buffer();
        auto time_start = chrono::system_clock::now();
        int run_result = ping(ping_addr);
        auto time_end = chrono::system_clock::now();
        chrono::milliseconds time_taken = chrono::duration_cast<chrono::milliseconds>(time_end-time_start);
        if(time_taken.count() > timeout_ms) run_result = -1;

        time_lock.lock();
        ++amt_attempts;
        if(run_result < 0) {
            amt_lost++;
        }
        else {
            cout<<"\b \b"<<flush; //Deletes previous "." from cout
            amt_recieved++;
            times.push_back(time_taken.count());
            total_times_ms += time_taken.count();
        }
        time_lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BETWEEN_RUNS_MS));
    }
}

/*
 *
 * This function prints timing data after every run of the function ping, which should return an int representing
 * whether or not the ping was recieved in time
 * @param int ping(char*): the function to ping
 * @param int amt_times: the amount of times to run ping(), if this value is 0, runs until user exits
 * @param ping_addr: the IP address given to ping() as it's parameter
 * 
 */
void time_function(int (*ping)(char*), int amt_times, char* ping_addr) {
    amt_recieved = 0;
    amt_lost = 0;
    total_times_ms = 0;

    for(amt_attempts = 0; amt_times == 0 || amt_attempts < amt_times; ) {
        clear_recv_buffer();
        auto time_start = chrono::system_clock::now();
        int run_result = ping(ping_addr);
        auto time_end = chrono::system_clock::now();
        chrono::milliseconds time_taken = chrono::duration_cast<chrono::milliseconds>(time_end-time_start);
        if(time_taken.count() > timeout_ms) run_result = -1;

        time_lock.lock();
        ++amt_attempts;
        if(run_result < 0) {
            amt_lost++;
            if(run_result * -1 == ENETUNREACH) cout<<"Network unreachable! \tPacket loss: "<<(amt_lost/(float)amt_attempts) *100<<"%"<<endl;
            else cout<<"Time exceeded!  \tPacket loss: "<<(amt_lost/(float)amt_attempts) *100<<"%"<<endl;
        }
        else {
            cout<<"Ping: "<<time_taken.count()<<"ms \t\tPacket loss: "<<(amt_lost/(float)amt_attempts) *100<<"%"<<endl;
            amt_recieved++;
            times.push_back(time_taken.count());
            total_times_ms += time_taken.count();
        }
        time_lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BETWEEN_RUNS_MS));
    }
}


/*
 * Prints error then exits
 * @param char* method: the method where the error occured
 */
void error_handler(char * method) {
    cout<<"Error in "<<method<<"Code: "<<errno<<": "<<strerror(errno)<<endl;
    if(errno == EACCES) cout<<"Try running: $ sudo sysctl -w net.ipv4.ping_group_range=\"0 80\"\nAlso try running program with effective root access"<<endl;
    exit(-1);
}

/*
 * Handles program exits by Ctrl+c and prints statistics
 * @param int code: the exit code
 */
void exit_handler(int code) {
    time_lock.lock();
    cout<<"\nPacket loss percentage: "<<(amt_lost/(float)amt_attempts)*100<<"%"<<endl;
    if(amt_recieved > 0){
        double mean = total_times_ms/(double)amt_recieved;
        double tmpsum = 0;
        vector<uint>::iterator it = times.begin();
        uint minP = WAIT_BETWEEN_RUNS_MS;
        uint maxP = 0;
        while(it!= times.end()){
            tmpsum +=  pow(abs(mean - *it),2);
            minP = min(minP, *it);
            maxP = max(maxP, *it);
            ++it;
        }
        cout<<"Mean:"<<mean<<"ms   Min: "<<minP<<"ms   Max: "<<maxP<< "ms   St. Dev: "<<sqrt(tmpsum / amt_recieved)<<"ms"<<endl;
    }
    else{
        cout<<"No Ping packets recieved"<<endl;
    }

    time_lock.unlock();
    exit(0);
}
