#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>

using std::ifstream;
using std::vector;
using std::pair;
using std::map;
using std::string;
using std::stringstream;
using std::getline;

struct greaterCost{
	bool operator()(const pair<int,vector<int>>& lp, const pair<int,vector<int>>& rp) const{
		if (lp.first != rp.first)
			return lp.first > rp.first;
		else
			// tie breaking: choose the lowest node ID to move to the 'finished' set
			return lp.second.back() > rp.second.back();
	}
};

class Node {
  public:
	map<int,int> neighborNodesCosts;				// (node id of nb, cost to that nb node)
	map<int,pair<int,vector<int>>> routingTable;	// (destination node id, (lowest cost, l.c. path via id sequence))

	Node() {}
	void editNeighborOrChangeCost(int, int);
};
void Node::editNeighborOrChangeCost(int neighborId, int newCost){
	if (newCost == -999){
		// -999 indicates that the previously existing link between the two nodes is broken
		neighborNodesCosts.erase(neighborId);
	} else if (newCost <= 0)
		// real link costs are always positive; never zero or negative
		return;
	else
		neighborNodesCosts[neighborId] = newCost;
}

void convergeRoutingTables(map<int, Node>& networkRouters, FILE *& fpOut){
	pair<int,vector<int>> nextLowestCostPath;
	int costSoFar, lastNode;

	// perform Dijkstra on each router to compute their routing tables
	for (auto & router : networkRouters){
		// empty previous table
		map<int,pair<int,vector<int>>>& rT = router.second.routingTable;
		rT.clear();

		// empty heap initialized to the starting node--its own node
		vector<pair<int,vector<int>>> minHeap;
		minHeap.emplace_back(0, vector<int>{router.first});
		
		while (!minHeap.empty()){
			std::pop_heap(minHeap.begin(), minHeap.end(), greaterCost());
			nextLowestCostPath = minHeap.back();	// (cost of path, list of nodes visited on path)
			minHeap.pop_back();
			costSoFar = nextLowestCostPath.first;
			lastNode = nextLowestCostPath.second.back();

			// if next l.c. path's last node is not visited, mark it as visited via registering its id with the
			// routing table, and add the post-fix path to its neighbors onto the min heap
			if (rT.count(lastNode) == 0){
				rT[lastNode] = nextLowestCostPath;
				for (auto nodeAndCost : networkRouters[lastNode].neighborNodesCosts){
					vector<int> pathCopy = nextLowestCostPath.second;
					pathCopy.push_back(nodeAndCost.first);
					minHeap.emplace_back(costSoFar + nodeAndCost.second, pathCopy);
					std::push_heap(minHeap.begin(), minHeap.end(), greaterCost());
				}
			}
		}

		for (auto const & destNode : rT){
    		fprintf(fpOut, "%d ", destNode.first);
    		if (destNode.second.second.size() > 1)
    			fprintf(fpOut, "%d ", destNode.second.second[1]);
    		else
    			fprintf(fpOut, "%d ", destNode.second.second[0]);
    		fprintf(fpOut, "%d\n", destNode.second.first);
    	}
	}
}

void sendMessages(const map<int, Node>& networkRouters, const string msgFile, FILE *& fpOut){
	ifstream msgStream(msgFile);
	string fileLine, msg;
    stringstream lineStream;
    int srcId, dstId;

    // look up best path between src and dest of each message and send it
    while (msgStream.good()){
    	getline(msgStream, fileLine);
    	if (fileLine.empty())
    		continue;
    	lineStream.str(fileLine);
    	lineStream >> srcId >> dstId >> std::ws;
    	getline(lineStream, msg);
    	lineStream.clear();

    	auto const srcDstEntry = networkRouters.at(srcId).routingTable.find(dstId);
    	if (srcDstEntry != networkRouters.at(srcId).routingTable.end()){
    		auto const & costAndPath = srcDstEntry->second;
	    	fprintf(fpOut, "from %d to %d cost %d hops ", srcId, dstId, costAndPath.first);
	    	for (auto it = costAndPath.second.begin(); it < costAndPath.second.end()-1; it++)
	    		fprintf(fpOut, "%d ", *it);
	    	fprintf(fpOut, "message %s\n", msg.c_str());
	    } else
	    	fprintf(fpOut, "from %d to %d cost infinite hops unreachable message %s\n", srcId, dstId, msg.c_str());
    }
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }
    FILE *fpOut;
    fpOut = fopen("output.txt", "w");

    map<int, Node> networkRouters;		// (node id, associated object)
    ifstream initTopo(argv[1]);
    ifstream topChanges(argv[3]);
    string fileLine;
    stringstream lineStream;
    int firstId, secondId, cost;

    // read in network topology and set up the graph of routers
    while (initTopo.good()){
    	getline(initTopo, fileLine);
    	if (fileLine.empty())
    		continue;
    	lineStream.str(fileLine);
    	lineStream >> firstId >> secondId >> cost;
    	lineStream.clear();

    	// insert uniquely new vertices and edges (or update with latest cost)
    	if (networkRouters.count(firstId) == 0)
    		networkRouters[firstId] = Node();
    	networkRouters[firstId].editNeighborOrChangeCost(secondId, cost);
    	if (networkRouters.count(secondId) == 0)
    		networkRouters[secondId] = Node();
    	networkRouters[secondId].editNeighborOrChangeCost(firstId, cost);
    }

    // converge tables before topology changes then print them and messages to output
    convergeRoutingTables(networkRouters, fpOut);
    sendMessages(networkRouters, argv[2], fpOut);

    // update tables after each change, and re-print them plus messages to output
    while (topChanges.good()){
    	getline(topChanges, fileLine);
    	if (fileLine.empty())
    		continue;
    	lineStream.str(fileLine);
    	lineStream >> firstId >> secondId >> cost;
    	lineStream.clear();

    	networkRouters[firstId].editNeighborOrChangeCost(secondId, cost);
    	networkRouters[secondId].editNeighborOrChangeCost(firstId, cost);

    	convergeRoutingTables(networkRouters, fpOut);
    	sendMessages(networkRouters, argv[2], fpOut);
    }

    /*// print out network topo for testing
    for (auto node : networkRouters){
    	std::cout << node.first << " : ";
    	for (pair<int, int> elem : node.second.neighborNodesCosts)
    		std::cout << elem.first << "," << elem.second << " ";
    	std::cout << "\n";
    }*/

    fclose(fpOut);
    return 0;
}

