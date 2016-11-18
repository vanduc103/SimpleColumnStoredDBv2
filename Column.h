/*
 * Column.h
 *
 *  Created on: Sep 23, 2016
 *      Author: duclv
 */
#include <vector>

#ifndef COLUMN_H_
#define COLUMN_H_

#include <math.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include "ColumnBase.h"
#include "Dictionary.h"
#include "PackedArray.h"

namespace std {

template<typename T>
class Column : public ColumnBase {
private:
	// value vector for column
	vector<size_t>* vecValue;

	// bit packing array
	PackedArray* packed;

	// dictionary vector for column
	Dictionary<T>* dictionary;

	// bulk insert ?
	bool bulkInsert = false;

	// DATA SPACE
	struct data_column {
		bool versionFlag;
		size_t encodedValueIdx;	// index to encodedValue in vecValue vector
		uint64_t csn;
	};
	vector<data_column>* dataColumn;

	// VERSION SPACE
	vector<size_t>* versionVecValue;
	struct version_column {
		size_t encodedValueIdx;	// index to encodedValue in versionVecValue vector
		uint64_t csn;
		version_column* next;
	};
	vector<version_column>* versionColumn;
	Dictionary<T>* deltaSpace;
	map<size_t, size_t>* hashtable;
public:
	Column() {
		dictionary = new Dictionary<T>();
		vecValue = new vector<size_t>();
		packed = new PackedArray();
		dataColumn = new vector<data_column>();
		versionVecValue = new vector<size_t>();
		versionColumn = new vector<version_column>();
		deltaSpace = new Dictionary<T>();
		hashtable = new map<size_t, size_t>();
	}
	virtual ~Column() {
		delete vecValue;
		delete dictionary;
		PackedArray_destroy(packed);
		delete dataColumn;
		delete versionVecValue;
		delete versionColumn;
		delete deltaSpace;
		delete hashtable;
	}

	vector<size_t>* getVecValue() {
		if (vecValue == NULL) {
			vecValue = new vector<size_t>();
		}
		vecValue->clear();
		for (int i = 0; i < packed->count; i++) {
			vecValue->push_back(PackedArray_get(packed, i));
		}
		return vecValue;
	}

	size_t vecValueSize() {
		return packed->count;
	}

	size_t numOfRows() {
		return packed->count;
	}

	size_t vecValueAt(size_t index) {
		if (index < 0 || index >= packed->count) {
			return -1; // indicate no result
		}
		return PackedArray_get(packed, index);
	}

	void updateVecValueAt(size_t index, size_t value) {
		if (index < 0 || index >= packed->count) {
			return; // no update
		}
		// update by bit backing
		PackedArray_set(packed, index, value);
	}

	void printVecValue(int row) {
		vecValue = getVecValue();
		for (size_t i = 0; i < (*vecValue).size() && i < row; i++) {
			cout << "vecValue[" << i << "] = " << (*vecValue)[i] << "\n";
		}
	}

	void bitPackingVecValue() {
		// #bit to represent encode dictionary value
		size_t numOfBit = (size_t) ceil(log2((double) dictionary->size()));
		// init bit packing array
		packed = PackedArray_create(numOfBit, vecValue->size());

		for (size_t i = 0; i < vecValue->size(); i++) {
			size_t value = vecValue->at(i);
			PackedArray_set(packed, i, value);
		}
		// free vecValue
		vecValue->resize(0);
	}

	vector<size_t>* unpackingVecValue() {
		if (vecValue == NULL) {
			vecValue = new vector<size_t>();
		}
		vecValue->clear();
		for (int i = 0; i < packed->count; i++) {
			vecValue->push_back(PackedArray_get(packed, i));
		}
		return vecValue;
	}

	Dictionary<T>* getDictionary() {
		if (dictionary == NULL) {
			dictionary = new Dictionary<T>();
		}
		return dictionary;
	}

	// Update new value for dictionary
	void updateDictionary(T& value, bool sorted = true, bool bulkInsert = true, uint64_t csn = 0) {
		// no bulk insert if no sort
		if (!sorted) bulkInsert = false;
		this->bulkInsert = bulkInsert;

		dictionary->setSorted(sorted);
		dictionary->addNewElement(value, vecValue, sorted, bulkInsert);
		if (!bulkInsert) {
			// build dataColumn vector
			data_column data;
			data.encodedValueIdx = vecValue->size() - 1;
			data.csn = csn;
			data.versionFlag = false;
			dataColumn->push_back(data);
		}
	}

	bool isBulkInsert() {
		return bulkInsert;
	}

	void bulkBuildVecVector(uint64_t csn = 0) {
		// bulk insert -> update vecValue after building entire dictionary
		vecValue->resize(0);
		vector<T>* bulkVecValue = dictionary->getBulkVecValue();
		if (bulkVecValue != NULL) {
			for (size_t i = 0; i < bulkVecValue->size(); i++) {
				// find position of valueId in dictionary
				vector<size_t> result;
				dictionary->search(bulkVecValue->at(i), ColumnBase::equalOp, result);
				size_t pos = result[0];
				if (pos != -1) vecValue->push_back(pos);
			}
			// build dataColumn vector
			dataColumn->resize(0);
			for (size_t i = 0; i < vecValue->size(); i++) {
				data_column data;
				data.encodedValueIdx = i;
				data.csn = csn;
				data.versionFlag = false;
				dataColumn->push_back(data);
			}
		}
		bulkVecValue->resize(0);
	}

	void createInvertedIndex() {
		if (dictionary != NULL)
			dictionary->buildInvertedIndex();
	}

	bool selection(T& searchValue, ColumnBase::OP_TYPE q_where_op,
					vector<bool>* q_resultRid, bool initResultRid = true) {
		// init q_resultRid to all true
		if (initResultRid)
			for (size_t i = 0; i < numOfRows(); i++) {
				q_resultRid->push_back(true);
			}
		vector<size_t> result;
		this->getDictionary()->search(searchValue, q_where_op, result);

		// find rowId with appropriate dictionary position
		for (size_t rowId = 0; !result.empty() && rowId < this->vecValueSize(); rowId++) {
			size_t dictPosition = this->vecValueAt(rowId);
			if ((q_where_op != ColumnBase::containOp && dictPosition >= result.front() && dictPosition <= result.back())
				|| (q_where_op == ColumnBase::containOp && binary_search(result.begin(), result.end(), dictPosition))) {
				// do nothing, keep q_resultRid true
			}
			else {
				// update to false -> not in result
				q_resultRid->at(rowId) = false;
			}
		}
		return true;
	}

	vector<T> projection(vector<bool>* q_resultRid, size_t limit, size_t& limitCount) {
		vector<T> outputs; // output result
		limitCount = 0; // reset limit count
		for (size_t rid = 0; rid < q_resultRid->size(); rid++) {
			if (q_resultRid->at(rid)) {
				size_t encodeValue = this->vecValueAt(rid);
				T* a = this->getDictionary()->lookup(encodeValue);
				outputs.push_back(*a);
				if (++limitCount >= limit) break;
			}
		}

		return outputs;
	}

	vector<T> projection(vector<int>* q_resultRid, size_t limit, size_t& limitCount) {
		vector<T> outputs; // output result
		limitCount = 0; // reset limit count
		for (size_t i = 0; i < q_resultRid->size(); i++) {
			size_t encodeValue = this->vecValueAt(q_resultRid->at(i));
			T* a = this->getDictionary()->lookup(encodeValue);
			outputs.push_back(*a);
			if (++limitCount >= limit) break;
		}

		return outputs;
	}

	// Build hashmap of valueId based on selected row ids
	void buildHashmap(map<size_t, vector<size_t>>& hashmap, vector<bool>* vecRowId) {
		hashmap.clear();
		for (size_t rowId = 0; rowId < vecRowId->size(); rowId++) {
			// get valueId from bit packing if row id is selected
			// then build hashmap
			if (vecRowId->at(rowId)) {
				size_t valueId = vecValueAt(rowId);
				hashmap[valueId].push_back(rowId);
			}
		}
	}

	// Return vector of matching row ids
	vector<size_t> probe(map<size_t, vector<size_t>>* hashmap, size_t probedValue) {
		if (hashmap != NULL) {
			try {
				return hashmap->at(probedValue);
			} catch (exception& e) {
				// empty vector
				return vector<size_t>();
			}
		}
		return vector<size_t>();
	}

	// DATA SPACE
	void insertDataVecValue(T&value, uint64_t csn) {
		// uncompress vecValue vector from bit packing
		vecValue = unpackingVecValue();
		// add new value to dictionary
		bool sorted = this->getType() == ColumnBase::intType;
		dictionary->addNewElement(value, vecValue, sorted, false);
		// get index of new insert value to vecValue vector
		size_t newInsertVecValueIdx = vecValue->size() - 1;
		// bit packing vecValue again
		bitPackingVecValue();
		// create new data space value
		data_column newData;
		newData.encodedValueIdx = newInsertVecValueIdx;
		newData.csn = csn;
		newData.versionFlag = false;
		dataColumn->push_back(newData);
	}

	uint64_t getCSN(size_t rid) {
		try {
			return dataColumn->at(rid).csn;
		} catch (out_of_range& e) {
			return 0;
		}
	}

	void setCSN(size_t rid) {
		try {
			data_column data = dataColumn->at(rid);
			data.csn = UINT64_MAX;
			dataColumn->at(rid) = data;
		} catch (out_of_range& e) {
			// nothing
		}
	}

	// VERSION SPACE
	void addVersionVecValue(T& value, uint64_t csn, size_t rid) {
		// set maximum csn so that another transaction cannot update
		this->setCSN(rid);
		// add to delta space and version vector (start from last dictionary position)
		bool sorted = dictionary->getSorted();
		deltaSpace->addNewElement(value, versionVecValue, sorted, false);
		// get index of encodedValue in versionVecValue
		size_t encodedValueIdx = versionVecValue->size() - 1;
		// create new version
		version_column newVersion;
		newVersion.encodedValueIdx = encodedValueIdx;
		newVersion.next = NULL;
		newVersion.csn = csn;
		// check previous version on hash table
		int preVersionIdx = -1;
		try {
			preVersionIdx = hashtable->at(rid);
		} catch (out_of_range& e) {
			// not existed on hash table, keep -1
			preVersionIdx = -1;
		}
		if (preVersionIdx >= 0) {
			version_column preVersion = versionColumn->at(preVersionIdx);
			// point the next pointer of new created version to previous version
			newVersion.next = &preVersion;
			// replace the previous version on Version space vector by new version
			versionColumn->at(preVersionIdx) = newVersion;
		} else {
			// add new version to Version space vector
			versionColumn->push_back(newVersion);
			// create a new entry for rid on Hash table
			(*hashtable)[rid] = versionColumn->size() - 1;
		}
		// update version_flag & new csn on DATA space
		data_column dataValue = dataColumn->at(rid);
		dataValue.versionFlag = true;
		dataValue.csn = csn + 200;
		dataColumn->at(rid) = dataValue;
	}

	vector<T> projectionWithVersion(vector<bool>* q_resultRid, uint64_t txTs,
			size_t limit, size_t& limitCount) {
		vector<T> outputs; // output result
		limitCount = 0; // reset limit count
		for (size_t rid = 0; rid < q_resultRid->size(); rid++) {
			if (q_resultRid->at(rid)) {
				// get data at rid
				data_column data = dataColumn->at(rid);
				// if has no version -> get value from data space
				if (!data.versionFlag) {
					// check txTs >= CSN
					if (txTs >= data.csn) {
						size_t vecValueIdx = data.encodedValueIdx;
						size_t dictIdx = this->vecValueAt(vecValueIdx);
						T* a = this->getDictionary()->lookup(dictIdx);
						if (a != NULL) {
							outputs.push_back(*a);
						}
					}
				}
				// has version-> get value from version space
				else {
					// get version index from hashtable
					int versionIdx = -1;
					try {
						versionIdx = hashtable->at(rid);
					} catch (exception& e) {
						// not existed rid in hashtable, do nothing
					}
					// get version from Version space
					if (versionIdx >= 0 && versionIdx < versionColumn->size()) {
						version_column versionData = versionColumn->at(
								versionIdx);
						// traverse all versions from newest to oldest to find version with CSN <= txTs
						while (versionData.csn > txTs
								&& versionData.next != NULL) {
							versionData = *versionData.next;
						}
						if (txTs >= versionData.csn) {
							size_t versionVecValueIdx = versionData.encodedValueIdx;
							size_t dictIdx = this->versionVecValue->at(
									versionVecValueIdx);
							// lookup in Delta space
							T* a = deltaSpace->lookup(dictIdx);
							if (a != NULL) {
								outputs.push_back(*a);
							}
						}
					}
				}
				// get maximum limitCount result
				if (++limitCount >= limit)
					break;
			}
		}

		return outputs;
	}

	void updateVersionSpace2DataSpace(size_t rid) {
		try {
			// get latest version of rid from hash table
			size_t versionIdx = hashtable->at(rid);
			version_column lastestVersion = versionColumn->at(versionIdx);
			size_t encodedValue = versionVecValue->at(lastestVersion.encodedValueIdx);
			// get dictionary value from delta space
			T* a = deltaSpace->lookup(encodedValue);
			// update this value into column's dictionary
			vecValue = unpackingVecValue();
			size_t newEncodedValue = dictionary->addNewElement(*a, vecValue, dictionary->getSorted(), false);
			vecValue->pop_back();	// remove last item
			bitPackingVecValue();
			// get data at rid
			data_column dataAtRid = dataColumn->at(rid);
			// update vecValue and data at rid
			updateVecValueAt(rid, newEncodedValue);
			dataAtRid.csn = lastestVersion.csn;	// lastest update csn
			dataAtRid.encodedValueIdx = rid;	// index to vecValue
			dataAtRid.versionFlag = true;
			dataColumn->at(rid) = dataAtRid;
		} catch (exception& e) {
			//do nothing
		}
	}

	void removeOldVersion(size_t rid, uint64_t txStartTs) {
		try {
			// get latest version of rid from hashtable
			size_t versionIdx = hashtable->at(rid);
			version_column version = versionColumn->at(versionIdx);
			// check if history version has csn < txStartTs then remove
			version_column* curVersion = &version;
			while (version.csn >= txStartTs && version.next != NULL) {
				curVersion = &version;
				version = *version.next;
			}
			// remove old version
			if (version.csn < txStartTs) {
				vector<version_column*> vecOld;
				while (version.next != NULL) {
					vecOld.push_back(version.next);
				}
				for (version_column* old : vecOld) {
					delete old;
				}
				curVersion->next = NULL;
			}
		} catch (exception& e) {
			//do nothing
		}
	}

};

} /* namespace std */

#endif /* COLUMN_H_ */
