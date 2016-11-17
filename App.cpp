//============================================================================
// Name        : App.cpp
// Author      : Le Van Duc
// Version     :
// Copyright   : 
// Description : Main program to run functions of simple column-stored DB
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include "Dictionary.h"
#include "Column.h"
#include "ColumnBase.h"
#include "Table.h"
#include "SQLParser.h"
#include "Util.h"
#include "App.h"

using namespace std;

// print operator type string value
std::ostream& operator<<(std::ostream& out, const ColumnBase::OP_TYPE value){
    const char* s = 0;
#define PROCESS_VAL(p, str) case(p): s = str; break;
    switch(value){
        PROCESS_VAL(ColumnBase::OP_TYPE::equalOp, "=");
        PROCESS_VAL(ColumnBase::OP_TYPE::neOp, "<>");
        PROCESS_VAL(ColumnBase::OP_TYPE::gtOp, ">");
        PROCESS_VAL(ColumnBase::OP_TYPE::geOp, ">=");
        PROCESS_VAL(ColumnBase::OP_TYPE::ltOp, "<");
        PROCESS_VAL(ColumnBase::OP_TYPE::leOp, "<=");
        PROCESS_VAL(ColumnBase::OP_TYPE::containOp, "CONTAIN");
    }
#undef PROCESS_VAL

    return out << s;
}

// print column type
std::ostream& operator<<(std::ostream& out, const ColumnBase::COLUMN_TYPE value){
    const char* s = 0;
#define PROCESS_VAL(p, str) case(p): s = str; break;
    switch(value){
        PROCESS_VAL(ColumnBase::intType, "INTEGER");
        PROCESS_VAL(ColumnBase::charType, "TEXT");
        PROCESS_VAL(ColumnBase::varcharType, "TEXT");
    }
#undef PROCESS_VAL
    return out << s;
}

Table* createTable(string createQuery) {
	hsql::SQLParserResult* pCreateQuery = hsql::SQLParser::parseSQLString(createQuery);
	string tableName;
	vector<string> cColumnName;
	vector<ColumnBase::COLUMN_TYPE> cColumnType;
	if (pCreateQuery->isValid) {
		hsql::SQLStatement* stmt = pCreateQuery->getStatement(0);
		if (stmt->type() == hsql::StatementType::kStmtCreate) {
			hsql::CreateStatement* createStmt = (hsql::CreateStatement*) stmt;
			tableName = createStmt->tableName;
			vector<hsql::ColumnDefinition*>* cols = createStmt->columns;
			for (hsql::ColumnDefinition* colDef : *cols) {
				cColumnName.push_back(colDef->name);
				switch (colDef->type) {
					case hsql::ColumnDefinition::INT:
					case hsql::ColumnDefinition::DOUBLE:
						cColumnType.push_back(ColumnBase::intType);
						break;
					case hsql::ColumnDefinition::TEXT:
						cColumnType.push_back(ColumnBase::charType);
						break;
				}
			}
		}
	}
	else {
		cout << "Create table statement is Invalid !" << endl;
		return NULL;
	}
	// init table
	vector<ColumnBase*>* columns = new vector<ColumnBase*>();
	Table* table = new Table(columns);
	table->setName(tableName);
	// init column
	for (size_t i = 0; i < cColumnName.size(); i++) {
		string name = cColumnName[i];
		ColumnBase::COLUMN_TYPE type = cColumnType[i];
		// create new column
		ColumnBase* colBase = new ColumnBase();
		if (type == ColumnBase::intType) {
			Column<int>* col = new Column<int>();
			colBase = col;
		}
		else {
			Column<string>* col = new Column<string>();
			colBase = col;
		}
		colBase->setName(name);
		colBase->setType(type);
		//if (name == "o_orderkey") colBase->setPrimaryKey(true);
		//if (name == "o_comment") colBase->setCreateInvertedIndex(true);
		columns->push_back(colBase);
	}

	// calculate time execution
	clock_t begin_time = clock();
	// create Transaction
	Transaction transaction;
	size_t txIdx = transaction.createTx();
	transaction.startTx(txIdx);
	uint64_t csn = transaction.getTimestampAsCSN();

	// read file
	cout << "Enter table source file path: ";
	string filePath = "/home/duclv/homework/data1M.csv";
	getline(cin, filePath);
	ifstream infile(filePath);
	string line;
	string delim = ",";
	int row = 0;
	while(getline(infile, line)) {
		size_t last = 0; size_t next = 0;
		char idx = 0;
		vector<string> items;	// split items
		string lastItem;
		string token;
		while ((next = line.find(delim, last)) != string::npos) {
			token = line.substr(last, next - last);
			last = next + delim.length();
			if (idx >= table->numOfColumns() - 1)
				lastItem += token + delim;
			else
				items.push_back(token);
			++idx;
		}
		// get the last token and add to last item
		token = line.substr(last);
		lastItem += token;
		items.push_back(lastItem);

		// process input data based on column type
		for (int i = 0; i < table->numOfColumns(); i++) {
			string item = items[i];
			ColumnBase* colBase = columns->at(i);
			if (colBase->getType() == ColumnBase::intType) {
				int intValue = stoi(item);
				bool sorted = true; bool bulkInsert = true;
				// update dictionary
				Column<int>* col = (Column<int>*) colBase;
				col->updateDictionary(intValue, sorted, bulkInsert, csn);
			}
			else {
				// char or varchar type
				boost::replace_all(item, "\"", "");
				bool sorted = false; bool bulkInsert = true;
				// update dictionary
				Column<string>* col = (Column<string>*) colBase;
				col->updateDictionary(item, sorted, bulkInsert, csn);
			}
		}
		++row;
	}
	infile.close();
	// print distinct numbers
	for (ColumnBase* colBase : (*columns)) {
		if (colBase->getType() == ColumnBase::intType) {
			Column<int>* col = (Column<int>*) colBase;
			cout << col->getName() << " #distinct values = " << col->getDictionary()->size()<<"/"<<row << endl;
		}
		else {
			Column<string>* col = (Column<string>*) colBase;
			cout << col->getName() << " #distinct values = " << col->getDictionary()->size()<<"/"<<row << endl;
		}
	}

	// process columns of table
	table->processColumn(csn);

	// loaded time
	cout << "Table Load time: " << float( clock () - begin_time ) /  CLOCKS_PER_SEC << " seconds " << endl;

	return table;
};

// UPDATE
string updateCommand(ServerSocket* client, Table* table, Transaction* transaction,
		vector<string> command, GarbageCollector* garbage, size_t txIdx) {
	if (command.size() < 2) {
		return "ERROR: No row id field !";
	}
	int test = stoi(command[1]);
	if (test < 1) {
		return "ERROR: row id must start from 1 !";
	}
	size_t rid = test - 1;	// rid start from 0
	// update value init
	int o_orderkey = -1;
	string o_orderstatus = "";
	int o_totalprice = -1;
	string o_comment = "";
	// create Transaction (if new command)
	if (txIdx == (size_t) -1) {
		txIdx = transaction->createTx();
		transaction->setClient(txIdx, client);
		transaction->setCommand(txIdx, command);
	}
	transaction->startTx(txIdx);
	uint64_t csn = transaction->getStartTimestamp(txIdx);
	// get update value from command and execute update
	if (command.size() >= 3) {
		o_orderkey = stoi(command[3-1]);
		Column<int>* col = (Column<int>*) table->getColumnByName("o_orderkey");
		if (col != NULL) {
			if (col->numOfRows() <= rid) {
				return "ERROR: row id excess number of rows !";
			}
			// check column's CSN with tx start time stamp
			if (col->getCSN(rid) <= csn)
				col->addVersionVecValue(o_orderkey, csn, rid);
			else
				transaction->addToWaitingList(txIdx);
		}
	}
	if (command.size() >= 4) {
		o_orderstatus = command[4-1];
		Column<string>* col = (Column<string>*) table->getColumnByName("o_orderstatus");
		if (col->numOfRows() <= rid) {
			return "ERROR: row id excess number of rows !";
		}
		// check column's CSN with tx start time stamp
		if (col->getCSN(rid) <= csn)
			col->addVersionVecValue(o_orderstatus, csn, rid);
		else
			transaction->addToWaitingList(txIdx);
	}
	if (command.size() >= 5) {
		o_totalprice = stoi(command[5-1]);
		Column<int>* col = (Column<int>*) table->getColumnByName("o_totalprice");
		if (col->numOfRows() <= rid) {
			return "ERROR: row id excess number of rows !";
		}
		// check column's CSN with tx start time stamp
		if (col->getCSN(rid) <= csn)
			col->addVersionVecValue(o_totalprice, csn, rid);
		else
			transaction->addToWaitingList(txIdx);
	}
	if (command.size() >= 6) {
		o_comment = command[6-1];
		Column<string>* col = (Column<string>*) table->getColumnByName("o_comment");
		if (col->numOfRows() <= rid) {
			return "ERROR: row id excess number of rows !";
		}
		// check column's CSN with tx start time stamp
		if (col->getCSN(rid) <= csn)
			col->addVersionVecValue(o_comment, csn, rid);
		else
			transaction->addToWaitingList(txIdx);
	}
	// commit Transaction (if not in waiting list)
	if (transaction->getTransaction(txIdx).status != Transaction::TRANSACTION_STATUS::WAITING) {
		transaction->commitTx(txIdx, csn);
		// add to Garbage Collector
		vector<size_t> updateRids;
		updateRids.push_back(rid);
		garbage->updateRecentlyUpdateRids(updateRids);

		return "Updated 1 row with rid = " + to_string(rid + 1);
	}
	return "WAITING";
}

// INSERT
string insertCommand(Table* table, Transaction* transaction, vector<string> command) {
	if (command.size() < 5) {
		cout << "ERROR: Not enough 4 fields to INSERT !" << endl;
		return "ERROR: Not enough 4 fields to INSERT !";
	}
	// insert value init
	int o_orderkey = -1;
	string o_orderstatus = "";
	int o_totalprice = -1;
	string o_comment = "";
	// create Transaction
	size_t txIdx = transaction->createTx();
	// get insert value from command and execute insert
	transaction->startTx(txIdx);
	uint64_t csn = transaction->getTimestampAsCSN();
	size_t numOfRow = 0;
	if (command.size() >= 2) {
		o_orderkey = stoi(command[2-1]);
		Column<int>* col = (Column<int>*) table->getColumnByName("o_orderkey");
		col->insertDataVecValue(o_orderkey, csn);
		numOfRow = col->numOfRows();
	}
	if (command.size() >= 3) {
		o_orderstatus = command[3-1];
		Column<string>* col = (Column<string>*) table->getColumnByName("o_orderstatus");
		col->insertDataVecValue(o_orderstatus, csn);
		numOfRow = col->numOfRows();
	}
	if (command.size() >= 4) {
		o_totalprice = stoi(command[4-1]);
		Column<int>* col = (Column<int>*) table->getColumnByName("o_totalprice");
		col->insertDataVecValue(o_totalprice, csn);
		numOfRow = col->numOfRows();
	}
	if (command.size() >= 5) {
		o_comment = command[5-1];
		Column<string>* col = (Column<string>*) table->getColumnByName("o_comment");
		col->insertDataVecValue(o_comment, csn);
		numOfRow = col->numOfRows();
	}
	// commit Transaction
	transaction->commitTx(txIdx, csn);
	return "INSERTED 1 row ! Number of rows is: " + numOfRow;
}

// SCAN
string scanCommand(Table* table, Transaction* transaction, vector<string> command) {
	if (command.size() < 4) {
		cout << "SCAN command must have at least 4 fields: SCAN|filter value|filter column|filter operator !" << endl;
		return "SCAN command must have at least 4 fields: SCAN|filter value|filter column|filter operator !";
	}
	string filterValue1 = command[1];
	string column1 = command[2];
	string operator1 = command[3];

	clock_t begin_time = clock();
	// create Transaction
	size_t txIdx = transaction->createTx();
	transaction->startTx(txIdx);
	uint64_t txTs = transaction->getStartTimestamp(txIdx);
	// select with filter value 1
	vector<bool>* q_resultRid = new vector<bool>();
	ColumnBase* colBase = table->getColumnByName(column1);
	if (colBase != NULL && colBase->getType() == ColumnBase::intType) {
		Column<int>* col = (Column<int>*) colBase;
		int searchValue = stoi(filterValue1);
		col->selection(searchValue, ColumnBase::sToOp(operator1), q_resultRid);
	}
	else if (colBase != NULL && colBase->getType() == ColumnBase::charType) {
		Column<string>* col = (Column<string>*) colBase;
		string searchValue = filterValue1;
		col->selection(searchValue, ColumnBase::sToOp(operator1), q_resultRid);
	}
	// filter 2
	if (command.size() >= 7) {
		string filterValue2 = command[4];
		string column2 = command[5];
		string operator2 = command[6];
		// select with filter value 2
		ColumnBase* colBase = table->getColumnByName(column2);
		bool initResultRid = false;
		if (colBase != NULL && colBase->getType() == ColumnBase::intType) {
			Column<int>* col = (Column<int>*) colBase;
			int searchValue = stoi(filterValue2);
			col->selection(searchValue, ColumnBase::sToOp(operator2), q_resultRid, initResultRid);
		}
		else if (colBase != NULL && colBase->getType() == ColumnBase::charType) {
			Column<string>* col = (Column<string>*) colBase;
			string searchValue = filterValue2;
			col->selection(searchValue, ColumnBase::sToOp(operator2), q_resultRid, initResultRid);
		}
	}

	// update list row id for Transaction
	vector<size_t> vecRowid;
	for (size_t i = 0; i < q_resultRid->size(); i++) {
		if (q_resultRid->at(i))
			vecRowid.push_back(i);
	}
	transaction->updateRid2Transaction(txIdx, vecRowid);
	// scan all columns with version space to get result
	//cout << "********* Print scan result ************" << endl;
	size_t totalresult = 0;
	for (size_t i = 0; i < q_resultRid->size(); i++) {
		if (q_resultRid->at(i))
			++totalresult;
	}
	size_t limit = 10;
	size_t limitCount = 0;
	vector<string> q_select_fields;
	for (ColumnBase* colBase : *table->columns()) {
		q_select_fields.push_back(colBase->getName());
	}
	vector<string> outputs (limit + 2);
	for (size_t idx = 0; idx < q_select_fields.size(); idx++) {
		string select_field_name = q_select_fields[idx];
		outputs[0] += select_field_name + ", ";
		ColumnBase* colBase = (ColumnBase*) table->getColumnByName(select_field_name);
		if (colBase == NULL) continue;
		if (colBase->getType() == ColumnBase::intType) {
			Column<int>* t = (Column<int>*) colBase;
			vector<int> tmpOut = t->projectionWithVersion(q_resultRid, txTs, limit, limitCount);
			for (size_t i = 0; i < tmpOut.size(); i++) {
				outputs[i+1] += to_string(tmpOut[i]) + ",   ";
				// padding whitespace
				for (int j = 11 - (outputs[i+1].length()); j > 0; j--) {
					outputs[i+1] += " ";
				}
			}
		}
		else {
			Column<string>* t = (Column<string>*) colBase;
			vector<string> tmpOut = t->projectionWithVersion(q_resultRid, txTs, limit, limitCount);
			for (size_t i = 0; i < tmpOut.size(); i++) {
				outputs[i+1] += "\"" + tmpOut[i] + "\"" + ",   ";
			}
		}
	}
	// commit transaction
	transaction->commitTx(txIdx, transaction->getTimestampAsCSN());
	// show result
	std::stringstream showing;
	if (limitCount >= limit) {
		showing << "Showing "<<limit << "/" << totalresult <<" results !";
	}
	else if (limitCount == 0) {
		showing << "No result found !";
	}
	else {
		showing << "Showing " << limitCount << "/" << totalresult <<" results !";
	}
	outputs.push_back(showing.str());
	std::stringstream returnResult;
	for (string output : outputs) {
		if (!output.empty()) {
			//cout << output << endl;
			returnResult << output << endl;
		}
	}
	returnResult << "Query time: " << float(clock() - begin_time)/CLOCKS_PER_SEC << " seconds " << endl;
	return returnResult.str();
}

