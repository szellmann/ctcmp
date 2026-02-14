#include <cfloat>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include "util.h"

struct Node {
  std::string name;
  double value;
  int fanIn{0}, fanOut{0};
};

struct Edge {
  int from, to;
};

struct Graph {
  std::vector<Node> V;
  std::vector<Edge> E;

  int findOrAddNode(const Node &n) {
    auto it
      = std::find_if(V.begin(),V.end(),[=](const Node &n2) { return n.name==n2.name; });

    if (it != V.end())
      return it-V.begin();

    V.push_back(n);
    return V.size()-1;
  }

  int findOrAddEdge(const Edge &e) {
    // digraph insert:
    auto it = std::find_if(
        E.begin(),E.end(),[=](const Edge &e2) { return e.from==e2.from && e.to==e2.to; });

    if (it != E.end())
      return it-E.begin();

    E.push_back(e);
    return E.size()-1;
  }
};

inline
Graph readDOT(std::string fileName) {
  // parser for the very specific dot layout Hamish's
  // app writes out....:

  Graph G;

  std::ifstream in(fileName);

  std::string line;
  while (std::getline(in,line)) {
    // parse edges:
    char v1[32], v2[32];
    int res = sscanf(line.c_str(),"%s -> %s", v1, v2);
    if (res && v1[0]=='v' && v2[0]=='v') {
      Node from, to;
      from.name = std::string(v1);
      to.name = std::string(v2);
      int i1 = G.findOrAddNode(from);
      int i2 = G.findOrAddNode(to);
      G.findOrAddEdge({i1,i2});
    }
    // parse values:
    if (line.find("rank = same") != std::string::npos) {
      auto splits = util::string_split(line,';');
      for (int i=0; i<splits.size(); ++i) {
        splits[i] = util::trim(splits[i]);
      }
      double value = std::stod(splits[1]);
      for (int i=2; i<splits.size(); ++i) {
        if (splits[i][0] == 'v') {
          Node tmp;
          tmp.name = splits[i];
          int nodeID = G.findOrAddNode(tmp);
          G.V[nodeID].value = value;
        }
      }
    }
  }

  // compute node fanIn/fanOut
  for (int i=0; i<G.E.size(); ++i) {
    Edge e = G.E[i];
    G.V[e.from].fanOut++;
    G.V[e.to].fanIn++;
  }

  return G;
}

static
double distance(const Graph &G1, const Graph &G2) {
  // O(n^2) compute graph affinity using custom similarity metric:
  std::vector<double> nodeDistances(G1.V.size(),DBL_MAX);
  //std::cout << G1.V.size() << ',' << G2.V.size() << '\n';
  double minValue=DBL_MAX,maxValue=-DBL_MAX;
  for (size_t i=0; i<G1.V.size(); ++i) {
    minValue = std::min(minValue,G1.V[i].value);
    maxValue = std::max(maxValue,G1.V[i].value);
  }
  for (size_t i=0; i<G2.V.size(); ++i) {
    minValue = std::min(minValue,G2.V[i].value);
    maxValue = std::max(maxValue,G2.V[i].value);
  }
  std::vector<double> normValues1(G1.V.size());
  for (size_t i=0; i<G1.V.size(); ++i) {
    normValues1[i] = (G1.V[i].value-minValue)/(maxValue-minValue);
  }
  std::vector<double> normValues2(G2.V.size());
  for (size_t i=0; i<G2.V.size(); ++i) {
    normValues2[i] = (G2.V[i].value-minValue)/(maxValue-minValue);
  }
  for (size_t i=0; i<G1.V.size(); ++i) {
    for (size_t j=0; j<G2.V.size(); ++j) {
      auto n1 = G1.V[i];
      auto n2 = G2.V[j];
      auto v1 = normValues1[i];
      auto v2 = normValues2[j];
      double s1 = std::abs(v1-v2);
      int    s2 = std::abs(n1.fanIn-n2.fanIn);
      int    s3 = std::abs(n1.fanOut-n2.fanOut);
      double w1=1/3.f;
      double w2=1/3.f;
      double w3=1/3.f;
      double dist = s1*w1+s2*w2+s3*w3;
      //std::cout << s1 << ',' << s2 << ',' << s3 << '\n';
      if (dist < nodeDistances[i]) {
        nodeDistances[i] = dist;
      }
    }
  }
  int cnt=0;
  double EPS=1e-2;
  for (size_t i=0; i<nodeDistances.size(); ++i) {
    if (nodeDistances[i]<EPS) cnt++;
  }
  return nodeDistances.empty()?DBL_MAX
    : 1.-cnt/(double)nodeDistances.size();
}

int main(int argc, char **argv) {
  if (argc < 3)
    return -1;

  Graph G[2];
  struct UserData {
    int ID;
  };

  auto func = [&](void *userData) {
    int ID = *(int *)userData;
    G[ID] = readDOT(argv[1+ID]);

    //std::cout << "G (" << argv[1+ID] << ")\n";
    //for (int i=0; i<G[ID].V.size(); ++i) {
    //  auto n = G[ID].V[i];
    //  std::cout << "node: " << i << ", name:" << n.name << ", value: "
    //      << n.value << ", fanIn: " << n.fanIn << ", fanOut: " << n.fanOut << '\n';
    //}
    //for (int i=0; i<G[ID].E.size(); ++i) {
    //  std::cout << G[ID].E[i].from << " -> " << G[ID].E[i].to << '\n';
    //}
    std::cout << distance(G[0],G[1]) << '\n';
  };

  int ID0=0, ID1=1;

  util::FileMonitor fm1(argv[1]);
  fm1.handlerFunc = func;
  fm1.userData = &ID0;

  util::FileMonitor fm2(argv[2]);
  fm2.handlerFunc = func;
  fm2.userData = &ID1;

  fm1.run(); fm2.run();

  // main loop:
  for (;;) {
    std::cout << "quit? [type q]: ";
    char c;
    std::cin >> c;
    if (c=='q') break;
  }

  // https://www.sciencedirect.com/science/article/pii/S0304397510004202
  // to string with bracketed pre-order encoding and then do string Levenshtein
  // Tree 1 / 2 3 \ 4 5 6 -> "1(2(4)(5))(3(6))"
}



