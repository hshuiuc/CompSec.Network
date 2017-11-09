#!/usr/bin/python2.7
import dpkt
import sys
import socket

def main():
    if (len(sys.argv) < 2):
        print "error: need argument"
        sys.exit(1)

    filename = sys.argv[1]
    f = open(filename)
    pcap = dpkt.pcap.Reader(f)
    ipToSynAck = {}     # IP to [SYN, SYN+ACK] count mapping

    for ts, buf in pcap:
        try:
            eth = dpkt.ethernet.Ethernet(buf)
        except:
            continue

        if type(eth.data) != dpkt.ip.IP:
            continue

        ip = eth.data
        if type(ip.data) != dpkt.tcp.TCP:
            continue

        tcp = ip.data
        syn_flag = (tcp.flags & dpkt.tcp.TH_SYN) != 0
        ack_flag = (tcp.flags & dpkt.tcp.TH_ACK) != 0
        other_flags = (tcp.flags & ~dpkt.tcp.TH_SYN & ~dpkt.tcp.TH_ACK) != 0

        if syn_flag and not other_flags:
            if ack_flag:
                destIP = ip.dst

                if destIP in ipToSynAck:
                    ipToSynAck[destIP][1] += 1
                else:
                    ipToSynAck[destIP] = [0, 1]
            else:
                srcIP = ip.src

                if srcIP in ipToSynAck:
                    ipToSynAck[srcIP][0] += 1
                else:
                    ipToSynAck[srcIP] = [1, 0]

    for uniqIP, (syn, synack) in ipToSynAck.items():
        if syn >= 3 * synack:   # if count of syn is at least 3 times more than syn + ack for an IP
            print socket.inet_ntoa(uniqIP)

    f.close()
    sys.exit(0)


if __name__ == '__main__':
    main()
