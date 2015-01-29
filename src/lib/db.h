#ifndef ANNISDB_H
#define ANNISDB_H

#include <string>
#include <stx/btree_map>
#include <stx/btree_multimap>
#include <stx/btree_set>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <list>

#include "types.h"
#include "comparefunctions.h"
#include "edgedb.h"
#include "stringstorage.h"
#include "graphstorageregistry.h"

namespace annis
{
class DB
{
  friend class ExactAnnoSearch;
  friend class ExactAnnoKeySearch;
  friend class RegexAnnoSearch;

  typedef std::map<Component, ReadableGraphStorage*>::const_iterator EdgeDBIt;
public:
  DB(bool useSpecializedEdgeDB = true);

  bool loadRelANNIS(std::string dirPath);
  bool load(std::string dirPath);
  bool save(std::string dirPath);

  bool hasNode(nodeid_t id);
  inline std::list<Annotation> getNodeAnnotationsByID(const nodeid_t &id) const
  {
    typedef stx::btree_map<NodeAnnotationKey, uint32_t>::const_iterator AnnoIt;

    NodeAnnotationKey lowerAnno = {id, 0, 0};
    NodeAnnotationKey upperAnno = {id, uintmax, uintmax};

    std::list<Annotation> result;
    std::pair<AnnoIt,AnnoIt> itRange = {
      nodeAnnotations.lower_bound(lowerAnno),
      nodeAnnotations.upper_bound(upperAnno)
    };

    for(AnnoIt it = itRange.first;
        it != itRange.second; it++)
    {
      const NodeAnnotationKey& key = it->first;
      result.push_back({key.anno_name, key.anno_ns, it->second});
    }

    return result;
  }

  inline std::string getNodeName(const nodeid_t &id) const
  {
    std::string result = "";

    std::pair<bool, Annotation> anno = getNodeAnnotation(id, annis_ns, annis_node_name);
    if(anno.first)
    {
      result = strings.str(anno.second.val);
    }
    return result;
  }

  inline std::string getNodeDocument(const nodeid_t &id) const
  {
    std::string result = "";

    std::pair<bool, Annotation> anno = getNodeAnnotation(id, annis_ns, "document");
    if(anno.first)
    {
      result = strings.str(anno.second.val);
    }
    return result;
  }

  inline std::string getNodeDebugName(const nodeid_t &id) const
  {
    std::stringstream ss;
    ss << getNodeDocument(id) << "/" << getNodeName(id) << "(" << id << ")";

    return ss.str();
  }

  inline std::pair<bool, Annotation> getNodeAnnotation(const nodeid_t &id, const std::uint32_t& nsID, const std::uint32_t& nameID) const
  {
    using AnnoIt = stx::btree_map<NodeAnnotationKey, uint32_t>::const_iterator;

    std::pair<bool, Annotation> result;
    result.first = false;

    NodeAnnotationKey key = {id, nameID, nsID};

    AnnoIt it = nodeAnnotations.find(key);

    if(it != nodeAnnotations.end())
    {
      result.first = true;
      result.second =  {nameID, nsID, it->second};
    }
    return result;
  }

  inline std::pair<bool, Annotation> getNodeAnnotation(const nodeid_t &id, const std::string& ns, const std::string& name) const
  {
    std::pair<bool, std::uint32_t> nsID = strings.findID(ns);
    std::pair<bool, std::uint32_t> nameID = strings.findID(name);

    if(nsID.first && nameID.first)
    {
      return getNodeAnnotation(id, nsID.second, nameID.second);
    }

    std::pair<bool, Annotation> noResult;
    noResult.first = false;
    return noResult;
  }

  std::vector<Component> getDirectConnected(const Edge& edge) const;
  std::vector<Component> getAllComponents() const;
  const ReadableGraphStorage *getEdgeDB(const Component& component) const;
  const ReadableGraphStorage *getEdgeDB(ComponentType type, const std::string& layer, const std::string& name) const;
  std::vector<const ReadableGraphStorage *> getEdgeDB(ComponentType type, const std::string& name) const;
  std::vector<const ReadableGraphStorage *> getAllEdgeDBForType(ComponentType type) const;

  std::vector<Annotation> getEdgeAnnotations(const Component& component,
                                             const Edge& edge);
  std::string info();

  inline std::uint32_t getNamespaceStringID() const {return annisNamespaceStringID;}
  inline std::uint32_t getNodeNameStringID() const {return annisNodeNameStringID;}
  inline std::uint32_t getEmptyStringID() const {return annisEmptyStringID;}
  inline std::uint32_t getTokStringID() const {return annisTokStringID;}

  void convertComponent(Component c, std::string optimizedImpl = "");

  virtual ~DB();
public:

  StringStorage strings;

private:
  /**
   * @brief Maps a fully qualified annotation name for a node to an annotation value
   */
  stx::btree_map<NodeAnnotationKey, uint32_t> nodeAnnotations;
  stx::btree_multimap<Annotation, nodeid_t> inverseNodeAnnotations;

  std::map<Component, ReadableGraphStorage*> edgeDatabases;
  GraphStorageRegistry registry;

  std::uint32_t annisNamespaceStringID;
  std::uint32_t annisEmptyStringID;
  std::uint32_t annisTokStringID;
  std::uint32_t annisNodeNameStringID;

  bool useSpecializedEdgeDB;

  bool loadRelANNISCorpusTab(std::string dirPath, std::map<std::uint32_t, std::uint32_t>& corpusIDToName);
  bool loadRelANNISNode(std::string dirPath, std::map<std::uint32_t, std::uint32_t>& corpusIDToName);
  bool loadRelANNISRank(const std::string& dirPath,
                        const std::map<uint32_t, EdgeDB*>& componentToEdgeDB);

  bool loadEdgeAnnotation(const std::string& dirPath,
                          const std::map<std::uint32_t, EdgeDB* >& pre2EdgeDB,
                          const std::map<std::uint32_t, Edge>& pre2Edge);

  void addNodeAnnotation(nodeid_t nodeID, Annotation& anno)
  {
    nodeAnnotations.insert2({nodeID, anno.name, anno.ns}, anno.val);
    inverseNodeAnnotations.insert2(anno, nodeID);
  }

  void clear();
  void addDefaultStrings();

  ReadableGraphStorage *createEdgeDBForComponent(const std::string& shortType, const std::string& layer,
                       const std::string& name);
  ReadableGraphStorage *createEdgeDBForComponent(ComponentType ctype, const std::string& layer,
                       const std::string& name);
  annis::EdgeDB* createWritableEdgeDB(ComponentType ctype, const std::string& layer,
                       const std::string& name);


  std::string getImplNameForPath(std::string directory);

  ComponentType componentTypeFromShortName(std::string shortType);
};

} // end namespace annis
#endif // ANNISDB_H
