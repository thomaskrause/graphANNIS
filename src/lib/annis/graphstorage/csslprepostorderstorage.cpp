/*
   Copyright 2017 Thomas Krause <thomaskrause@posteo.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "csslprepostorderstorage.h"
#include <annis/db.h>
#include <annis/annosearch/exactannokeysearch.h>

#include <algorithm>
#include <limits>

using namespace annis;

CSSLPrePostOrderStorage::CSSLPrePostOrderStorage()
  : preIdx(nullptr)
{
}

CSSLPrePostOrderStorage::~CSSLPrePostOrderStorage()
{
  freeSkiplist(preIdx);
  preIdx = nullptr;
}

void CSSLPrePostOrderStorage::copy(const annis::DB &db, const annis::ReadableGraphStorage &orig)
{
  clear();

  // create a skiplist with an adjusted size
  double expectedNumOfOrders = orig.numberOfEdges();
  if(orig.getStatistics().valid && !orig.getStatistics().cyclic)
  {
    expectedNumOfOrders = expectedNumOfOrders * orig.getStatistics().dfsVisitRatio;
  }
  // The highest lane should have a size of 16, thus calculate the number of skips we need in the highest line to reach
  // this size.
  double highestLaneSkip = (expectedNumOfOrders/16.0);
  double level = std::ceil(log(highestLaneSkip) / log(5));
  if(level <= 1.0)
  {
    // minimum is 2 levels
    level = 2.0;
  }
  //std::cout << "Using level " << level << " for skiplist" << std::endl;
  uint8_t levelCasted = std::numeric_limits<uint8_t>::max();
  if(level <levelCasted)
  {
    levelCasted = static_cast<uint8_t>(level);
  }
  preIdx = createSkipList(levelCasted, 5);

  // find all roots of the component
  std::set<nodeid_t> roots;
  ExactAnnoKeySearch nodes(db, annis_ns, annis_node_name);
  Match match;
  // first add all nodes that are a source of an edge as possible roots
  while(nodes.next(match))
  {
    nodeid_t n = match.node;
    // insert all nodes to the root candidate list which are part of this component
    if(!orig.getOutgoingEdges(n).empty())
    {
      roots.insert(n);
    }
  }

  nodes.reset();
  while(nodes.next(match))
  {
    nodeid_t source = match.node;

    std::vector<nodeid_t> outEdges = orig.getOutgoingEdges(source);
    for(auto target : outEdges)
    {
      Edge e = {source, target};

      // remove the nodes that have an incoming edge from the root list
      roots.erase(target);

      std::vector<Annotation> edgeAnnos = orig.getEdgeAnnotations(e);
      for(auto a : edgeAnnos)
      {
        edgeAnno.addAnnotation(e, a);
      }
    }
  }

  // Don't start with order 0 since the CSSL will always have a first node with key 0 and an empty value
  uint32_t currentOrder = 1;

  // traverse the graph for each sub-component
  for(const auto& startNode : roots)
  {
    unsigned int lastDistance = 0;

    NStack nodeStack;

    nodeStack.push({startNode, {currentOrder++, 0, 0}});

    CycleSafeDFS dfs(orig, startNode, 1, uintmax);
    for(DFSIteratorResult step = dfs.nextDFS(); step.found;
          step = dfs.nextDFS())
    {
      if(step.distance > lastDistance)
      {
        // first visited, set pre-order
        nodeStack.push({step.node, {currentOrder++, 0, static_cast<int32_t>(step.distance)}});
      }
      else
      {
        // Neighbour node, the last subtree was iterated completly, thus the last node
        // can be assigned a post-order.
        // The parent node must be at the top of the node stack,
        // thus exit every node which comes after the parent node.
        // Distance starts with 0 but the stack size starts with 1.
        while(nodeStack.size() > step.distance)
        {
          auto& entry = nodeStack.top();
          entry.order.post = currentOrder++;
          node2order.insert({entry.id, entry.order});

          nodeStack.pop();
        }
        // new node
        nodeStack.push({step.node, {currentOrder++, 0, static_cast<int32_t>(step.distance)}});
      }
      lastDistance = step.distance;
    } // end for each DFS step

    while(!nodeStack.empty())
    {
      auto& entry = nodeStack.top();
      entry.order.post = currentOrder++;
      node2order.insert({entry.id, entry.order});

      nodeStack.pop();
    }

  } // end for each root

  // add all node IDs to vector
  order2node.reserve(node2order.size());
  for(const auto& e : node2order)
  {
    order2node.push_back(e);
  }

  // sort the node ID vector by the pre-order
  std::sort(order2node.begin(), order2node.end(),
            [](std::pair<nodeid_t, OrderEntry> a, std::pair<nodeid_t, OrderEntry> b)
  {
    return a.second.pre < b.second.pre;
  });

  // make sure to insert the entries to the preIdx in correct order
  for(auto& e : order2node)
  {
    insertElement(preIdx, e.second.pre);
  }

  stat = orig.getStatistics();
  edgeAnno.calculateStatistics(db.strings);
}

void CSSLPrePostOrderStorage::freeSkiplist(SkipList* slist)
{
  if(slist != nullptr)
  {

    // free all inserted nodes
    {
      DataNode* n = slist->head;
      while(n != nullptr && n != slist->tail)
      {
        DataNode* old = n;
        n = n->next;

        free(old);
      }
    }
    // always delete the tail
    free(slist->tail);

    // delete the proxy nodes
    for (uint32_t i = 0; i < slist->items_per_level[0]; i++)
    {
      if(slist->flane_pointers[i] != nullptr)
      {
        free(slist->flane_pointers[i]);
      }
    }

    free(slist->items_per_level);
    free(slist->flane_items);
    free(slist->starts_of_flanes);
    free(slist->flanes);
    free(slist->flane_pointers);

    free(slist);
  }
}

void CSSLPrePostOrderStorage::clear()
{
  node2order.clear();
  order2node.clear();

  freeSkiplist(preIdx);
  preIdx = nullptr;

  edgeAnno.clear();
}

bool CSSLPrePostOrderStorage::isConnected(const Edge &edge, unsigned int minDistance, unsigned int maxDistance) const
{
  const auto itSourceBegin = node2order.lower_bound(edge.source);
  const auto itSourceEnd = node2order.upper_bound(edge.source);
  const auto itTargetRange = node2order.equal_range(edge.target);

  for(auto itSource=itSourceBegin; itSource != itSourceEnd; itSource++)
  {
    const std::pair<nodeid_t, OrderEntry>& sourceEntry = *itSource;
    for(auto itTarget=itTargetRange.first; itTarget != itTargetRange.second; itTarget++)
    {
      const std::pair<nodeid_t, OrderEntry>& targetEntry = *itTarget;

      if(sourceEntry.second.pre <= targetEntry.second.pre
         && targetEntry.second.post <= sourceEntry.second.post)
      {
        // check the level
        unsigned int diffLevel = std::abs(targetEntry.second.level - sourceEntry.second.level);
        if(minDistance <= diffLevel && diffLevel <= maxDistance)
        {
          return true;
        }
      }
    }
  }


  return false;
}

int CSSLPrePostOrderStorage::distance(const Edge &edge) const
{
  if(edge.source == edge.target)
  {
    return 0;
  }

  const auto itSourceBegin = node2order.lower_bound(edge.source);
  const auto itSourceEnd = node2order.upper_bound(edge.source);
  const auto itTargetRange = node2order.equal_range(edge.target);

  bool wasFound = false;
  int32_t minLevel = std::numeric_limits<int32_t>::max();

  for(auto itSource=itSourceBegin; itSource != itSourceEnd; itSource++)
  {
    const std::pair<nodeid_t, OrderEntry>& sourceEntry = *itSource;
    for(auto itTarget=itTargetRange.first; itTarget != itTargetRange.second; itTarget++)
    {
      const std::pair<nodeid_t, OrderEntry>& targetEntry = *itTarget;

      if(sourceEntry.second.pre <= targetEntry.second.pre
         && targetEntry.second.post <= sourceEntry.second.post)
      {
        // check the level
        int32_t diffLevel = targetEntry.second.level - sourceEntry.second.level;
        if(diffLevel >= 0)
        {
          wasFound = true;
          minLevel = std::min(minLevel, diffLevel);
        }
      }
    }
  }

  if(wasFound)
  {
    return minLevel;
  }


  return -1;
}

std::unique_ptr<EdgeIterator> CSSLPrePostOrderStorage::findConnected(nodeid_t sourceNode, unsigned int minDistance, unsigned int maxDistance) const
{
  return std::unique_ptr<EdgeIterator>(new CSSLPrePostIterator(*this, sourceNode, minDistance, maxDistance));
}


CSSLPrePostOrderStorage::CSSLPrePostIterator::CSSLPrePostIterator(
    const CSSLPrePostOrderStorage &storage, nodeid_t sourceNode, unsigned int minDistance, unsigned int maxDistance)
  : storage(storage), sourceNode(sourceNode), minDistance(minDistance), maxDistance(maxDistance),
    uniqueCheck(true),
//    uniqueCheck(storage.getStatistics().valid && storage.getStatistics().rootedTree && storage.getStatistics().dfsVisitRatio <= 1.0 ? false : true),
    currentNodeIdx(boost::none)
{
  init();
}

std::pair<bool, nodeid_t> CSSLPrePostOrderStorage::CSSLPrePostIterator::next()
{
  std::pair<bool, nodeid_t> result(0, false);

  while(!searchRanges.empty())
  {
    const CSSLSearchRange& r = searchRanges.top();

    while(currentNodeIdx
          && *currentNodeIdx < r.endIdx
          && *currentNodeIdx < storage.order2node.size()
          && storage.order2node[*currentNodeIdx].second.pre < r.maximumPost)
    {
      const OrderEntry& currentOrder = storage.order2node[*currentNodeIdx].second;

      unsigned int diffLevel = std::abs(currentOrder.level - r.startLevel);

      const nodeid_t& currentNodeID = storage.order2node[*currentNodeIdx].first;
      // check post order and level as well
      if(currentOrder.post <= r.maximumPost && minDistance <= diffLevel && diffLevel <= maxDistance
         && (!uniqueCheck || visited.find(currentNodeID) == visited.end()))
      {
        // success
        result.first = true;
        result.second = currentNodeID;

        if(uniqueCheck)
        {
          visited.insert(result.second);
        }

        (*currentNodeIdx)++;
        return result;
      }
      else if(currentOrder.pre < r.maximumPost)
      {
        // proceed with the next entry in the range
        (*currentNodeIdx)++;
      }
      else
      {
        // abort searching in this range
        break;
      }
    } // end while range not finished yet

    // this range is finished, try next one
    searchRanges.pop();
    if(!searchRanges.empty())
    {
      currentNodeIdx = searchRanges.top().startIdx;
    }
  }

  return result;
}

void CSSLPrePostOrderStorage::CSSLPrePostIterator::reset()
{
  while(!searchRanges.empty())
  {
    searchRanges.pop();
  }
  visited.clear();
  currentNodeIdx = boost::none;
  init();
}

CSSLPrePostOrderStorage::CSSLPrePostIterator::~CSSLPrePostIterator()
{
}

void CSSLPrePostOrderStorage::CSSLPrePostIterator::init()
{
  // get pre/post orders of the source node
  auto subComponentBegin = storage.node2order.lower_bound(sourceNode);

  for(auto it=subComponentBegin; it != storage.node2order.end() && it->first == sourceNode; it++)
  {
    const OrderEntry& order = it->second;

    // find all pre-order entry that are between the source nodes pre and post order
    RangeSearchResult r = searchRange(storage.preIdx, order.pre, order.post);
    if(r.count > 0)
    {
      searchRanges.push({r.startIdx, r.endIdx, order.post, order.level});
    }
  }
  if(!searchRanges.empty())
  {
    currentNodeIdx = searchRanges.top().startIdx;
  }
}
