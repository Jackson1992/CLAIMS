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
 * /Claims/Executor/query_exec_tracker.h
 *
 *  Created on: Mar 24, 2016
 *      Author: fzh
 *		   Email: fzhedu@gmail.com
 *
 * Description:
 *
 */

#ifndef EXEC_TRACKER_STMT_EXEC_TRACKER_H_
#define EXEC_TRACKER_STMT_EXEC_TRACKER_H_
#include <boost/unordered/unordered_map.hpp>
#include <string>

#include "../common/Block/ResultSet.h"
#include "../common/error_define.h"
#include "../exec_tracker/segment_exec_tracker.h"
#include "../utility/lock.h"
using std::string;

namespace claims {
class StmtExecStatus;
#define kMaxNodeNum 10000
class StmtExecTracker {
 public:
  StmtExecTracker();
  virtual ~StmtExecTracker();
  u_int64_t GenQueryId() { return query_id_gen_++; }
  RetCode RegisterStmtES(StmtExecStatus* stmtes);
  RetCode UnRegisterStmtES(u_int64_t query_id);
  RetCode CancelStmtExec(u_int64_t query_id);
  static void CheckStmtExecStatus(caf::event_based_actor* self,
                                  StmtExecTracker* stmtes);
  bool UpdateSegExecStatus(NodeSegmentID node_segment_id,
                           SegmentExecStatus::ExecStatus exec_status,
                           string exec_info);
  u_int64_t get_logic_time() { return logic_time_; }

 private:
  std::atomic_ullong logic_time_;
  actor stmt_exec_tracker_actor_;
  std::atomic_ullong query_id_gen_;
  boost::unordered_map<u_int64_t, StmtExecStatus*> query_id_to_stmtes_;
  Lock lock_;
};

class StmtExecStatus {
 public:
  enum ExecStatus { kError, kOk, kCancelled, kDone };
  StmtExecStatus(string sql_stmt);
  virtual ~StmtExecStatus();
  u_int64_t get_query_id() { return query_id_; }
  void set_query_id(u_int64_t query_id) { query_id_ = query_id; }
  RetCode CancelStmtExec();
  RetCode RegisterToTracker();
  RetCode UnRegisterFromTracker();
  short GenSegmentId() { return segment_id_gen_++; }
  void AddSegExecStatus(SegmentExecStatus* seg_exec_status);
  bool UpdateSegExecStatus(NodeSegmentID node_segment_id,
                           SegmentExecStatus::ExecStatus exec_status,
                           string exec_info, u_int64_t logic_time);
  bool CouldBeDeleted();
  bool HaveErrorCase(u_int64_t logic_time);
  void set_exec_status(ExecStatus exec_status) { exec_status_ = exec_status; }
  ExecStatus get_exec_status() { return exec_status_; }
  string get_exec_info() { return exec_info_; }
  void set_exec_info(string exec_info) { exec_info_ = exec_info; }
  ResultSet* get_query_result() { return query_result_; }
  void set_query_result(ResultSet* query_result) {
    query_result_ = query_result;
  }
  bool IsCancelled() { return exec_status_ == ExecStatus::kCancelled; }

 private:
  string exec_info_;
  ExecStatus exec_status_;
  ResultSet* query_result_;
  boost::unordered_map<NodeSegmentID, SegmentExecStatus*> node_seg_id_to_seges_;
  u_int64_t query_id_;
  string sql_stmt_;
  std::atomic_short segment_id_gen_;
  Lock lock_;
};

}  // namespace claimsEXEC_TRACKER_STMT_EXEC_TRACKER_H_XEC_TRACKER_H_
#endif