/*
 * GarbageCollector.h
 *
 *  Created on: Nov 13, 2016
 *      Author: duclv
 */

#ifndef GARBAGECOLLECTOR_H_
#define GARBAGECOLLECTOR_H_

#include <vector>
#include <thread>
#include <chrono>
#include "Transaction.h"
#include "Table.h"
#include "ColumnBase.h"
#include "Column.h"
#include <stdio.h>

namespace std {

class GarbageCollector {
private:
	// list of recently updated rids
	vector<size_t> recentlyUpdatedRids;
	// Transaction and Table
	Transaction* transaction;
	Table* table;

public:
	GarbageCollector() {
		recentlyUpdatedRids = vector<size_t>(0);
		transaction = NULL;
		table = NULL;
	}
	virtual ~GarbageCollector() {}

	void updateRecentlyUpdateRids(vector<size_t> updateRids) {
		recentlyUpdatedRids.insert(recentlyUpdatedRids.end(), updateRids.begin(), updateRids.end());
	}

	void setTransaction(Transaction* transaction) {
		this->transaction = transaction;
	}

	void setTable(Table * table) {
		this->table = table;
	}

	// run garbage collection
	void run() {
		// if no recently updated rid => stop
		if (recentlyUpdatedRids.size() == 0) {
			return;
		}
		cout << "Garbage Collector running ! with size = " << recentlyUpdatedRids.size() << endl;
		// get active transactions
		vector<size_t> vecActiveTx = transaction->listActiveTransaction();
		// loop through all columns of table to garbage collecting
		for (ColumnBase* colBase : (*table->columns())) {
			if (colBase->getType() == ColumnBase::intType) {
				Column<int>* col = (Column<int>*) colBase;
				// from rid -> update Version space to Data space
				// -> delete old versions
				for (size_t i = 0; i < recentlyUpdatedRids.size(); i++) {
					size_t rid = recentlyUpdatedRids.at(i);
					// update Version space to Data space
					col->updateVersionSpace2DataSpace(rid);
					// check active Transaction to delete old versions
					for (size_t k = 0; k < vecActiveTx.size(); k++) {
						Transaction::transaction tx = transaction->getTransaction(vecActiveTx.at(k));
						vector<size_t> vecRid = tx.vecRid;
						bool beUsing = false;
						for (size_t m = 0; m < vecRid.size(); m++) {
							if (vecRid.at(m) == rid) {
								beUsing = true; break;
							}
						}
						if (beUsing) {
							col->removeOldVersion(rid, tx.startTs);
						}
					}
				}
			}
			else {
				Column<string>* col = (Column<string>*) colBase;
				// from rid -> update Version space to Data space
				// -> delete old versions
				for (size_t i = 0; i < recentlyUpdatedRids.size(); i++) {
					size_t rid = recentlyUpdatedRids.at(i);
					// update Version space to Data space
					col->updateVersionSpace2DataSpace(rid);
					// check active Transaction to delete old versions
					for (size_t k = 0; k < vecActiveTx.size(); k++) {
						Transaction::transaction tx = transaction->getTransaction(vecActiveTx.at(k));
						vector<size_t> vecRid = tx.vecRid;
						bool beUsing = false;
						for (size_t m = 0; m < vecRid.size(); m++) {
							if (vecRid.at(m) == rid) {
								beUsing = true; break;
							}
						}
						if (beUsing) {
							col->removeOldVersion(rid, tx.startTs);
						}
					}
				}
			}
		}
		// clear recently updated rid
		recentlyUpdatedRids.clear();
	}

	void start(int interval)
	{
		std::thread([this, interval]()
		{
			while (true) {
				run();
				std::this_thread::sleep_for(
				std::chrono::milliseconds(interval));
			}
		}).detach();
	}
};

} /* namespace std */

#endif /* GARBAGECOLLECTOR_H_ */
