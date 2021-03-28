#include "ALGraph.h"
#include <algorithm>
#include <queue>
ALGraph::ALGraph(unsigned size) : size_{size}
{
  for (unsigned i = 0; i < size_; ++i)
    adjlist_.push_back(std::vector<AdjInfo>());
}
ALGraph::~ALGraph(void)
{
}

void ALGraph::AddDEdge(unsigned source, unsigned destination, unsigned weight)
{
  auto id = source - 1;
  adjlist_[id].push_back(AdjInfo{destination, weight, 0});
  std::sort(adjlist_[id].begin(), adjlist_[id].end());
}

void ALGraph::AddUEdge(unsigned node1, unsigned node2, unsigned weight)
{
  AddDEdge(node1, node2, weight);
  AddDEdge(node2, node1, weight);
}

std::vector<DijkstraInfo> ALGraph::Dijkstra(unsigned start_node) const
{
  std::vector<GNode> queue;
  for (unsigned i = 0; i < adjlist_.size(); ++i)
  {
    unsigned id = i +1;
    unsigned cost = 0;
    if (id != start_node)
      cost = INFINITY_;
    queue.push_back(GNode{id, id, cost, false});
  }
  std::make_heap(queue.begin(), queue.end());

  std::vector<GNode> completed_nodes(size_);

  while (!queue.empty())
  {
    std::pop_heap(queue.begin(), queue.end());
    auto node = queue.back();
    queue.pop_back();
    
    node.evaluated = true;
    completed_nodes[node.id - 1] = node;

    for (auto adj_node : adjlist_[node.id - 1])
    {
      auto adj_node_from_queue = std::find(queue.begin(), queue.end(), adj_node.adj_node_id);
      auto new_cost = node.cost + adj_node.cost;
      if (!adj_node_from_queue->evaluated && new_cost < adj_node_from_queue->cost)
      {
        adj_node_from_queue->cost = new_cost;
        adj_node_from_queue->prev = node.id;
        std::make_heap(queue.begin(), queue.end());
      }
    }
  }

  std::vector<DijkstraInfo> info(size_);
  for (auto node : completed_nodes)
  {
    auto prev_node = node;
    auto prev_id = prev_node.id;

    std::vector<unsigned> path;
    path.push_back(prev_id);

    while (prev_id != start_node)
    {
      prev_node = completed_nodes[prev_id - 1];
      prev_id = prev_node.prev;
      path.push_back(prev_id);
    }
    std::reverse(path.begin(), path.end());
    //info[node.id - 1] = DijkstraInfo{node.cost, path};
  }

  return info;
}

ALIST ALGraph::GetAList(void) const{
    // copy into driver communicable structures
    ALIST alist;
    for (auto ilist: adjlist_) {
        alist.push_back(std::vector<AdjacencyInfo>());
        for (auto item: ilist)
            alist.back().push_back(AdjacencyInfo{item.adj_node_id, item.weight});
    }
    return alist;
}

bool ALGraph::GNode::operator<(const GNode &rhs) const
{
  return cost > rhs.cost;
}

bool ALGraph::GNode::operator==(const unsigned _id) const
{
  return id > _id;
}

bool ALGraph::AdjInfo::operator<(const AdjInfo &rhs) const
{
  return weight < rhs.weight;
}

bool ALGraph::AdjInfo::operator>(const AdjInfo &rhs) const
{
  return weight > rhs.weight;
}