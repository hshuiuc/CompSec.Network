/* 
 * File:   receiver_main.cpp
 * Author: Ben Li
 *
 * Created on Oct. 25, 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <cstdint>
#include <fstream>
#include <unordered_map>

#define HEADER_SIZE 28              // 20B IPv4 + 8B UDP
#define CUSTOM_HEAD 4               // FIN bit + 31 bits ACK #
#define MSS (1500 - HEADER_SIZE)    // maximum segment size | size of our packets
#define N (350 * (MSS - CUSTOM_HEAD))   // size of in-flight packets window; custom header not stored
#define FIN_MASK 0x80000000         // MSB of custom header indicates client close req
#define MAX_SEQ 0x7FFFFFFF          // rest of 31 bits represent sequence numbering range

struct sockaddr_in si_me, si_other;
int s, recvlen, rv;
socklen_t slen;
uint8_t     buf[MSS];
uint8_t     pktWnd[N];
uint32_t    seqBaseN;
uint32_t    seqNum;
std::ofstream dest;
std::unordered_map<uint32_t, int> pktTracker;     // sequence #, length pairs


void diep(char const *s) {
    perror(s);
    exit(1);
}

bool isInPktWnd(uint32_t lowerBound, uint32_t n, uint32_t upperBound){
    lowerBound &= MAX_SEQ;  // effectively %= FIN_MASK
    upperBound &= MAX_SEQ;  // n passed in must be in [0, MAX_SEQ]

    if (lowerBound > upperBound) {
        // Window range is wrapped around 2^31 (MAX_SEQ) -> 0
        if ((lowerBound <= n && n <= MAX_SEQ) || /* 0 <= n && */ n <= upperBound)
            return true;
    } 
    else
        if (lowerBound <= n && n <= upperBound)
            return true;
    return false;
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    slen = sizeof (si_other);
    memset(buf, 0, MSS);
    memset(pktWnd, 0, N);
    seqBaseN = 0;
    dest.open(destinationFile, std::ios::out | std::ios::trunc);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");

	/* Now receive data and send acknowledgements */
    while(1) {
        recvlen = recvfrom(s, buf, MSS, 0, (struct sockaddr *)&si_other, &slen);

        seqNum = (int)buf[0] << 24 | (int)buf[1] << 16 | (int)buf[2] << 8 | (int)buf[3];

        if(seqNum & FIN_MASK){
            // FIN bit is set: reply "OK" via mirroring packet, and close connection
            // Note: sender should not send FIN until all desired ACKs have been received
            for(int finacks = 0; finacks < 3; finacks++)
                sendto(s, buf, recvlen, 0, (struct sockaddr *)&si_other, slen);
            break;
        }

        seqNum &= MAX_SEQ;
        if (isInPktWnd(seqBaseN, seqNum, seqBaseN + N - 1)) {
            uint32_t pktWndIdx = (seqNum - seqBaseN) & MAX_SEQ;

            if (pktWndIdx == 0) {
                /*  Expected packet can't already be stored, since we continuously update 
                    in-order arrivals; prepare to deliver all in-order data. */
                memcpy(pktWnd, &buf[CUSTOM_HEAD], recvlen - CUSTOM_HEAD);
            } 
            else {
                /*  Store out-of-order packet (incl. custom header), if not already buffered,
                    keeping track of its sequence and length for next in-order delivery */
                if (pktTracker.find(seqNum) == pktTracker.end()){
                    memcpy(&pktWnd[pktWndIdx], &buf[CUSTOM_HEAD], recvlen - CUSTOM_HEAD);
                    pktTracker[seqNum] = recvlen - CUSTOM_HEAD;
                }

                // then re-send cumulative ACK
                goto _resendACK;
            }
        } else if (isInPktWnd(seqBaseN - N, seqNum, seqBaseN - 1)) {
            /*  Retransmitted duplicates of recent packets; resend cumulative ACK
                Note: since window always shift on in-order pkt, expected seq is base number. */
        _resendACK:
            for(int m = 0; m <= 3; m++)
                buf[m] = (uint8_t)(seqBaseN >> (3 - m)*8);

            rv = sendto(s, buf, CUSTOM_HEAD, 0, (struct sockaddr *)&si_other, slen);
            printf("duplicate ACK sent (pkt rcvd) for seq # %d\n", seqBaseN);
            continue;   // back to waiting for next packet
        } else {
            // Ignore packet - this case should practically never be possible
            continue;
        }
        //-----------------------------------------------------If packet is the expected,
        // Iteratively find window range of all in-order segments
        uint32_t nextExpectedSeq, nextInOrdSegLen;
        nextExpectedSeq = (seqNum + recvlen - CUSTOM_HEAD) & MAX_SEQ;

        while (pktTracker.find(nextExpectedSeq) != pktTracker.end()){
            nextInOrdSegLen = pktTracker[nextExpectedSeq];
            pktTracker.erase(nextExpectedSeq);  // free sequence # key to be used for future pkts
            nextExpectedSeq += nextInOrdSegLen;
        }

        // Send new cumulative ACK to sender
        for(int m = 0; m <= 3; m++)
            buf[m] = (uint8_t)(nextExpectedSeq >> (3 - m)*8);
        rv = sendto(s, buf, CUSTOM_HEAD, 0, (struct sockaddr *)&si_other, slen);
        printf("new ACK sent for seq # %d\n", nextExpectedSeq);

        /******************************************************************
        Recall, at this point:      PKTS WINDOW of N size
            __________________________________________________________
           |    IN-ORDER SEGS      .    MISSING/OUT-of-ORDER SEGS
           |_______________________.__________________________________
            ^seqBaseN == seqNum     ^nextExpectedSeq
        ******************************************************************/

        // Update window by sliding all packets towards base, and deliver data to dest File
        dest.write((char *)pktWnd, nextExpectedSeq - seqBaseN);

        memmove(pktWnd, &pktWnd[nextExpectedSeq - seqBaseN], N - (nextExpectedSeq - seqBaseN));
        memset(&pktWnd[N - (nextExpectedSeq - seqBaseN)], 0, nextExpectedSeq - seqBaseN);

        seqBaseN = nextExpectedSeq;
        // memset(buf, 0, MSS);    // this is only for testing; display output better
    }

    dest.close();
    close(s);
	printf("%s received.", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

