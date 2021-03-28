//---------------------------------------------------------------------------
#ifndef ALGRAPH_H
#define ALGRAPH_H
//---------------------------------------------------------------------------
#include <vector>

struct DijkstraInfo
{
  unsigned cost;
  std::vector<unsigned> path;
};

struct AdjacencyInfo
{
  unsigned id;
  unsigned weight;
};

typedef std::vector<std::vector<AdjacencyInfo>> ALIST;

class ALGraph
{
public:
  ALGraph(unsigned size);
  ~ALGraph(void);
  void AddDEdge(unsigned source, unsigned destination, unsigned weight);
  void AddUEdge(unsigned node1, unsigned node2, unsigned weight);

  std::vector<DijkstraInfo> Dijkstra(unsigned start_node) const;
  ALIST GetAList(void) const;

private:
  // An EXAMPLE of some other classes you may want to create and
  // implement in ALGraph.cpp
  struct GNode
  {

    unsigned id;
    unsigned prev;
    unsigned cost;
    bool evaluated;
    bool operator<(const GNode &rhs) const;
    bool operator==(const unsigned _id) const;
  };
  class GEdge;
  struct AdjInfo
  {
    //GNode *node;
    unsigned adj_node_id;
    unsigned weight;
    unsigned cost;
    //AdjInfo();
    bool operator<(const AdjInfo &rhs) const;
    bool operator>(const AdjInfo &rhs) const;
  };

  // Other private fields and methods
  const unsigned INFINITY_ = static_cast<unsigned>(-1);
  using ADJList = std::vector<std::vector<AdjInfo>>;
  ADJList adjlist_;
  unsigned size_;
};

#endif
