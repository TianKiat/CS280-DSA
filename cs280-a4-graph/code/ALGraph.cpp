/******************************************************************************/
/*!
\file   ALGraph.cpp
\author Ng Tian Kiat
\par    email: tiankiat.ng\@digipen.edu
\par    Course: CS280
\par    Assignment 4
\date   28 March 2021
\brief  
  This file contains the implementations for the ALGraph.
*/
/******************************************************************************/
#include "ALGraph.h"
#include <algorithm>
#include <queue>

/******************************************************************************/
/*!
\brief
  This is the constructor for a ALGraph.
\param size, the number of nodes in the graph.
*/
/******************************************************************************/
ALGraph::ALGraph(unsigned size)
{
  list_.reserve(size);
  for (size_t i = 0; i < size; ++i)
    list_.push_back(ALIST::value_type());
}

/******************************************************************************/
/*!
\brief
  This is the destructor for a ALGraph.
*/
/******************************************************************************/
ALGraph::~ALGraph(void)
{
}

/******************************************************************************/
/*!
\brief
  This is the function adds a directed edge from a source node to destination node.
\param source, the source node.
\param destination, the destination node.
\param weight, the weight of the directed edge.
*/
/******************************************************************************/
void ALGraph::AddDEdge(unsigned source, unsigned destination, unsigned weight)
{
  unsigned id = source - 1;
  AdjacencyInfo info{destination, weight};
  auto &adj_list = list_.at(id);

  for (auto i = adj_list.begin(); i != adj_list.end(); ++i)
  {
    if (info.weight < i->weight)
    {
      adj_list.insert(i, info);
      return;
    }
    else if (info.weight == i->weight && info.id < i->id)
    {
      adj_list.insert(i, info);
      return;
    }
  }
  adj_list.push_back(info);
}

/******************************************************************************/
/*!
\brief
  This is the function adds a undirected edge connecting 2 nodes.
\param node1, the first node.
\param node2, the second node.
\param weight, the weight of the undirected edge.
*/
/******************************************************************************/
void ALGraph::AddUEdge(unsigned node1, unsigned node2, unsigned weight)
{
  AddDEdge(node1, node2, weight);
  AddDEdge(node2, node1, weight);
}

/******************************************************************************/
/*!
\brief
  This is the function performs Djikstra's shortest path algorithm on a graph,
  given a source node. It returns a driver parsable object, that contains the
  shortest path to any node on the graph from a starting node.
\param starting_node, the source node for the algorithm.
\return an object that contains the shortest path to every node in the graph,
 from the starting node.
*/
/******************************************************************************/
std::vector<DijkstraInfo> ALGraph::Dijkstra(unsigned start_node) const
{
  auto GetAdjList = [=](unsigned i) { return list_.at(i - 1); };
  auto comp = [](const GNode *lhs, const GNode *rhs) -> bool { return *lhs < *rhs; };

  std::priority_queue<GNode *, std::vector<GNode *>, decltype(comp)> pq(comp);
  std::vector<DijkstraInfo> result;
  std::vector<GNode> nodes;

  result.reserve(list_.size());
  nodes.reserve(list_.size());

  //initialize all nodes in graph
  for (unsigned i = 1; i < list_.size() + 1; ++i)
  {
    GNode node;
    if (i != start_node)
    {
      node.evaluated = false;
      node.info.cost = INFINITY_;
    }
    else
    {
      node.evaluated = true;
      node.info.cost = 0;
      node.info.path.push_back(start_node);
    }

    node.id = i;
    nodes.push_back(node);
  }

  //push all nodes into queue with updated edge costs.
  const auto &adj_nodes = GetAdjList(start_node);

  for (unsigned i = 0; i < adj_nodes.size(); ++i)
  {
    const auto &info = adj_nodes.at(i);
    auto &node = nodes[info.id - 1];
    node.info.cost = info.weight;

    pq.push(&node);
    node.info.path.push_back(start_node);
  }

  //go through the priority queue and evaluate all nodes.
  while (!pq.empty())
  {
    GNode *v = pq.top();
    pq.pop();

    v->evaluated = true;
    v->info.path.push_back(v->id);

    const auto &adj_list = GetAdjList(v->id);

    for (unsigned i = 0; i < adj_list.size(); ++i)
    {
      const auto &info = adj_list.at(i);
      auto &u = nodes.at(info.id - 1);

      unsigned new_cost = info.weight + v->info.cost;

      if (new_cost < u.info.cost)
      {
        u.info.cost = new_cost;
        u.info.path = v->info.path;

        if (!v->evaluated)
          u.info.path.push_back(v->id);

        pq.push(&u);
      }
    }
  }

  for (unsigned i = 0; i < nodes.size(); ++i)
  {
    result.push_back(nodes.at(i).info);
  }
  return result;
}

/******************************************************************************/
/*!
\brief
  This is a getter for the Adjacency list of the graph.
\return ALIST, a adjency list of all nodes in the graph.
*/
/******************************************************************************/
ALIST ALGraph::GetAList(void) const
{
  return list_;
}

/******************************************************************************/
/*!
\brief
  This function defines the less than operator for the GNode struct.
  It returns true if the this->cost is lesser than rhs.cost.
\param rhs, the node to compare against.
\return true if the this->cost is lesser than rhs.cost.
*/
/******************************************************************************/
bool ALGraph::GNode::operator<(const GNode &rhs) const
{
  return info.cost < rhs.info.cost;
}