/*
 * Copyright [2012-2015] DaSE@ECNU
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * /Claims/logical_operator/logical_query_plan_root.h
 *
 *  Created on: Nov 13, 2013
 *      Author: wangli, yukai
 *		   Email: yukai2014@gmail.com
 *
 * Description:
 *
 */

#ifndef LOGICAL_OPERATOR_LOGICAL_QUERY_PLAN_ROOT_H_
#define LOGICAL_OPERATOR_LOGICAL_QUERY_PLAN_ROOT_H_
#include <string>
#include <vector>
#include "../common/ids.h"
#include "../logical_operator/logical_limit.h"
#include "../physical_operator/physical_operator_base.h"
#include "../logical_operator/logical_operator.h"

namespace claims {
namespace logical_operator {

/***
 * @brief: top logical operator in logical query plan,
 *         generating a few top physical operators.
 */
class LogicalQueryPlanRoot : public LogicalOperator {
 public:
  /**
   * three styles decide which one the top physical operator is:
   * 1) BlockStreamPrint: print result on console directly
   * 2) BlockStreamPerformanceMonitorTop: calculate the real-time performance
   * and print
   * 3) BlockStreamResultCollector : collect result and store it in block
   * buffer, then return to client
   */
  enum OutputStyle { kPrint, kPerformance, kResultCollector };
  /**
   * @brief Method description:
   * @param collecter_node_id : specify the id of node that return result to
   * client, which is called master
   * @param child : the child logical operator of this operator
   * @param fashion : decide the top physical operator
   *                  (BlockStreamPrint,BlockStreamPerformanceMonitorTop,BlockStreamResultCollector)
   *                  generated from this logical operator
   *        limit_constraint : apply the necessary info about limit, default
   * value is no limit
   */
  LogicalQueryPlanRoot(NodeID collecter_node_id, LogicalOperator* child,
                       const OutputStyle& fashion = kResultCollector,
                       LimitConstraint limit_constraint = LimitConstraint());
  virtual ~LogicalQueryPlanRoot();
  PlanContext GetPlanContext();
  /**
   * @brief Method description: generate a few physical operators according to
   * this logical operator
   * @param block_size : give the size of block in physical plan
   * @return the total physical plan include the physical plan generated by
   * child operator
   */
  PhysicalOperatorBase* GetPhysicalPlan(const unsigned& block_size);

  /**
   * @brief Method description: NOT USED NOW !!!
   *                            get the optimal physical plan
   */
  bool GetOptimalPhysicalPlan(Requirement requirement,
                              PhysicalPlanDescriptor& physical_plan_descriptor,
                              const unsigned& block_size = 4096 * 1024);

  /**
   * @brief Method description: print the limit info and call child to print
   * their logical operator info
   * @param level : specify the level of this operator, the top level is 0. so
   * this class's level is always 0.
   *                level means level*8 space indentation at the begin of line
   * @return  void
   */
  void Print(int level = 0) const;

 private:
  /**
   * @brief Method description: get all attribute name from PlanContext
   * @param PlanContext :
   * @return  vector of attribute names;
   */
  std::vector<std::string> GetAttributeName(
      const PlanContext& plan_context) const;

 private:
  NodeID collecter_node;
  LogicalOperator* child_;
  OutputStyle style_;
  LimitConstraint limit_constraint_;
};

}  // namespace logical_operator
}  // namespace claims

#endif  // LOGICAL_OPERATOR_LOGICAL_QUERY_PLAN_ROOT_H_
