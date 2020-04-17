# Ping from Scratch
## Built using system calls in ubuntu

### Usage: 

make: builds executable

./Ping (URL or IP address to ping - Works with IPv4 and IPv6)

NOTE!: Due to limitations of my home network, I was unable to test the IPv6 functionality beyond localhost(::1)

#### Arguments:
-t: Maximum round trip time in milliseconds, default is 1000

-r: Amount of times to ping, default is 0 and will ping until user exits with Ctrl+c

-f: Prints "." to console, each period represents a packet that was lost or didn't arrive in time
    
