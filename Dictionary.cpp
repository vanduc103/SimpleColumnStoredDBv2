/*
 * Dictionary.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */
#ifndef _Dictionary_
#define _Dictionary_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <locale>
#include "Dictionary.h"
#include "porter2_stemmer.h"

using namespace std;

template<class T>
bool compFunc(T value1, T value2) {
	return value1 < value2;
};

template<class T>
bool equalFunc(T value1, T value2) {
	return value1 == value2;
};

bool equalFunc(string value1, int value2) {
	return false;
};

void strTolower(string& value) {
	transform(value.begin(), value.end(), value.begin(), ::tolower);
}

void strTolower(int& value) {
	// do nothing
}


template<class T>
T* Dictionary<T>::lookup(size_t index) {
	if (items->empty() || index < 0 || index >= items->size()) {
		return NULL;
	} else {
		return &items->at(index);
	}
}

template<class T>
void Dictionary<T>::search(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result) {
	if (sorted) searchWithSorted(value, opType, result);
	else searchWithNoSorted(value, opType, result);
}

template<class T>
void Dictionary<T>::searchWithSorted(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result) {
	if (items->empty()) {
		// return -1 to show no result
		result.push_back(-1);
	} else {
		// find the lower bound for value in vector
		typename vector<T>::iterator lower;
		lower = std::lower_bound(items->begin(), items->end(), value,
				compFunc<T>);

		// based on operator to find exact position in dictionary
		switch (opType) {
		case ColumnBase::equalOp: {
			if (lower != items->end() && equalFunc(*lower, value)) {
				result.push_back(lower - items->begin());
			} else {
				// return -1 to show no result
				result.push_back(-1);
			}
			break;
		}
		case ColumnBase::neOp: {
			int exclusivePosition = -1;
			if (lower != items->end() && equalFunc(*lower, value)) {
				exclusivePosition = lower - items->begin();
			}
			// return all dictionary positions except exclusiveValue
			for (size_t i = 0; i < items->size(); i++) {
				if (i != exclusivePosition) {
					result.push_back(i);
				}
			}
			break;
		}
		case ColumnBase::ltOp: {
			// return positions from 0 to lower
			for (size_t i = 0;
					(lower == items->end()) ?
							i < items->size() : i < (lower - items->begin());
					i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::leOp: {
			unsigned int position = -1;
			if (lower == items->end()) {
				position = items->size();
			} else if (equalFunc(*lower, value)) {
				position = (lower - items->begin()) + 1;
			} else {
				position = lower - items->begin();
			}
			// return from 0 to position
			for (size_t i = 0; i < position; i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::gtOp: {
			unsigned int position = items->size();
			if (lower == items->end()) {
				// all items are less than value
				position = items->size();
			} else if (equalFunc(*lower, value)) {
				position = (lower - items->begin()) + 1;
			} else {
				position = lower - items->begin();
			}
			// return from postion to items.size()
			for (size_t i = position; i < items->size(); i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::geOp: {
			// return from lower to items.size()
			unsigned int i =
					(lower == items->end()) ?
							items->size() : (lower - items->begin());
			for (; i < items->size(); i++) {
				result.push_back(i);
			}
			break;
		}
		case ColumnBase::containOp: {
			// search by inverted index
			struct invertedIndex idxContain;
			strTolower(value);	// lower to compare with index
			Porter2Stemmer::stem(value); // stem to compare with index
			idxContain.word = value;
			typename vector<invertedIndex>::iterator lowerIdx;
			lowerIdx = std::lower_bound(vecIndexLevel0->begin(), vecIndexLevel0->end(), idxContain);
			// found
			if (lowerIdx != vecIndexLevel0->end() && *lowerIdx == idxContain) {
				invertedIndex idx = vecIndexLevel0->at(lowerIdx - vecIndexLevel0->begin());
				result.insert(result.end(), idx.location.begin(), idx.location.end());
				// sort result
				std::sort(result.begin(), result.end());
			}
			break;
		}
		}
	}
}

template<class T>
void Dictionary<T>::searchWithNoSorted(T& value, ColumnBase::OP_TYPE opType, vector<size_t>& result) {
	if (items->empty()) {
		// return -1 to show no result
		result.push_back(-1);
	} else {
		// Search with no sorted dictionary => scan through all items
		for (size_t i = 0; i < items->size(); i++) {
			T dictionaryValue = items->at(i);
			// based on operator to find exact position in dictionary
			switch (opType) {
			case ColumnBase::equalOp: {
				// equal
				if (equalFunc(dictionaryValue, value)) {
					result.push_back(i);
					// return immediately because dictionary has no duplicate
					return;
				}
				break;
			}
			case ColumnBase::neOp: {
				// not equal
				if (!equalFunc(value, dictionaryValue)) {
					result.push_back(i);
				}
				break;
			}
			case ColumnBase::ltOp: {
				// less than
				if (compFunc(dictionaryValue, value)){
					result.push_back(i);
				}
				break;
			}
			case ColumnBase::leOp: {
				// less than or equal = not greater than
				if (!compFunc(value, dictionaryValue)) {
					result.push_back(i);
				}
				break;
			}
			case ColumnBase::gtOp: {
				// greater than
				if (compFunc(value, dictionaryValue)){
					result.push_back(i);
				}
				break;
			}
			case ColumnBase::geOp: {
				// greater than or equal = not less than
				if (!compFunc(dictionaryValue, value)) {
					result.push_back(i);
				}
				break;
			}
			case ColumnBase::containOp: {
				// search by inverted index
				struct invertedIndex idxContain;
				strTolower(value);	// to lower to compare with index
				idxContain.word = value;
				typename vector<invertedIndex>::iterator lowerIdx;
				lowerIdx = std::lower_bound(vecIndexLevel0->begin(), vecIndexLevel0->end(),
						idxContain);
				// found
				if (lowerIdx != vecIndexLevel0->end() && *lowerIdx == idxContain) {
					invertedIndex idx = vecIndexLevel0->at(lowerIdx - vecIndexLevel0->begin());
					result.insert(result.end(), idx.location.begin(), idx.location.end());
					// sort result
					std::sort(result.begin(), result.end());
				}
				break;
			}
			}
		}
	}
}

template<class T>
size_t Dictionary<T>::addNewElement(T& value, vector<size_t>* vecValue, bool sorted, bool bulkInsert) {
	// bulk insert
	if (bulkInsert) bulkVecValue->push_back(value);

	if (items->empty()) {
		items->push_back(value);
		vecValue->push_back(0);
		(*sMap)[value] = 1;
		return 0;
	} else if (!sorted) {
		// check if value existed on dictionary
		if ((*sMap)[value] == 0) {
			items->push_back(value);
			vecValue->push_back(items->size() - 1);
			(*sMap)[value] = vecValue->back() + 1;
		}
		else {
			vecValue->push_back((*sMap)[value] - 1);
		}
		return vecValue->back();
	} else {
		// find the lower bound for value in vector
		typename vector<T>::iterator lower;
		lower = std::lower_bound(items->begin(), items->end(), value,
				compFunc<T>);

		// value existed
		if (lower != items->end() && equalFunc(value, *lower)) {
			// return the position of lower
			long elementPos = lower - items->begin();
			vecValue->push_back(elementPos);
			return elementPos;
		} else {
			// The position of new element in dictionary
			size_t newElementPos = 0L;
			if (lower == items->end()) {
				// insert to the end of dictionary
				newElementPos = items->size();
				items->push_back(value);
				vecValue->push_back(newElementPos);
			} else {
				newElementPos = lower - items->begin();
				// insert into dictionary
				items->insert(lower, value);
				// update (+1) to all elements in vecValue have value >= newElementPos
				if (!bulkInsert) {
					for (int i = 0; i < vecValue->size(); i++) {
						if (vecValue->at(i) >= newElementPos) {
							++vecValue->at(i);
						}
					}
				}
				vecValue->push_back(newElementPos);
			}

			// return the position of new element
			return newElementPos;
		}
	}
}

template<class T>
void Dictionary<T>::sort() {
	std::sort(items->begin(), items->end(), compFunc<T>);
}

template<>
void Dictionary<string>::buildInvertedIndex() {
	// make an unordered_map of words from all items
	vector<string>* strItems = (vector<string>*) items;
	unordered_map<string, vector<size_t>> mapWordsLevel0;
	size_t wordCount = 0;
	for (size_t i = 0; i < strItems->size(); i++) {
		// split item into word by whitespace
		vector<string> words;
		istringstream iss(strItems->at(i));
		copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(words));
		// add to map
		for (size_t j = 0; j < words.size(); j++) {
			string word = words[j];
			// by pass if work length < 4
			if (word.size() < 4) continue;
			// just create inverted index for 'gift'
			if (word.find("gift") == string::npos) continue;
			Porter2Stemmer::trim(word);	// normalize word
			Porter2Stemmer::stem(word);	// stem by porter algorithm
			vector<size_t> locationLevel0 = mapWordsLevel0[word];
			locationLevel0.push_back(i);
			mapWordsLevel0[word] = locationLevel0;
		}
		wordCount += words.size();
	}
	cout << "Total words count: " << wordCount << endl;
	// create vector of inverted index level 0 from map
	for (const auto& m : mapWordsLevel0) {
		string word = m.first;
		vector<size_t> location = m.second;
		// inverted index level 0
		invertedIndex idxLevel0;
		idxLevel0.word = word;
		idxLevel0.location = location;
		vecIndexLevel0->push_back(idxLevel0);
	}
	// sort vector of inverted index
	std::sort(vecIndexLevel0->begin(), vecIndexLevel0->end());
}

template<class T>
size_t Dictionary<T>::size() {
	return items->size();
}

template<class T>
void Dictionary<T>::print(int row) {
	for (int i = 0; i < items->size() && i < row; i++) {
		cout << "Dictionary[" << i << "] = " << items->at(i) << "\n";
	}
}


template class Dictionary<string> ;
template class Dictionary<int> ;

#endif
