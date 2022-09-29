import dpkt
import socket
import sys
import base64

ttl_map = dict()
tos_map = dict()

server_ip = '140.113.213.213'

def sol(pcapngPath):
    try:
        with open(pcapngPath, 'rb') as pcapng:
            for _, packet in dpkt.pcapng.Reader(pcapng):
                eth = dpkt.ethernet.Ethernet(packet)
                if eth.type != dpkt.ethernet.ETH_TYPE_IP:
                    continue
                ip = eth.data
                if ip.p != dpkt.ip.IP_PROTO_TCP:
                    continue
                dst_ip = socket.inet_ntoa(ip.dst)
                src_ip = socket.inet_ntoa(ip.src)    
                if dst_ip != server_ip and src_ip != server_ip:
                    continue
                tcp = ip.data
                if ip.tos in tos_map:
                    tos_map[ip.tos] = -1
                else:
                    tos_map[ip.tos] = tcp.data
                if ip.ttl in ttl_map:
                    ttl_map[ip.ttl] = -1
                else:
                    ttl_map[ip.ttl] = ip.tos
        for _, tos in ttl_map.items():
            if tos != -1 and tos_map[tos] != -1:
                FLAG = base64.b64decode(tos_map[tos]).decode('UTF-8')
                print(FLAG)
                sys.exit(0)
    except FileNotFoundError:
        print("DNE!!!")
        
if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage:")
        print("python3 lab1_part2.py <file path>")
        print("python3.10 lab1_part2.py <file path>")
        sys.exit(0)
    elif len(sys.argv) > 2:
        print("Too many arguments!!!")
        sys.exit(1)
    sol(sys.argv[1])
