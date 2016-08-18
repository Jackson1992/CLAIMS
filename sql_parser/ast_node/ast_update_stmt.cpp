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
 * /CLAIMS/sql_parser/ast_node/tDeleteStmt.cpp
 *
 *  Created on: Nov 19, 2015
 *      Author: yuyang
 *		   Email: youngfish93@hotmail.com
 *
 * Description:
 *
 */

#include "../ast_node/ast_update_stmt.h"

#include <glog/logging.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <bitset>

#include "./ast_select_stmt.h"
#include "../../common/error_define.h"
#include "../../catalog/table.h"
#include "../../Environment.h"
using namespace claims::common;  // NOLINT
using std::cout;
using std::endl;
using std::string;
using std::setw;
using std::bitset;

// namespace claims {
// namespace ast_node {

AstUpdateStmt::AstUpdateStmt(AstNodeType ast_node_type, 
		AstNode* update_set_list, AstNode* update_table_list, 
		AstNode* where_list)
    : AstNode(ast_node_type),
    update_set_list_(update_set_list),
    update_table_list_(update_table_list),
    where_list_(where_list) {}

AstUpdateStmt::~AstUpdateStmt() {
  if (NULL != update_set_list_) {
  	delete update_set_list_;
	update_set_list_ = NULL;
  }
  
  if (NULL != where_list_) {
    delete where_list_;
    where_list_ = NULL;
  }
}

void AstUpdateStmt::Print(int level) const {
  cout << setw(level * TAB_SIZE) << " "
       << "|Update Stmt|" << endl;
  

  if (NULL != update_set_list_) {
    update_set_list_->Print(level + 1);
  }

  if (NULL != where_list_) {
    where_list_->Print(level + 1);
  }
}

RetCode AstUpdateStmt::SemanticAnalisys(SemanticContext* sem_cnxt) {
  RetCode ret = rSuccess;
  string tablename =
      dynamic_cast<AstTable*>(dynamic_cast<AstFromList*>(from_list_)->args_)
          ->table_name_;
  TableDescriptor* new_table =
      Environment::getInstance()->getCatalog()->getTable(tablename);
  if (NULL == new_table) {
    LOG(ERROR) << "The table " + tablename + " is not existed.";
    sem_cnxt->error_msg_ = "The table " + tablename + " is not existed.";
    ret = rTableNotExisted;
    return ret;
  }
  string tabledel = tablename + "_DEL";
  new_table = Environment::getInstance()->getCatalog()->getTable(tabledel);
  if (NULL == new_table) {
    LOG(ERROR) << "The table DEL " + tabledel +
                      " is not existed during delete data." << std::endl;
    sem_cnxt->error_msg_ =
        "The table DEL " + tabledel + " is not existed during delete data.";
    ret = rTableNotExisted;
    return ret;
  }
  return ret;
}

AstUpdateSetList::AstUpdateSetList(AstNodeType ast_node_type, 
	AstNode * args0, AstNode * args1, AstNode * next) 
	: ast_node_type_(ast_node_type),
	args0_(args0),
	args1_(args1),
	next_(next) {}

AstUpdateSetList::~AstUpdateSetList()
{
	if (NULL != args0_) {
		delete args0_;
		args0_= NULL;
	}

	if (NULL != args1_) {
		delete args1_;
		args1_= NULL;
	}

	if (NULL != next_) {
		delete next_;
		next_= NULL;
	}
}

AstUpdateSetList::Print(int level) const
{

}

AstUpdateSetList::SemanticAnalisys(SemanticContext * sem_cnxt)
{

}

AstUpdateSetList::PushDownCondition(PushDownConditionContext & pdccnxt)
{

}

AstUpdateSetList::GetLogicalPlan(LogicalOperator * & logic_plan)
{

}






// } /* namespace ast_node */
// } /* namespace claims */
