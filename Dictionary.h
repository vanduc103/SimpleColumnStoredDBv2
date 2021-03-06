/*
 * Dictionary.h
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */
#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include "ColumnBase.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

template<class T>
class Dictionary {
private:
	struct invertedIndex {
		string word;
		char position;	 // position on text
		vector<size_t> location; // position on dictionary
		// functions
		bool operator<(const invertedIndex& a) const
		{
			return word < a.word;
		}
		bool operator==(const invertedIndex& a) const
		{
			return word == a.word;
		}
	};

	vector<T>* items;
	bool sorted = false;
	std::unordered_map<T, size_t>* sMap;
	vector<T>* bulkVecValue;
	vector<invertedIndex>* vecIndexLevel0;
public:
	Dictionary() {
		items = new vector<T>();
		sMap = new unordered_map<T, size_t>();
		bulkVecValue = new vector<T>();
		vecIndexLevel0 = new vector<invertedIndex>();
	}
	virtual ~Dictionary() {
		delete items;
		delete vecIndexLevel0;
		delete sMap;
		delete bulkVecValue;
	}

	T* lookup(size_t index);

	void search(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result);
	void searchWithSorted(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result);
	void searchWithNoSorted(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result);

	size_t addNewElement(T& value, vector<size_t>* vecValue, bool sorted, bool bulkInsert);
	size_t size();
	void print(int row);
	void buildInvertedIndex();
	void sort();

	vector<T>* getBulkVecValue() {
		return bulkVecValue;
	}

	void setSorted(bool sorted) {
		this->sorted = sorted;
	}
	bool getSorted() {
		return this->sorted;
	}

	void clearTemp() {
		bulkVecValue->resize(0);
	}

};


#endif /* DICTIONARY_H_ */
