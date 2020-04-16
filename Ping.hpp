int ping_addr(char*);
int ping_ipv6(char*);
void setup_ipv6(char*);
void setup_ipv4(char*);
int main(int, char*[]);
void error_handler(char*);
void time_function(int (*f)(char*), int, char*);
void flood_function(int (*f)(char*), int, char*);
void exit_handler(int);
