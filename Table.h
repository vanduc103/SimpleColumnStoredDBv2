/*
 * Table.h
 *
 *  Created on: Sep 26, 2016
 *      Author: duclv
 */

#ifndef TABLE_H_
#define TABLE_H_

#include <string>
#include <tuple>
#include <vector>
#include <iostream>
#include "Column.h"
#include "ColumnBase.h"

namespace std {

class Table {
private:
	vector<ColumnBase*>* m_columns;
	string name;
public:
	virtual ~Table() {
		for (ColumnBase* column : (*m_columns)) {
			delete column;
		}
		delete m_columns;
	}

	Table(vector<ColumnBase*>* columns) {
		if (columns == NULL) columns = new vector<ColumnBase*>();
		m_columns = columns;
	}

	vector<ColumnBase*>* columns() {
		return m_columns;
	}

	string getName() {
		return name;
	}
	void setName(string tableName) {
		name = tableName;
	}

	int numOfColumns() {
		return (int) m_columns->size();
	}

	ColumnBase* getColumnByName(string colName) {
		//int tupleSize = tuple_size<decltype(m_columns)>::value;;
		for (size_t i = 0; i < m_columns->size(); i++) {
			ColumnBase* column = m_columns->at(i);
			if (column->getName() == colName)
				return column;
		}
		return NULL;
	}

	// do some processes on all columns
	void processColumn(uint64_t csn = 0) {
		for (ColumnBase* colBase : *m_columns) {
			if (colBase->getType() == ColumnBase::intType) {
				Column<int>* col = (Column<int>*) colBase;
				if (col->isBulkInsert())
					col->bulkBuildVecVector(csn);
				col->getDictionary()->clearTemp();
				col->bitPackingVecValue();
			}
			else if (colBase->getType() == ColumnBase::charType ||
					 colBase->getType() == ColumnBase::varcharType) {
				Column<string>* col = (Column<string>*) colBase;
				if (col->isBulkInsert()) {
					col->bulkBuildVecVector(csn);
				}
				if (col->isCreateInvertedIndex()) {
					col->createInvertedIndex();
				}
				col->getDictionary()->clearTemp();
				col->bitPackingVecValue();
			}
		}
	}
};

} /* namespace std */

#endif /* TABLE_H_ */
