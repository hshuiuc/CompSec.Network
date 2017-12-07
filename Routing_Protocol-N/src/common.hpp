#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

using std::cout;
using std::endl;
using std::ifstream;
using std::ostream;
using std::string;
using std::vector;

#define max(a, b) (a > b ? a : b)

const int INFINITE = -999;
const int NO_ROUTE = -1;

class Route {
 public:
  int next_hop;
  int dist;
  Route() {
    next_hop = NO_ROUTE;
    dist = INFINITE;
  }
  Route(int next_hop, int dist) : next_hop(next_hop), dist(dist) {
  }

  
};

struct Node {
  int n;
  vector<int> edges;
  vector<Route> routes;
  Node(int n) {
    this->n = n;
    edges.assign(n, INFINITE);
    routes.resize(n);
  }

  void SetEdge(int dst, int weight) {
    edges[dst] = weight;
  }

  int GetDist(int dst) {
    return routes[dst].dist;
  }

  void SetDist(int dst, int distance) {
    routes[dst].dist = distance;
  }

  void SetRoute(int dst, int next_hop, int distance) {
    routes[dst].next_hop = next_hop;
    routes[dst].dist = distance;
  }

  void ResetRoutes() {
    routes.assign(n, { NO_ROUTE, INFINITE });
  }

  void SetN(int new_n) {
    edges.resize(new_n);
    for (int i = n; i < new_n; i++) edges[i] = INFINITE;
    routes.resize(n);
    for (int i = n; i < new_n; i++) routes[i] = { NO_ROUTE, INFINITE };
    this->n = new_n;
  }

  vector<int> Neighbors() {
    vector<int> neighbors;
    for (int dst = 0; dst < n; dst++) {
      if (edges[dst] != INFINITE) neighbors.push_back(dst);
    }
    return neighbors;
  }

  friend ostream& operator<<(ostream& os, const Node& node) {
    for (int i = 0; i < node.n; i++) {
      if (node.routes[i].dist != INFINITE) {
        os << (i + 1) << " " << (node.routes[i].next_hop + 1) << " " << node.routes[i].dist << endl;
      }
    }
    return os;
  }
};

class Graph {
 public:
  int n;
  vector<Node *> nodes;

  Graph(int n) {
    this->n = n;
    for (int i = 0; i < n; i++) {
      cout << "Creating node" << endl;
      nodes.push_back(new Node(n));
    }
  }

  Graph(Graph&& other) : n(other.n), nodes(std::move(other.nodes)) {
    other.n = 0;
    other.nodes.resize(0);
  }

  ~Graph() {
    cout << "n = " << n << endl;
    for (int i = 0; i < n; i++) {
      cout << "Freeing node" << endl;
      delete nodes[i];
    }
  }

  void SetEdge(int src, int dst, int weight) {
    while (n <= src || n <= dst) AddNode();
    nodes[src]->SetEdge(dst, weight);
    nodes[dst]->SetEdge(src, weight);
  }

  void SetDist(int src, int dst, int distance) {
    nodes[src]->SetDist(dst, distance);
  }

  void ResetRoutes() {
    for (int i = 0; i < n; i++) {
      nodes[i]->ResetRoutes();
    }
  }
  
  void AddNode() {
    ResetRoutes();
    for (int i = 0; i < n; i++) {
      nodes[i]->SetN(n + 1);
    }
    nodes.resize(n + 1);
    nodes[n] = new Node(n + 1);
    n++;
  }

  void DistanceVectorRoute() {
    for (int node = 0; node < n; node++) {
      Route& self_route = nodes[node]->routes[node];
      self_route.next_hop = node;
      self_route.dist = 0;
    }

    bool complete = false;
    while (!complete) {
      complete = true;
      for (int src = 0; src < n; src++) {
        for (int dst = 0; dst < n; dst++) {
          if (dst == src) continue;
          Route& min_route = nodes[src]->routes[dst];
          for (int hop = 0; hop < n; hop++) {
            if (nodes[src]->edges[hop] == INFINITE
                || nodes[hop]->routes[dst].next_hop == src
                || nodes[hop]->routes[dst].dist == INFINITE) {
              continue;
            }
            int distance = nodes[src]->edges[hop] + nodes[hop]->routes[dst].dist;
            if (min_route.dist == INFINITE
		|| distance < min_route.dist
		|| (distance == min_route.dist && hop < min_route.next_hop)) {
              cout << "Setting route " << (src + 1) << " to " << (dst + 1) << " distance "
                   << distance << " through " << (hop + 1) << endl;
              complete = false;
              min_route.next_hop = hop;
              min_route.dist = distance;
            }
          }
        }
      }
    }
  }

  void LinkStateRoute() {
    for (int src = 0; src < n; src++) {
      Node& src_node = *nodes[src];
      Route& self_route = src_node.routes[src];
      self_route.next_hop = src;
      self_route.dist = 0;
      
      vector<int> parent(n, -1);
      vector<int> unknown;
      for (int i = 0; i < n; i++) unknown.push_back(i);
      while (!unknown.empty()) {

        int min_node = -1;
        for (int node : unknown) {
          if (src_node.routes[node].dist == INFINITE) continue;
          if (min_node == -1
              || (src_node.routes[node].dist < src_node.routes[min_node].dist
                  || (src_node.routes[node].dist == src_node.routes[min_node].dist
                      && node < min_node))) {
            min_node = node;
          }
        }

        Remove(unknown, min_node);
        for (int v : nodes[min_node]->Neighbors()) {
          Route& v_route = src_node.routes[v];
          int new_dist = src_node.routes[min_node].dist + nodes[min_node]->edges[v];
          if (v_route.dist == INFINITE
              || new_dist < v_route.dist
              || (new_dist == v_route.dist && min_node < parent[v])) {
            v_route.dist = new_dist;
            v_route.next_hop = min_node == src ? v : src_node.routes[min_node].next_hop;
            parent[v] = min_node;
          }
        }
      }
    }
  }

  friend ostream& operator<<(ostream& os, const Graph& g) {
    for (int i = 0; i < g.n; i++) {
      os << *g.nodes[i] << endl;
    }
    return os;
  }

 private:
  void Remove(vector<int>& v, int i) {
    for (auto it = v.begin(); it != v.end(); it++) {
      if ((*it) == i) {
        v.erase(it);
        break;
      }
    }
  }
};

class GraphUtil {
 public:
  enum Type {
    LINK_STATE,
    DISTANCE_VECTOR
  };

  GraphUtil(Type type, const string& topofile, const string& changesfile, const string& messagefile)
      : type(type),
      g(Build(topofile)),
      change_idx(0),
      changes(ReadFile(changesfile)),
      messages(ReadFile(messagefile)) {
  }

  Graph* graph() {
    return &g;
  }

  static Graph Build(const string& filename) {
    vector<vector<string>> lines(ReadFile(filename));

    cout << "Lines read" << endl;

    int max_node = 0;
    for (const vector<string>& line : lines) {
      cout << "Line" << endl;
      for (int i = 0; i < 2; i++) {
        cout << line[i] << endl;
        max_node = max(max_node, atoi(line[i].c_str()));
      }
    }

    cout << "max_node: " << max_node << endl;

    Graph g(max_node);

    cout << "Constructed graph" << endl;

    for (const vector<string>& edge : lines) {
      cout << "Setting edge" << endl;
      g.SetEdge(atoi(edge[0].c_str()) - 1, atoi(edge[1].c_str()) - 1, atoi(edge[2].c_str()));
    }
  
    return g;
  }

  bool HasChange() {
    return change_idx < changes.size();
  }

  void ApplyChange() {
    vector<string> line = std::move(changes[change_idx]);
    g.SetEdge(atoi(line[0].c_str()) - 1, atoi(line[1].c_str()) - 1, atoi(line[2].c_str()));
    g.ResetRoutes();
    change_idx++;
  }

  void Route() {
    if (type == DISTANCE_VECTOR) {
      g.DistanceVectorRoute();
    } else {
      g.LinkStateRoute();
    }
  }

  void PrintRoutingTables(ostream& os) {
    os << g;
  }

  void SendMessages(ostream & os) {
    for (const vector<string>& line : messages) {
      int src = atoi(line[0].c_str()) - 1;
      int dst = atoi(line[1].c_str()) - 1;
      int cost = g.nodes[src]->routes[dst].dist;
      string message = std::move(line[2]);
      vector<int> hops;
      if (cost != INFINITE) {
        int cur = src;
        while (cur != dst) {
          hops.push_back(cur);
          cur = g.nodes[cur]->routes[dst].next_hop;
        }
      }
      os << "from " << (src + 1) << " to " << (dst + 1) << " cost ";
      if (cost == INFINITE) {
        os << "infinite hops unreachable ";
      } else {
        os << cost << " hops ";
        for (int hop : hops) os << (hop + 1) << " ";
      }
      os << "message " << message << endl << endl;
    }
  }

 private:
  static vector<vector<string>> ReadFile(const string& filename) {
    ifstream f(filename, ifstream::in);
    if (!f.is_open()) {
      return vector<vector<string>>();;
    }

    cout << "File open" << endl;

    vector<vector<string>> lines;
    while (!f.eof()) {
      string line;
      std::getline(f, line);
      if (!line.empty()) {
        lines.push_back(SplitIntoThree(line));
      }
    }
    f.close();
    return lines;
  }

  static vector<string> SplitIntoThree(const string& s) {
    vector<string> toks;
    char* s_copy = strdup(s.c_str());
    toks.push_back(string(strtok(s_copy, " ")));
    toks.push_back(string(strtok(NULL, " ")));
    toks.push_back(string(strtok(NULL, "")));
    free(s_copy);
    return toks;
  }

  Type type;
  Graph g;
  int change_idx;
  vector<vector<string>> changes;
  vector<vector<string>> messages;
  
};
