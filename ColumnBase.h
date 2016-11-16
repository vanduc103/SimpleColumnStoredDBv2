/*
 * ColumnBase.h
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */

#ifndef COLUMNBASE_H_
#define COLUMNBASE_H_

#include <string>

namespace std {

class ColumnBase {
public:
	enum COLUMN_TYPE {intType, charType, varcharType};
	enum OP_TYPE {equalOp, neOp, ltOp, leOp, gtOp, geOp, containOp};
private:
	string name;
	COLUMN_TYPE type;
	int size;
	bool isPrimaryKey = false; // default is false
	bool createInvertedIndex = false; // create inverted index for this column ?
public:
	ColumnBase();
	virtual ~ColumnBase();

	string getName();
	void setName(string nameValue);

	COLUMN_TYPE getType();
	void setType(COLUMN_TYPE typeValue);

	int getSize();
	void setSize(int sizeValue);

	bool primaryKey();
	void setPrimaryKey(bool isPrimaryKey);

	bool isCreateInvertedIndex() {return createInvertedIndex; }
	void setCreateInvertedIndex(bool createInvertedIndex) {this->createInvertedIndex = createInvertedIndex; }

	static OP_TYPE sToOp(string op) {
		if (op == ">") return OP_TYPE::gtOp;
		else if (op == ">=") return OP_TYPE::geOp;
		else if (op == "<") return OP_TYPE::ltOp;
		else if (op == "<=") return OP_TYPE::leOp;
		else if (op == "=") return OP_TYPE::equalOp;
		else if (op == "<>") return OP_TYPE::neOp;
		return OP_TYPE::containOp;
	}
};

} /* namespace std */

#endif /* COLUMNBASE_H_ */
