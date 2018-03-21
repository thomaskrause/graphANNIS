/*
 * Copyright 2017 Thomas Krause.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.corpus_tools.graphannis;

import com.google.common.base.Objects;
import com.google.common.collect.Multimap;
import com.google.common.collect.Range;
import com.sun.jna.NativeLong;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.apache.commons.lang3.tuple.ImmutablePair;
import org.apache.commons.lang3.tuple.Pair;
import org.corpus_tools.graphannis.CAPI.AnnisComponent;
import org.corpus_tools.graphannis.CAPI.AnnisVec_AnnisComponent;
import org.corpus_tools.salt.SALT_TYPE;
import org.corpus_tools.salt.SaltFactory;
import org.corpus_tools.salt.common.SDocumentGraph;
import org.corpus_tools.salt.common.SOrderRelation;
import org.corpus_tools.salt.common.SSpan;
import org.corpus_tools.salt.common.STextualDS;
import org.corpus_tools.salt.common.STextualRelation;
import org.corpus_tools.salt.common.SToken;
import org.corpus_tools.salt.core.GraphTraverseHandler;
import org.corpus_tools.salt.core.SAnnotationContainer;
import org.corpus_tools.salt.core.SFeature;
import org.corpus_tools.salt.core.SGraph;
import org.corpus_tools.salt.core.SLayer;
import org.corpus_tools.salt.core.SNode;
import org.corpus_tools.salt.core.SRelation;
import org.corpus_tools.salt.util.SaltUtil;

/**
 * Allows to extract a Salt-Graph from a database subgraph.
 * @author Thomas Krause <thomaskrause@posteo.de>
 */
public class SaltExport {

  private static void mapLabels(SAnnotationContainer n, Map<Pair<String, String>, String> labels) {
    for (Map.Entry<Pair<String, String>, String> e : labels.entrySet()) {
      if ("annis".equals(e.getKey().getKey())) {
        n.createFeature(e.getKey().getKey(), e.getKey().getValue(), e.getValue());
      } else {
        n.createAnnotation(e.getKey().getKey(), e.getKey().getValue(), e.getValue());
      }
    }

  }

  private static boolean hasDominanceEdge(CAPI.NodeIDByRef nID, CAPI.AnnisGraphDB g) {

    // TODO: implement
    AnnisVec_AnnisComponent components = CAPI.annis_graph_all_components(g);
    for (int i = 0; i < CAPI.annis_vec_component_size(components).intValue(); i++) {
      AnnisComponent c = CAPI.annis_vec_component_get(components, new NativeLong(i));
      if (CAPI.AnnisComponentType.Dominance == CAPI.annis_component_type(c)) {
        return true;
      }
    }

    return false;
  }

  private static SNode mapNode(CAPI.NodeIDByRef nID, CAPI.AnnisGraphDB g) {
    SNode newNode;

    // get all annotations for the node into a map, also create the node itself
    Map<Pair<String, String>, String> labels = new LinkedHashMap<>();
    int copyID = nID.getValue();
    CAPI.AnnisVec_AnnisAnnotation annos = CAPI.annis_graph_node_labels(g, new CAPI.NodeID(copyID));
    for (long i = 0; i < CAPI.annis_vec_annotation_size(annos).longValue(); i++) {
      CAPI.AnnisAnnotation.ByReference a = CAPI.annis_vec_annotation_get(annos, new NativeLong(i));

      String ns = CAPI.annis_graph_str(g, a.key.ns).toString();
      String name = CAPI.annis_graph_str(g, a.key.name).toString();
      String value = CAPI.annis_graph_str(g, a.value).toString();

      if (name != null && value != null) {
        if (ns == null) {
          labels.put(new ImmutablePair<>("", name), value);
        } else {
          labels.put(new ImmutablePair<>(ns, name), value);
        }
      }
    }
    annos.dispose();

    if (labels.containsKey(new ImmutablePair<>("annis", "tok"))) {
      newNode = SaltFactory.createSToken();
    } else if (hasDominanceEdge(nID, g)) {
      newNode = SaltFactory.createSStructure();
    } else {
      newNode = SaltFactory.createSSpan();
    }
    newNode.createFeature("annis", "node_id", copyID);

    String nodeName = labels.get(new ImmutablePair<>("annis", "node_name"));
    if (nodeName != null) {
      if (!nodeName.startsWith("salt:/")) {
        nodeName = "salt:/" + nodeName;
      }
      newNode.setId(nodeName);
      // get the name from the ID
      newNode.setName(newNode.getPath().fragment());

    }

    mapLabels(newNode, labels);

    return newNode;
  }

  //  
  //  private static void mapAndAddEdge(SDocumentGraph g, CAPI.Node node, CAPI.Edge origEdge, Map<Long, SNode> nodesByID)
  //  {
  //    SNode source = nodesByID.get(origEdge.sourceID());
  //    SNode target = nodesByID.get(origEdge.targetID());
  //    
  //    
  //    String edgeType = null;
  //    if(source != null && target != null && source != target)
  //    {      
  //      if (origEdge.componentName() != null)
  //      {
  //        edgeType = origEdge.componentName().getString();
  //      }
  //
  //      
  //      SRelation<?,?> rel = null;
  //      switch(origEdge.componentType().getString())
  //      {
  //        case "DOMINANCE":
  //          if (edgeType == null || edgeType.isEmpty())
  //          {
  //            // We don't include edges that have no type if there is an edge
  //            // between the same nodes which has a type.
  //            for(long i=0; i < node.outgoingEdges().size(); i++)
  //            {
  //              CAPI.Edge outEdge = node.outgoingEdges().get(i);
  //              if(outEdge.targetID() == origEdge.targetID() 
  //                && outEdge.componentName() != null
  //                && !outEdge.componentName().getString().isEmpty())
  //              {
  //                // exclude this relation
  //                return;
  //              }
  //            }
  //          } // end mirror check
  //          
  //          rel = g.createRelation(source, target, SALT_TYPE.SDOMINANCE_RELATION, null);
  //          
  //          break;
  //        case "POINTING":
  //          rel = g.createRelation(source, target, SALT_TYPE.SPOINTING_RELATION, null);
  //          break;
  //        case "ORDERING":
  //          rel = g.createRelation(source, target, SALT_TYPE.SORDER_RELATION, null);
  //          break;
  //        case "COVERAGE":
  //          // only add coverage edges in salt to spans, not structures
  //          if(source instanceof SSpan && target instanceof SToken)
  //          {
  //            rel = g.createRelation(source, target, SALT_TYPE.SSPANNING_RELATION, null);
  //          }
  //          break;
  //      }
  //      
  //      
  //      if(rel != null)
  //      {
  //        if(edgeType != null)
  //        {
  //          rel.setType(edgeType);
  //        }
  //        
  //        mapLabels(rel, origEdge.labels());
  //        String layerName = origEdge.componentLayer() == null ? null : origEdge.componentLayer().getString();
  //        if(layerName != null && !layerName.isEmpty())
  //        {
  //          List<SLayer> layer = g.getLayerByName(layerName);
  //          if(layer == null || layer.isEmpty())
  //          {
  //            SLayer newLayer = SaltFactory.createSLayer();
  //            newLayer.setName(layerName);
  //            g.addLayer(newLayer);
  //            layer = Arrays.asList(newLayer);
  //          }
  //          layer.get(0).addRelation(rel);
  //        }
  //      }
  //    }
  //  }
  //  
  //  private static void addNodeLayers(SDocumentGraph g)
  //  {
  //    List<SNode> nodeList = new LinkedList<>(g.getNodes());
  //    for(SNode n : nodeList)
  //    {
  //      SFeature featLayer = n.getFeature("annis", "layer");
  //      if(featLayer != null)
  //      {
  //        SLayer layer = g.getLayer(featLayer.getValue_STEXT());
  //        if(layer == null)
  //        {
  //          layer = SaltFactory.createSLayer();
  //          layer.setName(featLayer.getValue_STEXT());
  //          g.addLayer(layer);
  //        }
  //        layer.addNode(n);
  //      }
  //    }
  //  }
  //  
  //  private static void recreateText(final String name, List<SNode> rootNodes, final SDocumentGraph g)
  //  {
  //    final StringBuilder text = new StringBuilder();
  //    final STextualDS ds = g.createTextualDS("");
  //    
  //    ds.setName(name);
  //    
  //    Map<SToken, Range<Integer>> token2Range = new HashMap<>();
  //    
  //    
  //    // traverse the token chain using the order relations
  //    g.traverse(rootNodes, SGraph.GRAPH_TRAVERSE_TYPE.TOP_DOWN_DEPTH_FIRST,
  //      "ORDERING_" + name,
  //      new GraphTraverseHandler()
  //    {
  //      @Override
  //      public void nodeReached(SGraph.GRAPH_TRAVERSE_TYPE traversalType,
  //        String traversalId, SNode currNode, SRelation<SNode, SNode> relation,
  //        SNode fromNode, long order)
  //      {
  //        if(fromNode != null)
  //        {
  //          text.append(" ");
  //        }
  //        
  //        SFeature featTok = currNode.getFeature("annis::tok");
  //        if(featTok != null && currNode instanceof SToken)
  //        {
  //          int idxStart = text.length();
  //          text.append(featTok.getValue_STEXT());
  //          token2Range.put((SToken) currNode, Range.closed(idxStart, text.length()));
  //        }
  //      }
  //
  //      @Override
  //      public void nodeLeft(SGraph.GRAPH_TRAVERSE_TYPE traversalType,
  //        String traversalId, SNode currNode, SRelation<SNode, SNode> relation,
  //        SNode fromNode, long order)
  //      {
  //      }
  //
  //      @Override
  //      public boolean checkConstraint(SGraph.GRAPH_TRAVERSE_TYPE traversalType,
  //        String traversalId, SRelation relation, SNode currNode,
  //        long order)
  //      {
  //        if(relation == null)
  //        {
  //          // TODO: check if this is ever true
  //          return true;
  //        }
  //        else if(relation instanceof SOrderRelation && Objects.equal(name, relation.getType()))
  //        {
  //          return true;
  //        }
  //        else
  //        {
  //          return false;
  //        }
  //      }
  //    });
  // 
  //    // update the actual text
  //    ds.setText(text.toString());
  //    
  //    // add all relations
  //    
  //    token2Range.forEach((t, r) -> 
  //    {
  //      STextualRelation rel = SaltFactory.createSTextualRelation();
  //      rel.setSource(t);
  //      rel.setTarget(ds);
  //      rel.setStart(r.lowerEndpoint());
  //      rel.setEnd(r.upperEndpoint());
  //      g.addRelation(rel);
  //    });
  //  }
  //  
  //  
  public static SDocumentGraph map(CAPI.AnnisGraphDB orig) {
    SDocumentGraph g = SaltFactory.createSDocumentGraph();

    // create all new nodes
    CAPI.AnnisIterPtr_AnnisNodeID itNodes = CAPI.annis_graph_nodes_by_type(orig, "node");

    for (CAPI.NodeIDByRef nID = CAPI.annis_iter_nodeid_next(itNodes); nID != null; nID = CAPI
        .annis_iter_nodeid_next(itNodes)) {
      SNode n = mapNode(nID, orig);
      // add them to the graph
      g.addNode(n);
    }
    itNodes.dispose();

    //    Map<Long, SNode> newNodesByID = new LinkedHashMap<>();
    //    for(Map.Entry<Long, CAPI.Node> entry : nodesByID.entrySet())
    //    {
    //      CAPI.Node v = entry.getValue();
    //      SNode n = mapNode(v);
    //      newNodesByID.put(entry.getKey(), n);
    //    }
    //    // add them to the graph
    //    newNodesByID.values().stream().forEach(n -> g.addNode(n));
    //    
    //    // create and add all edges
    //    nodesByID.values().
    //      forEach((n) ->
    //    {
    //      for(long i=0; i < n.outgoingEdges().size(); i++)
    //      {
    //        mapAndAddEdge(g, n, n.outgoingEdges().get(i), newNodesByID);
    //      }
    //    });
    //    
    //    // find all chains of SOrderRelations and reconstruct the texts belonging to them
    //    Multimap<String, SNode> orderRoots = g.getRootsByRelationType(SALT_TYPE.SORDER_RELATION);
    //    orderRoots.keySet().
    //      forEach((name) ->
    //    {
    //      ArrayList<SNode> roots = new ArrayList<>(orderRoots.get(name));
    //      if(SaltUtil.SALT_NULL_VALUE.equals(name))
    //      {
    //        name = null;
    //      }
    //      recreateText(name, roots , g);
    //    });
    //    
    //    addNodeLayers(g);
    //    
    return g;
  }
}
