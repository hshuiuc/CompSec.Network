#include "csma.hpp"

using std::cout;

// N=25, L=20, R=8 16 32 64 128, M=6, T=50000
//const int DEFAULT_N = 25;
const int DEFAULT_L = 20;
const vector<unsigned int> DEFAULT_R = { 8, 16, 32, 64, 128 };
const int DEFAULT_M = 6;
const int DEFAULT_T = 50000;

void run_all() {
  run_3a();
  run_3b();
  run_3c();
  run_3d();
  run_3e();
}

float variance(const vector<Node>& xSamples, const float xBar, int attr){
    float sigmaSq = 0.0;

    for (Node x : xSamples){
        if (attr == 1)
            sigmaSq += std::pow(x.packets_sent - xBar, 2);
        else if (attr == 2)
            sigmaSq += std::pow(x.collisions - xBar, 2);
    }

    sigmaSq /= xSamples.size();
    return sigmaSq;
}

Results simulateCSMA(const unsigned int N, const unsigned int L, vector<unsigned int> R, const unsigned int M, const unsigned int T) {
    vector<Node> nodes(N);
    for (int i = 0; i < N; i++)
        nodes[i].Init(R, M);

    vector<int> transmissionReadyNodes;    // track index of nodes that reached backoff = 0
    Results results;
    int& totalTimeTicks = results.totalTimeTicks;
    int& utilizedTicks = results.utilizedTicks;
    int& idleTicks = results.idleTicks;
    int& totalCollisions = results.numCollisions;
    int successfulTransmissions = 0, totalPerNodeCollisions = 0;    // multiple nodes reaching 0 backoff increments collision count

    while (totalTimeTicks < T){
        transmissionReadyNodes.clear();

        // decrement backoff for all nodes and find ones ready to transmit a packet
        unsigned int min_backoff = nodes[0].GetWaitTimeLeft();
        for (int i = 0; i < N; i++) {
            if (nodes[i].GetWaitTimeLeft() < min_backoff) min_backoff = nodes[i].GetWaitTimeLeft();
        }
        totalTimeTicks += min_backoff;
        idleTicks += min_backoff;
        for (int i = 0; i < N; i++) {
            if (nodes[i].WaitTicks(min_backoff)) transmissionReadyNodes.push_back(i);
        }

        if (transmissionReadyNodes.size() == 1){
            // send a (fixed-size) packet from that node, indicate successful attempt
            totalTimeTicks += L;
            utilizedTicks += L;
            successfulTransmissions += 1;
            nodes[transmissionReadyNodes[0]].OnPacketAttempt(true);
        } else {
            // more than one node ready to transmit, meaning collision occurs
            totalCollisions += 1;
            totalPerNodeCollisions += transmissionReadyNodes.size();
            for (int nodeIdx : transmissionReadyNodes)
                nodes[nodeIdx].OnPacketAttempt(false);
            totalTimeTicks++;
        }
    }

    results.sentVariance = variance(nodes, ((float) successfulTransmissions)/N, 1);
    results.collisionsVariance = variance(nodes, ((float) totalPerNodeCollisions)/N, 2);
    return results;
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 2) {
        cout << "Usage: ./csma input.txt\n";
        return -1;
    }

    // parse input parameter file for runtime values
    std::ifstream parameters(argv[1]);
    unsigned int ival;
    parameters.ignore();
    parameters >> ival;
    const unsigned int NUM_NODES = ival;
    parameters.ignore(2);
    parameters >> ival;
    const unsigned int PACKET_TICK_SIZE = ival;
    parameters.ignore(2);
    vector<unsigned int> COLLISION_ERANGE;
    while (parameters.peek() != '\n'){
        parameters >> ival;
        COLLISION_ERANGE.push_back(ival);
    }
    parameters.ignore(2);
    parameters >> ival;
    const unsigned int MAX_RETRANSMISSION = ival;
    parameters.ignore(2);
    parameters >> ival;
    const unsigned int TOTAL_SIMULATION_TICKS = ival;
    parameters.close();

    Results results = simulateCSMA(NUM_NODES, PACKET_TICK_SIZE, COLLISION_ERANGE, MAX_RETRANSMISSION, TOTAL_SIMULATION_TICKS);

    // print out statistics
    std::ofstream output("output.txt");
    output << "Channel utilization " << results.utilizedTicks*100.0 / results.totalTimeTicks << " %\n";
    output << "Channel idle fraction " << results.idleTicks*100.0 / results.totalTimeTicks << " %\n";
    output << "Total number of collisions " << results.numCollisions << "\n";
    output << "Variance in number of successful transmissions " << results.sentVariance << "\n";
    output << "Variance in number of collisions " << results.collisionsVariance << "\n";
    output.close();

    // for analysis pdf
    //run_all();

    return 0;
}

void run_3a() {
  std::ofstream output("3a.txt");
  Results results;
  for (unsigned int N = 5; N <= 100; N++) {
    results = simulateCSMA(N, DEFAULT_L, DEFAULT_R, DEFAULT_M, DEFAULT_T);
    output << results.utilizedTicks*100.0 / results.totalTimeTicks << '\n';
  }
  output.close();
}

void run_3b() {
  std::ofstream output("3b.txt");
  Results results;
  for (unsigned int N = 5; N <= 100; N++) {
    results = simulateCSMA(N, DEFAULT_L, DEFAULT_R, DEFAULT_M, DEFAULT_T);
    output << results.idleTicks*100.0 / results.totalTimeTicks << '\n';
  }
  output.close();
}

void run_3c() {
  std::ofstream output("3c.txt");
  Results results;
  for (unsigned int N = 5; N <= 100; N++) {
    results = simulateCSMA(N, DEFAULT_L, DEFAULT_R, DEFAULT_M, DEFAULT_T);
    output << results.numCollisions << '\n';
  }
  output.close();
}

void run_3d() {
  vector<vector<unsigned int>> Rs;
  for (unsigned int start = 1; start <= 16; start *= 2) {
    vector<unsigned int> r;
    for (int i = 0; i < 7; i++) {
      r.push_back(start << i);
    }
    Rs.push_back(r);
  }

  std::ofstream output("3d.txt");
  for (auto& R : Rs) {
    Results results;
    for (unsigned int N = 5; N <= 100; N++) {
      results = simulateCSMA(N, DEFAULT_L, R, DEFAULT_M, DEFAULT_T);
      output << results.utilizedTicks*100.0 / results.totalTimeTicks;
      if (N < 100) output << ',';
    }
    output << '\n';
  }
  output.close();
}

void run_3e() {
  vector<unsigned int> Ls = { 20, 40, 60, 80, 100 };

  std::ofstream output("3e.txt");
  for (auto L : Ls) {
    Results results;
    for (unsigned int N = 5; N <= 100; N++) {
      results = simulateCSMA(N, L, DEFAULT_R, DEFAULT_M, DEFAULT_T);
      output << results.utilizedTicks*100.0 / results.totalTimeTicks;
      if (N < 100) output << ',';
    }
    output << '\n';
  }
  output.close();
}

