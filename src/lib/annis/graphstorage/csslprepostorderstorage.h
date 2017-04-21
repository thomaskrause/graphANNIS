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

#pragma once

#include <google/btree_map.h>
#include <annis/graphstorage/graphstorage.h>
#include <annis/types.h>
#include <annis/annostorage.h>
#include "prepostorderstorage.h"

#include <boost/optional.hpp>

#include <set>
#include <vector>
#include <stack>

extern "C" {
  #include <cssl/skiplist.h>
}


namespace annis
{

class CSSLPrePostOrderStorage : public ReadableGraphStorage
{
public:

  using OrderEntry = PrePost<uint32_t, int32_t>;
  using NStack = std::stack<NodeStackEntry<uint32_t, int32_t>, std::list<NodeStackEntry<uint32_t, int32_t> > >;

  using PreIdxValueT = std::pair<const nodeid_t, OrderEntry>;

  class CSSLPrePostIterator : public EdgeIterator
  {
  public:
    struct CSSLSearchRange
    {
      size_t startIdx;
      size_t endIdx;
      uint32_t maximumPost;
      int32_t startLevel;
    };

  public:
    CSSLPrePostIterator(const CSSLPrePostOrderStorage& storage,
                        nodeid_t sourceNode,
                        unsigned int minDistance,
                        unsigned int maxDistance);
    virtual std::pair<bool, nodeid_t> next() override;
    virtual void reset() override;

    virtual ~CSSLPrePostIterator();
  private:

    const CSSLPrePostOrderStorage& storage;
    const nodeid_t sourceNode;
    const unsigned int minDistance;
    const unsigned int maxDistance;

    std::stack<CSSLSearchRange, std::list<CSSLSearchRange>> searchRanges;

    const bool uniqueCheck;
    btree::btree_set<nodeid_t> visited;

    boost::optional<size_t> currentNodeIdx;
  private:
    void init();
  };

public:
  CSSLPrePostOrderStorage();

  virtual ~CSSLPrePostOrderStorage();

  template<class Archive>
  void serialize(Archive & archive)
  {
    // TODO: archive members
    archive(cereal::base_class<ReadableGraphStorage>(this));
  }

  virtual void copy(const DB& db, const ReadableGraphStorage& orig) override;

  virtual void clear() override;

  virtual std::vector<Annotation> getEdgeAnnotations(const Edge& edge) const override
  {
    return edgeAnno.getAnnotations(edge);
  }

  virtual bool isConnected(const Edge& edge, unsigned int minDistance = 1, unsigned int maxDistance = 1) const override;
  virtual int distance(const Edge &edge) const override;
  virtual std::unique_ptr<EdgeIterator> findConnected(nodeid_t sourceNode,
                                           unsigned int minDistance = 1,
                                           unsigned int maxDistance = 1) const override;

  virtual std::vector<nodeid_t> getOutgoingEdges(nodeid_t node) const override
  {

    std::vector<nodeid_t> result;
    result.reserve(10);

    auto connectedIt = findConnected(node, 1, 1);
    for(auto c=connectedIt->next(); c.first; c=connectedIt->next())
    {
      result.push_back(c.second);
    }

    return result;
  }

  virtual size_t numberOfEdges() const override
  {
    return node2order.size();
  }
  virtual size_t numberOfEdgeAnnotations() const override
  {
    return edgeAnno.numberOfAnnotations();
  }

  virtual const BTreeMultiAnnoStorage<Edge>& getAnnoStorage() const override
  {
    return edgeAnno;
  }

  virtual size_t estimateMemorySize() override
  {
    // TODO: add other members
    return
        edgeAnno.estimateMemorySize()
        //+ size_estimation::element_size(node2order)
        + sizeof(CSSLPrePostOrderStorage);
  }

private:

  btree::btree_multimap<nodeid_t, OrderEntry> node2order;
  /** a vector sorted by the order of the node (like a flat map) */
  std::vector<std::pair<nodeid_t, OrderEntry>> order2node;

  SkipList* preIdx;
  BTreeMultiAnnoStorage<Edge> edgeAnno;

private:

  static void freeSkiplist(SkipList* preIdx);

};

} // end namespace annis


#include <cereal/archives/binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>


CEREAL_REGISTER_TYPE(annis::CSSLPrePostOrderStorage)

