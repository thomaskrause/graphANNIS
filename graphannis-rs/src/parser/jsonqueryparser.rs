use json;
use json::JsonValue;
use query::conjunction::Conjunction;
use query::disjunction::Disjunction;
use exec::nodesearch::NodeSearchSpec;

use std::collections::BTreeMap;

pub fn parse(query_as_string: &str) -> Option<Disjunction> {
    let parsed = json::parse(query_as_string);

    if let Ok(root) = parsed {
        let mut conjunctions: Vec<Conjunction> = Vec::new();
        // iterate over all alternatives
        match root["alternatives"] {
            JsonValue::Array(ref alternatices) => {
                for alt in alternatices.iter() {
                    let mut q = Conjunction::new();

                    // add all nodes
                    let mut node_id_to_pos: BTreeMap<usize, usize> = BTreeMap::new();
                    if let JsonValue::Object(ref nodes) = alt["nodes"] {
                        for (node_name, node) in nodes.iter() {
                            if let JsonValue::Object(ref node_object) = *node {
                                if let Ok(ref node_id) = node_name.parse::<usize>() {
                                    let pos = parse_node(node_object, &mut q);
                                }
                            }
                        }
                    }

                    conjunctions.push(q);
                    unimplemented!();
                }
            }
            _ => {
                return None;
            }
        };

        if !conjunctions.is_empty() {
            return Some(Disjunction::new(conjunctions));
        }
    }

    return None;
}

fn parse_node(node: &json::object::Object, q: &mut Conjunction) -> usize {
    // annotation search?
    if let JsonValue::Array(ref a) = node["nodeAnnotations"] {
        if !a.is_empty() {
            // get the first one
            let a = &a[0];
            return add_node_annotation(
                q,
                a["namespace"].as_str(),
                a["name"].as_str(),
                a["value"].as_str(),
                a["textMatching"].as_str(),
            );
        }
    }
    unimplemented!()
}

fn add_node_annotation(
    q: &mut Conjunction,
    ns: Option<&str>,
    name: Option<&str>,
    value: Option<&str>,
    text_matching: Option<&str>,
) -> usize {
    if let Some(name_val) = name {
        let exact = text_matching == Some("EXACT_EQUAL");
        let regex = text_matching == Some("REGEXP_EQUAL");
        // TODO: replace regex with normal text matching if this is not an actual regular expression

        // search for the value
        if exact {
            // has namespace?
            let n = NodeSearchSpec::new_exact(ns, name_val, value);
            return q.add_node(n);
        } else if regex {

        }
    }
    unimplemented!()
}