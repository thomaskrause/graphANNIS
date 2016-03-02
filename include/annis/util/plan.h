/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Plan.h
 * Author: thomas
 *
 * Created on 1. März 2016, 11:48
 */

#pragma once

#include <memory>
#include <vector>
#include <annis/iterators.h>

namespace annis
{
  
struct ExecutionNode
{
  size_t lhsIdx;
  size_t rhsIdx;

  std::shared_ptr<BinaryIt> join;
};

class Plan
{
public:
  Plan(const std::vector<std::shared_ptr<AnnoIt>>& source);
  
  Plan(const Plan& orig);
  virtual ~Plan();
  
  bool executeStep(std::vector<Match>& result);
  double getCost();
  
private:
//  ExecutionNode root;
  std::vector<std::shared_ptr<AnnoIt>> source;
  double cost;
  
};

} // end namespace annis