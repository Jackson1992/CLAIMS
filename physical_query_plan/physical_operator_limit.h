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
 * /CLAIMS/physical_query_plan/physical_operator_limit.h
 *
 *  Created on: Sep 18, 2015
 *      Author: wangli,Hanzhang
 *		   Email:wangli1426@gmail.com
 *
 * Description: Implementation of Limit operator in physical layer.
 *
 */

#ifndef PHYSICAL_QUERY_PLAN_PHYSICAL_OPERATOR_LIMIT_H_
#define PHYSICAL_QUERY_PLAN_PHYSICAL_OPERATOR_LIMIT_H_

#include "BlockStreamIteratorBase.h"

//namespace claims {
//namespace physical_query_plan {
/**
 * @brief Method description: Implementation of limit physical operator. This
 * operator is a traditional model of iterator. Execute inline function to judge
 * which position is starting point and whether tuples is acquired.
 */

class BlockStreamLimit : public BlockStreamIteratorBase {
 public:
  struct State {
    friend class BlockStreamLimit;

   public:
    State(Schema* schema, BlockStreamIteratorBase* child, unsigned long limits,
          unsigned block_size, unsigned long start_position = 0);
    State();

   private:
    Schema* schema_;
    BlockStreamIteratorBase* child_;
    unsigned long limits_;
    unsigned block_size_;
    unsigned long start_position_;

   private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& schema_& child_& limits_& block_size_& start_position_;
    }
  };
  BlockStreamLimit();
  BlockStreamLimit(State state);
  virtual ~BlockStreamLimit();
  bool Open(const PartitionOffset&);
  bool Next(BlockStreamBase*);
  bool Close();
  void Print();

 private:
  // this function judges whether tuples we need are acquired.
  inline bool limitExhausted() const {
    return received_tuples_ >= state_.limits_;
  }
  // finding where we should start the operation of limit is the aim of this
  // function.
  inline bool shouldSkip() const { return tuple_cur_ < state_.start_position_; }

 private:
  State state_;
  unsigned received_tuples_;
  BlockStreamBase* block_for_asking_;

  /* the index of current tuple*/
  unsigned long tuple_cur_;
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& boost::serialization::base_object<BlockStreamIteratorBase>(*this) &
        state_;
  }
};
//}  // namespace physical_query_plan
//}  // namespace claims

#endif  //  PHYSICAL_QUERY_PLAN_PHYSICAL_OPERATOR_LIMIT_H_