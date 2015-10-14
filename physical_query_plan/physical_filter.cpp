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
 * /CLAIMS/physical_query_plan/physical_filter.cpp
 *
 *  Created on: Aug 28, 2013
 *      Author: wangli, Hanzhang
 *		   Email: wangli1426@gmail.com
 *
 * Description: Implementation of Filter operator in physical layer.
 *
 */

#include "../physical_query_plan/physical_filter.h"
#include <assert.h>
#include <limits>
#include "../../utility/warmup.h"
#include "../../utility/rdtsc.h"
#include "../../common/ExpressionCalculator.h"
#include "../../common/Expression/execfunc.h"
#include "../../common/Expression/qnode.h"
#include "../../common/Expression/initquery.h"
#include "../../common/Expression/queryfunc.h"
#include "../../common/data_type.h"
#include "../../Config.h"
#include "../../Parsetree/sql_node_struct.h"
#include "../../codegen/ExpressionGenerator.h"

#define NEWCONDITION

// namespace claims{
// namespace physica query_plan{

ExpandableBlockStreamFilter::ExpandableBlockStreamFilter(State state)
    : state_(state),
      generated_filter_function_(0),
      generated_filter_processing_fucntoin_(0) {
  InitExpandedStatus();
}

ExpandableBlockStreamFilter::ExpandableBlockStreamFilter()
    : generated_filter_function_(0), generated_filter_processing_fucntoin_(0) {
  InitExpandedStatus();
}

ExpandableBlockStreamFilter::~ExpandableBlockStreamFilter() {}
ExpandableBlockStreamFilter::State::State(Schema* schema,
                                          BlockStreamIteratorBase* child,
                                          vector<QNode*> qual,
                                          map<string, int> colindex,
                                          unsigned block_size)
    : schema_(schema),
      child_(child),
      qual_(qual),
      colindex_(colindex),
      block_size_(block_size) {}
ExpandableBlockStreamFilter::State::State(
    Schema* schema, BlockStreamIteratorBase* child,
    std::vector<AttributeComparator> comparator_list, unsigned block_size)
    : schema_(schema),
      child_(child),
      comparator_list_(comparator_list),
      block_size_(block_size) {}

/**
 * @brief Method description:To choose which optimization way of filter
 *function.(if enable_codegen isn't open, we should execute the function:
 *computerFilter ). traditional model of iterator of physical plan.
 * 1)If a block can be optimized by llvm, we choose
 *generated_filter_processing_function_.
 * 2)If a tuple can be optimized by llvm, we choose
 *computerFilterwithGeneratedCode.
 * 3)If it can't be optimized by llvm , we still choose computerFilter.
 */
bool ExpandableBlockStreamFilter::Open(const PartitionOffset& part_off) {
  RegisterExpandedThreadToAllBarriers();
  filter_thread_context* ftc =
      (filter_thread_context*)CreateOrReuseContext(crm_core_sensitive);

  if (TryEntryIntoSerializedSection()) {
    if (Config::enable_codegen) {
      ticks start = curtick();
      generated_filter_processing_fucntoin_ =
          getFilterProcessFunc(state_.qual_[0], state_.schema_);
      if (generated_filter_processing_fucntoin_) {
        printf("CodeGen (full feature) succeeds!(%f8.4ms)\n",
               getMilliSecond(start));
      } else {
        generated_filter_function_ =
            getExprFunc(state_.qual_[0], state_.schema_);
        if (generated_filter_function_) {
          ff_ = ComputeFilterwithGeneratedCode;
          printf("CodeGen (partial feature) succeeds!(%f8.4ms)\n",
                 getMilliSecond(start));
        } else {
          ff_ = ComputeFilter;
          printf("CodeGen fails!\n");
        }
      }
    } else {
      ff_ = ComputeFilter;
      printf("CodeGen closed!\n");
    }
  }
  bool ret = state_.child_->Open(part_off);
  SetReturnStatus(ret);
  BarrierArrive();
  return GetReturnStatus();
}

bool ExpandableBlockStreamFilter::Next(BlockStreamBase* block) {
  void* tuple_from_child;
  void* tuple_in_block;
  filter_thread_context* tc = (filter_thread_context*)GetContext();
  while (true) {
    if (tc->block_stream_iterator_->currentTuple() == 0) {
      /* mark the block as processed by setting it empty*/
      tc->block_for_asking_->setEmpty();
      if (state_.child_->Next(tc->block_for_asking_)) {
        delete tc->block_stream_iterator_;
        tc->block_stream_iterator_ = tc->block_for_asking_->createIterator();
      } else {
        if (!block->Empty())
          return true;
        else
          return false;
      }
    }
    process_logic(block, tc);
    /**
     * @brief Method description: There are totally two reasons for the end of
     * the while loop.
     * (1) block is full of tuples satisfying filter (should return true to the
     * caller)
     * (2) block_for_asking_ is exhausted (should fetch a new block from child
     * and continue to process)
     */

    if (block->Full())
      // for case (1)
      return true;
  }
}

/**
 * @brief Method description:According to which optimization of generate filter
 * function,execute function with related parameters. Different operator has
 * different implementation in process_logic().
 */
void ExpandableBlockStreamFilter::process_logic(BlockStreamBase* block,
                                                filter_thread_context* tc) {
  if (generated_filter_processing_fucntoin_) {
    int b_cur = block->getTuplesInBlock();
    int c_cur = tc->block_stream_iterator_->get_cur();
    const int b_tuple_count = block->getBlockCapacityInTuples();
    const int c_tuple_count = tc->block_for_asking_->getTuplesInBlock();

    generated_filter_processing_fucntoin_(
        block->getBlock(), &b_cur, b_tuple_count,
        tc->block_for_asking_->getBlock(), &c_cur, c_tuple_count);
    ((BlockStreamFix*)block)->setTuplesInBlock(b_cur);
    tc->block_stream_iterator_->set_cur(c_cur);
  } else {
    void* tuple_from_child;
    void* tuple_in_block;
    while ((tuple_from_child = tc->block_stream_iterator_->currentTuple()) >
           0) {
      bool pass_filter = true;
#ifdef NEWCONDITION
      ff_(pass_filter, tuple_from_child, generated_filter_function_,
          state_.schema_, tc->thread_qual_);
#else
      pass_filter = true;
      for (unsigned i = 0; i < state_.comparator_list_.size(); i++) {
        if (!state_.comparator_list_[i].filter(state_.schema_->getColumnAddess(
                state_.comparator_list_[i].get_index(), tuple_from_child))) {
          pass_filter = false;
          break;
        }
      }
#endif
      if (pass_filter) {
        const unsigned bytes =
            state_.schema_->getTupleActualSize(tuple_from_child);
        if ((tuple_in_block = block->allocateTuple(bytes)) > 0) {
          block->insert(tuple_in_block, tuple_from_child, bytes);
          tuple_after_filter_++;
        } else {
          /* we have got a block full of result tuples*/
          return;
        }
      }
      /* point the iterator to the next tuple */
      tc->block_stream_iterator_->increase_cur_();
    }
    /* mark the block as processed by setting it empty*/
    tc->block_for_asking_->setEmpty();
  }
}

bool ExpandableBlockStreamFilter::Close() {
  InitExpandedStatus();
  DestoryAllContext();
  state_.child_->Close();
  return true;
}

void ExpandableBlockStreamFilter::Print() {
  printf("filter: \n");
  for (int i = 0; i < state_.qual_.size(); i++) {
    printf("  %s\n", state_.qual_[i]->alias.c_str());
  }
  state_.child_->Print();
}

void ExpandableBlockStreamFilter::ComputeFilter(bool& ret, void* tuple,
                                                expr_func func_gen,
                                                Schema* schema,
                                                vector<QNode*> thread_qual_) {
  ret = ExecEvalQual(thread_qual_, tuple, schema);
}

void ExpandableBlockStreamFilter::ComputeFilterwithGeneratedCode(
    bool& ret, void* tuple, expr_func func_gen, Schema* schema,
    vector<QNode*> allocator) {
  func_gen(tuple, &ret);
}

ExpandableBlockStreamFilter::filter_thread_context::~filter_thread_context() {
  delete block_for_asking_;
  delete temp_block_;
  delete block_stream_iterator_;
  for (int i = 0; i < thread_qual_.size(); i++) {
    delete thread_qual_[i];
  }
}

ThreadContext* ExpandableBlockStreamFilter::CreateContext() {
  filter_thread_context* ftc = new filter_thread_context();
  ftc->block_for_asking_ =
      BlockStreamBase::createBlock(state_.schema_, state_.block_size_);
  ftc->temp_block_ =
      BlockStreamBase::createBlock(state_.schema_, state_.block_size_);
  ftc->block_stream_iterator_ = ftc->block_for_asking_->createIterator();
  ftc->thread_qual_ = state_.qual_;
  for (int i = 0; i < state_.qual_.size(); i++) {
    Expr_copy(state_.qual_[i], ftc->thread_qual_[i]);
    InitExprAtPhysicalPlan(ftc->thread_qual_[i]);
  }
  return ftc;
}

//}//namespace claims
//}// namespace physical_query_plan