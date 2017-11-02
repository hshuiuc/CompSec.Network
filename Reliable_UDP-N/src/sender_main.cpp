/*
 * File:   sender_main.c
 * Author: Sam Okrent
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

using std::condition_variable;
using std::cout;
using std::endl;
using std::ifstream;
using std::max;
using std::min;
using std::mutex;
using std::unique_lock;

#define MAX_PACKET_SIZE 1500
#define MAX_DATA_SIZE (MAX_PACKET_SIZE - 28)
#define HEADER_SIZE 4
#define MSS (MAX_DATA_SIZE - HEADER_SIZE)
#define WIN_SIZE_BYTES (50*1024*1024 / 50)
#define WIN_SIZE (WIN_SIZE_BYTES / MAX_PACKET_SIZE)
#define FIN_MASK 0x80000000
#define FIN_BYTE 0x80
#define ACK_MASK 0x7FFFFFFF
#define MAX_SEQ_NUM ACK_MASK
#define WRAP (uint64_t) FIN_MASK

#define ALPHA    0.125f
#define BETA     0.25f

struct sockaddr_in si_other;
int s, slen;
uint64_t bytes_to_send;

mutex m;
condition_variable cond;

ifstream* f;

uint32_t seq_num = 0;
uint32_t send_num = 0;
uint64_t bytes_sent = 0;
uint32_t cwnd = 4 * MSS;
int dup_acks = 0;
uint32_t ssthresh = 32 * MSS;
bool fast_retransmit = false;
int timeoutInterval = 20; // milliseconds
float estRTT = 20.0;
float devRTT = 0.0;
uint64_t timer;

/* Updates the timeout interval per call */
void calculateTimeoutInterval(int sampleRTT) {
    devRTT = (1-BETA)*devRTT + BETA*abs(sampleRTT - estRTT);
    estRTT = (1-ALPHA)*estRTT + ALPHA*sampleRTT;
    timeoutInterval = (int)(estRTT + 4*devRTT);
}

void diep(const char *s) {
    perror(s);
    exit(1);
}

uint64_t GetTimeMillis() {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

bool IsWrap(uint64_t first, uint64_t second) {
    return (second < first &&
            second <= WIN_SIZE_BYTES &&
            first >= (MAX_SEQ_NUM - WIN_SIZE_BYTES));
}

bool IsGreaterSequenceNumber(uint64_t first, uint64_t second) {
    return IsWrap(first, second) || ((second > first) && !IsWrap(second, first));
}

uint64_t SequenceDistance(uint64_t first, uint64_t second) {
    if (IsWrap(first, second)) {
        return (WRAP + second) - first;
    } else {
        return second - first;
    }
}

uint64_t ReadChunk(uint64_t start_idx, char* chunk) {
        cout << "ReadChunk(" << start_idx << ")" << endl;
        f->seekg(start_idx, std::ios_base::beg);
        uint64_t num_bytes = min(bytes_to_send - start_idx, (uint64_t) MSS);
        f->read(chunk, num_bytes);
        num_bytes = f->gcount();
        if (f->eof()) {
                cout << "End of file reached!" << endl;
                unique_lock<mutex> lock(m);
                bytes_to_send = start_idx + num_bytes;
        }
        return num_bytes;
}

void SendData() {
    cout << "Starting data sending thread." << endl;
    char data[HEADER_SIZE + MSS];
    uint32_t local_send_num;
    uint64_t read_idx;
    unique_lock<mutex> lock(m);
    bool reached_bytes_to_send = false;
    while(bytes_sent < bytes_to_send) {
        cond.wait(lock, [&reached_bytes_to_send]() {
            bool fits_in_c_window = !IsGreaterSequenceNumber((seq_num + cwnd) & MAX_SEQ_NUM,
                                        (send_num + MSS) & MAX_SEQ_NUM);
            bool more_bytes_to_send = (bytes_sent + SequenceDistance(seq_num, send_num) < bytes_to_send);
            reached_bytes_to_send = bytes_sent >= bytes_to_send;
            /*cout << "Wait condition check: "
                << " fits in congestion window: "
                << fits_in_c_window
                << ", have more bytes to send: "
                << more_bytes_to_send
                << ", Sequence distance: " << SequenceDistance(seq_num, send_num) << endl;
            cout << "seq_num: " << seq_num << " cwnd: " << cwnd
                << " send_num: " << send_num << " bytes_sent: " << bytes_sent << endl;*/
            return fast_retransmit || reached_bytes_to_send || (more_bytes_to_send && fits_in_c_window);
        });
        if (reached_bytes_to_send)
            break;
        if (fast_retransmit) {
            local_send_num = seq_num;
            fast_retransmit = false;
        } else {
            local_send_num = send_num;
        }
        read_idx = bytes_sent + SequenceDistance(seq_num, local_send_num);
        lock.unlock();

        cout << "New packet send attempt. seq_num: " << seq_num
            << " bytes_sent: " << bytes_sent
            << " send_num: " << local_send_num << " cwnd: " << cwnd << endl;

        // Read and send another packet
        int data_len = ReadChunk(read_idx, data + HEADER_SIZE);
        data[0] = (char) ((local_send_num & 0xff000000) >> 24);
        data[1] = (char) ((local_send_num & 0x00ff0000) >> 16);
        data[2] = (char) ((local_send_num & 0x0000ff00) >> 8);
        data[3] = (char) ((local_send_num & 0x000000ff) >> 0);
        if (sendto(s, data, HEADER_SIZE + data_len, 0, (struct sockaddr*) &si_other, slen) < 0) {
            cout << "Error sending data" << endl;
            lock.lock();
            continue; // Retry this chunk
        }

        cout << "Sent " << data_len << " bytes." << endl;

        lock.lock();
        if (send_num == local_send_num) {
            send_num += data_len;
            send_num = send_num & MAX_SEQ_NUM;
        }
    }
    lock.unlock();

    cout << "Starting FIN sending process" << endl;

    // FIN stuff
    bool received_fin_ack = false;
    int fin_attempts = 0;
    while (fin_attempts < 3 && !received_fin_ack) {
        memset(data, 0, HEADER_SIZE);
        data[0] = FIN_BYTE;
        for (int i = 0; i < 3; i++) {
            if (sendto(s, &data, HEADER_SIZE, 0, (struct sockaddr*) &si_other, slen) < 0) {
                cout << "Error sending FIN" << endl;
            }
        }
        uint64_t timer = GetTimeMillis();
        int ret = 0;
        bool timed_out = false;
        unsigned char response[HEADER_SIZE];
        do {
            ret = recvfrom(s, response, HEADER_SIZE, 0, NULL, NULL);
        } while (ret < 0 && errno == EAGAIN && !(timed_out = (GetTimeMillis() - timer > timeoutInterval)));
        if (timed_out) {
            cout << "Timed out waiting for FIN ACK." << endl;
            fin_attempts++;
        } else if (ret > 0 && response[0] == FIN_BYTE) {
            cout << "Received FIN ACK, exiting." << endl;
            received_fin_ack = true;
        } else {
            cout << "Response is not an expected FIN ack." << endl;
        }
    }
}

// Not thread-safe
void HandlePacketLoss(bool fast_recovery) {
    // ssthresh = cwnd / 2;
    send_num = seq_num; // Change if doing fast retransmit
    if (fast_recovery) {
        cwnd = max(ssthresh + 3, (uint32_t) 4 * MSS);
        // fast_retransmit = true;
    } else {
        cwnd = 4 * MSS;
    }
    dup_acks = 0;
    timer = GetTimeMillis();
    cout << "Handling packet loss, new ssthresh: " << ssthresh
            << " new cwnd: " << cwnd
            << " new send_num: " << send_num << endl;
    cond.notify_all();
}

void ListenForReceiverAcks() {
    cout << "Starting ack listening thread." << endl;
    timer = GetTimeMillis();
    unique_lock<mutex> lock(m);
    int ret = 0;
    bool timed_out = false;
    unsigned char data[4];
    uint32_t ack_num = 0;
    int saved_errno = 0;
    while(bytes_sent < bytes_to_send) {
        lock.unlock();
        ret = 0;
        timed_out = false;
        do {
            ret = recvfrom(s, data, 4, 0, NULL, NULL);
            if (ret > 0) {
                ack_num = ((data[0] & 0x7F) << 24) | (data[1] << 16) |
                        (data[2] << 8) | (data[3] << 0);
                std::cout << "got a ACK pkt with number: " << ack_num << "\n";
            } else {
                saved_errno = errno;
                timed_out = (GetTimeMillis() - timer > timeoutInterval);
            }
        } while (ret < 0 && saved_errno == EAGAIN && !timed_out);

        lock.lock();
        if (ret < 0 && saved_errno != EAGAIN) {
            perror("Error reading from socket");
        } else if (timed_out) {
            cout << "Time out, seq_num: " << seq_num << ", send_num: " << send_num << endl;
            HandlePacketLoss(false);
        } else if (IsGreaterSequenceNumber(seq_num, ack_num)) {
            // Handle ack
            cout << "Received new ack: " << ack_num << endl;
            if (cwnd < ssthresh) {
                cwnd = min(cwnd + MSS, ssthresh);
            } else {
                cwnd += (MSS * MSS) / cwnd;
            }
            bytes_sent += SequenceDistance(seq_num, ack_num);
            seq_num = ack_num;
            if (IsGreaterSequenceNumber(seq_num, send_num))
                send_num = seq_num;
            dup_acks = 0;
            cond.notify_all();
            timer = GetTimeMillis();
        } else if (ack_num == seq_num) {
            cout << "Dup ack: " << ack_num << ", send_num: " << send_num << endl;
            dup_acks++;
            if (dup_acks == 3) {
                HandlePacketLoss(true);
            }
        } else if (ack_num < seq_num) {
            // Do nothing?
        }
    }
    lock.unlock();
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, uint64_t bytesToTransfer) {
    // Open the file
    ifstream file(filename, std::ios::in|std::ios::binary);
    if (!file.is_open()) {
        diep("Could not open file");
    }

    bytes_to_send = bytesToTransfer;
    f = &file;
    /*
    file.seekg(0, std::ios_base::end);
    bytes_to_send = min((uint64_t) file.tellg(), bytesToTransfer);
    file.seekg(0, std::ios_base::beg);
    f = &file;
    */

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    struct timeval rcv_timeout;
    memset((char*) &rcv_timeout, 0, sizeof(rcv_timeout));
    rcv_timeout.tv_usec = 5000; // 5 milliseconds
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof(struct timeval)) < 0) {
        diep("Could not set recv timeout");
    }

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    std::thread recv_thread(ListenForReceiverAcks);
    std::thread send_thread(SendData);
    send_thread.join();
    recv_thread.join();

    printf("Closing the socket\n");
    close(s);
    file.close();
    return;

}

int main(int argc, char** argv) {

    unsigned short int udpPort;
    uint64_t numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}