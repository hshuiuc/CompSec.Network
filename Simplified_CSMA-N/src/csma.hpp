#ifndef CSMA_H
#define CSMA_H

#include <stdlib.h>
#include <sys/time.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <cmath>

using std::vector;

/*
struct param_t {
    unsigned int NUM_NODES, PACKET_TICK_SIZE, MAX_RETRANSMISSION, TOTAL_SIMULATION_TICKS;
    vector<unsigned int> COLLISION_ERANGE;
};
param_t getParameters(paramFilename);
*/

class Node {
  public:
    static std::unique_ptr<std::minstd_rand0> random;
    static bool random_initted;

    unsigned int max_backoff;
    unsigned int backoff;
    unsigned int attempts;
    unsigned int collisions;
    unsigned int packets_sent = 0;
    vector<unsigned int> R;
    unsigned int M;

    void Init(vector<unsigned int>& backoffs, unsigned int max_attempts) {
        if (!random_initted) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          random.reset(new std::minstd_rand0(tv.tv_sec * 1000 + tv.tv_usec / 1000));
          random_initted = true;
        }

        R = backoffs;
        M = max_attempts;
        max_backoff = R[0];
        attempts = 0;
        collisions = 0;
        packets_sent = 0;
        ChooseBackoff();
    }

    // Returns whether the node is ready to send packet
    bool WaitTicks(unsigned int ticks) {
        backoff -= ticks;
        return (backoff == 0);
    }

    unsigned int GetWaitTimeLeft() {
        return backoff;
    }

    void ChooseBackoff() {
        backoff = (*random)() % (max_backoff + 1);
    }

    // @param success Whether the last packet sent successfully
    void OnPacketAttempt(bool success) {
        if (success) {
            packets_sent++;
            attempts = 0;
            max_backoff = R[0];
        } else {
            collisions++;
            attempts++;
            if (attempts == M) {
                attempts = 0;
                max_backoff = R[0];
            } else {
                int backoff_idx = (attempts >= R.size()) ? (R.size() - 1) : attempts;
                max_backoff = R[backoff_idx];
            }
        }
        ChooseBackoff();
    }
};

bool Node::random_initted = false;
std::unique_ptr<std::minstd_rand0> Node::random = NULL;

typedef struct ResultsStruct {
  int totalTimeTicks = 0;
  int utilizedTicks = 0;
  int idleTicks = 0;
  int numCollisions = 0;
  float sentVariance = 0.0;
  float collisionsVariance = 0.0;
} Results;

void run_3a();
void run_3b();
void run_3c();
void run_3d();
void run_3e();
void run_all(); 
float variance(const vector<Node>& xSamples, const float xBar, int attr);
Results simulateCSMA(const unsigned int N, const unsigned int L, vector<unsigned int> R, const unsigned int M, const unsigned int T);

#endif