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

#include "identicalcoverage.h"
#include <annis/wrapper.h>                    // for SingleElementWrapper
#include <utility>                            // for operator==, pair
#include <vector>                             // for vector
#include "annis/db.h"                         // for DB
#include "annis/graphstorage/graphstorage.h"  // for ReadableGraphStorage
#include "annis/iterators.h"                  // for AnnoIt
#include "annis/operators/operator.h"         // for Operator
#include "annis/util/helper.h"                // for TokenHelper
#include <annis/types.h>

using namespace annis;

IdenticalCoverage::IdenticalCoverage(const DB &db, DB::GetGSFuncT getGraphStorageFunc)
: tokHelper(getGraphStorageFunc, db),
  anyNodeAnno(Init::initAnnotation(db.getNodeNameStringID(), 0, db.getNamespaceStringID()))
{
  gsOrder = getGraphStorageFunc(ComponentType::ORDERING, annis_ns, "");
  gsLeftToken = getGraphStorageFunc(ComponentType::LEFT_TOKEN, annis_ns, "");
  gsRightToken = getGraphStorageFunc(ComponentType::RIGHT_TOKEN, annis_ns, "");
  gsCoverage = getGraphStorageFunc(ComponentType::COVERAGE, annis_ns, "");
}

bool IdenticalCoverage::filter(const Match& lhs, const Match& rhs)
{
  auto lhsTokRange = tokHelper.leftRightTokenForNode(lhs.node);
  auto rhsTokRange = tokHelper.leftRightTokenForNode(rhs.node);
  return lhsTokRange == rhsTokRange;
}

std::unique_ptr<AnnoIt> IdenticalCoverage::retrieveMatches(const Match& lhs)
{ 
  nodeid_t leftToken;
  nodeid_t rightToken;
  if(tokHelper.isToken(lhs.node))
  {
    // is token
    leftToken = lhs.node;
    rightToken = lhs.node;
  }
  else
  {
    leftToken = gsLeftToken->getOutgoingEdges(lhs.node)[0];
    rightToken = gsRightToken->getOutgoingEdges(lhs.node)[0];
  }
  
  // find each non-token node that is left-aligned with the left token and right aligned with the right token
  auto leftAligned = gsLeftToken->getOutgoingEdges(leftToken);
  bool includeToken = leftToken == rightToken;
  
  // check for shortcuts where we don't need to return a complicated ListWrapper
  if(includeToken && leftAligned.empty())
  {
    // we only need to return a single match
    return std::unique_ptr<SingleElementWrapper>(new SingleElementWrapper(Init::initMatch(anyNodeAnno, leftToken)));
  }
  else if(!includeToken &&  leftAligned.size() == 1)
  {
    // check if also right aligned
    auto candidateRight = gsRightToken->getOutgoingEdges(leftAligned[0])[0];
    if(candidateRight == rightToken)
    {
      return std::unique_ptr<SingleElementWrapper>(new SingleElementWrapper(Init::initMatch(anyNodeAnno, leftAligned[0])));
    }
    else
    {
      // empty result
      return std::unique_ptr<AnnoIt>();
    }
  } // end shortcuts
  
  // use the ListWrapper as default case for matches with more than one result
  std::unique_ptr<ListWrapper> w = std::unique_ptr<ListWrapper>(new ListWrapper());
  
  // add the connected token itself as a match the span covers only one token
  if(includeToken)
  {
    w->addMatch({leftToken, anyNodeAnno});
  }
  
  for(const auto& candidate : leftAligned)
  {
    // check if also right aligned
    std::vector<nodeid_t>  outEdges = gsRightToken->getOutgoingEdges(candidate);
    if(!outEdges.empty())
    {
      auto candidateRight = outEdges[0];
      if(candidateRight == rightToken)
      {
        w->addMatch({candidate, anyNodeAnno});
      }
    }
  }

  return std::move(w);
}

double IdenticalCoverage::selectivity() 
{
  if(gsOrder == nullptr || gsCoverage == nullptr)
  {
    return Operator::selectivity();
  }
  auto statsOrder = gsOrder->getStatistics();

  double numOfToken = statsOrder.nodes;


  // Assume two nodes have same identical coverage if they have the same
  // left covered token and the same length (right covered token is not independent
  // of the left one, this is why we should use length).
  // The probability for the same length is taken is assumed to be 1.0, histograms
  // of the distribution would help here.

  return 1.0 / numOfToken;


}


IdenticalCoverage::~IdenticalCoverage()
{
}

