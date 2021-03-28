/******************************************************************************/
/*!
\file   ALGraph.h
\author Ng Tian Kiat
\par    email: tiankiat.ng\@digipen.edu
\par    Course: CS280
\par    Assignment 4
\date   28 March 2021
\brief  
  This file contains the declarations for the ALGraph.
*/
/******************************************************************************/
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
    bool evaluated;
    DijkstraInfo info;
    bool operator<(const GNode &rhs) const;
  };
  class GEdge;
  struct AdjInfo
  {
    //GNode *node;
    unsigned id;
    unsigned weight;
    unsigned cost;
    //AdjInfo();
  };

  // Other private fields and methods
  const unsigned INFINITY_ = static_cast<unsigned>(-1);
  ALIST list_;
};

#endif
