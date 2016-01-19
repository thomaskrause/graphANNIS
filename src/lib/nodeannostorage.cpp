/* 
 * File:   nodeannostorage.cpp
 * Author: thomas
 * 
 * Created on 14. Januar 2016, 13:53
 */

#include <annis/nodeannostorage.h>

#include <annis/stringstorage.h>

#include <cmath>
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/set.hpp>

using namespace annis;

NodeAnnoStorage::NodeAnnoStorage(StringStorage& strings)
: strings(strings)
{
}

bool NodeAnnoStorage::load(std::string dirPath)
{
  std::ifstream in;
  in.open(dirPath + "/nodeAnnotations.btree");
  nodeAnnotations.restore(in);
  in.close();

  in.open(dirPath + "/inverseNodeAnnotations.btree");
  inverseNodeAnnotations.restore(in);
  in.close();

  in.open(dirPath + "/nodeAnnoKeys.archive");
  boost::archive::binary_iarchive iaNodeAnnoKeys(in);
  iaNodeAnnoKeys >> nodeAnnoKeys;
  in.close();
}

bool NodeAnnoStorage::save(std::string dirPath)
{
  std::ofstream out;

  out.open(dirPath + "/nodeAnnotations.btree");
  nodeAnnotations.dump(out);
  out.close();

  out.open(dirPath + "/inverseNodeAnnotations.btree");
  inverseNodeAnnotations.dump(out);
  out.close();

  out.open(dirPath + "/nodeAnnoKeys.archive");
  boost::archive::binary_oarchive oaNodeAnnoKeys(out);
  oaNodeAnnoKeys << nodeAnnoKeys;
  out.close();
}

void NodeAnnoStorage::clear()
{
  nodeAnnotations.clear();
  inverseNodeAnnotations.clear();
}

void NodeAnnoStorage::calculateStatistics()
{
  
  const int maxHistogramBuckets = 100;
  const int maxSampledAnnotations = 1000;
  
  histogramBounds.clear();
  
  // collect statistics for each annotation key separatly
  std::map<AnnotationKey, std::vector<std::string>> globalValueList;
  for(const auto& annoKey : nodeAnnoKeys)
  {
    histogramBounds[annoKey] = std::vector<std::string>();
    globalValueList[annoKey] = std::vector<std::string>();
  }
  
  // sample about X annotations in total
  auto n = (int) nodeAnnotations.size();
  if(n > 0)
  {
    const int stepSize = n / maxSampledAnnotations;
    for(auto it = nodeAnnotations.begin(); 
      it != nodeAnnotations.end(); std::advance(it, stepSize))
    {
      const NodeAnnotationKey& nodeKey = it.key();
      AnnotationKey annoKey = {nodeKey.anno_name, nodeKey.anno_ns};
      auto& valueList = globalValueList[annoKey];
      std::string val = strings.str(it.data());
      valueList.push_back(val);
    }
  }
  
  
  // create uniformly distributed histogram bounds for each node annotation key 
  for(auto it=globalValueList.begin(); it != globalValueList.end(); it++)
  {
    auto& values = it->second;
    
    std::sort(values.begin(), values.end());
    
    int numValues = values.size();
    
    int numHist = maxHistogramBuckets;
    if(numValues < numHist)
    {
      numHist = numValues;
    }
    
    if(numHist >= 2)
    {
      auto& h = histogramBounds[it->first];
      h.resize(numHist);

      int delta = (numValues-1) / (numHist -1);
      int deltaFraction = (numValues -1) % (numHist - 1);

      int pos = 0;
      int posFraction = 0;
      for(int i=0; i < numHist; i++)
      {
        h[i] = values[pos];
        pos += delta;
        posFraction += deltaFraction;
        
        if(posFraction >= (numHist - 1))
        {
          pos++;
          posFraction -= (numHist - 1);
        }
      }
    }
  }
}


size_t NodeAnnoStorage::guessCount(const std::string& ns, const std::string& name, const std::string& val)
{
  auto nameID = strings.findID(name);
  if(nameID.first)
  {
    auto nsID = strings.findID(ns);
    if(nsID.first)
    {
      static const char minChar = std::numeric_limits<char>::min();
      std::string exclusiveUpper = val + minChar;
      return guessCount(boost::optional<std::uint32_t>(nsID.second), nameID.second, 
        val, exclusiveUpper);
    }
  }
  
  
  // if none of the conditions above is valid the annotation key does not exist
  return 0;
}

size_t NodeAnnoStorage::guessCount(const std::string& name, const std::string& val)
{
  auto nameID = strings.findID(name);
  if(nameID.first)
  {
    static const char minChar = std::numeric_limits<char>::min();
    std::string exclusiveUpper = val + minChar;
    return guessCount(boost::optional<std::uint32_t>(), nameID.second, val, exclusiveUpper);
  }
  return 0;
}


size_t NodeAnnoStorage::guessCount(boost::optional<std::uint32_t> nsID, 
  std::uint32_t nameID, 
  const std::string& lowerVal, const std::string& upperVal)
{
  std::list<AnnotationKey> keys;
  if(nsID)
  {
    keys.push_back({nameID, *nsID});
  }
  else
  {
    // find all complete keys which have the given name
    auto itKeyUpper = nodeAnnoKeys.upper_bound({nameID, std::numeric_limits<std::uint32_t>::max()});
    for(auto itKeys = nodeAnnoKeys.lower_bound({nameID, 0}); itKeys != itKeyUpper; itKeys++)
    {
      keys.push_back(*itKeys);
    }
  }
  
  size_t sumHistogramBuckets = 0;
  size_t countMatches = 0;
  // guess for each annotation fully qualified key and return the sum of all guesses
  for(const auto& key : keys)
  {
    auto itHisto = histogramBounds.find(key);
    if(itHisto != histogramBounds.end())
    {
      // find the range in which the value is contained
      const auto& histo = itHisto->second;
      
      sumHistogramBuckets += histo.size();
      
      for(auto it=std::lower_bound(histo.begin(), histo.end(), lowerVal); 
        it != histo.end() && *it < upperVal; it++)
      {
        countMatches++;
      }
    }
  }
  
  double selectivity = ((double) countMatches) / ((double) sumHistogramBuckets);
  
  return std::round(selectivity * ((double) nodeAnnotations.size()));
}



NodeAnnoStorage::~NodeAnnoStorage()
{
}

