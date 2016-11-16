/*
 * Transaction.h
 *
 *  Created on: Nov 11, 2016
 *      Author: duclv
 */

#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include <chrono>
#include <vector>
#include <string>
#include "server/ServerSocket.h"

namespace std {

class Transaction {
public:
	enum TRANSACTION_STATUS {WAITING, STARTED, COMMITED, ABORTED};
	struct transaction {
		uint64_t txnId;		// transaction id
		uint64_t csn;		// commit squence number
		uint64_t startTs;	// start time stamp
		TRANSACTION_STATUS status;
		ServerSocket* client;
		vector<size_t> vecRid;
		vector<string> command;
	};
	static vector<transaction>* vecTransaction;
	static vector<size_t>* vecActiveTransaction;
	static vector<size_t>* vecWaitingTransaction;

private:


public:
	Transaction();
	virtual ~Transaction();

	size_t createTx();
	void startTx(size_t txIdx);
	void updateRid2Transaction(size_t txIdx, vector<size_t> vecRid);
	uint64_t getStartTimestamp(size_t txIdx);
	uint64_t getTimestampAsCSN();
	void commitTx(size_t txIdx, uint64_t csn);
	void abortTx(size_t txIdx);
	vector<size_t> listActiveTransaction();
	Transaction::transaction getTransaction(size_t txIdx);
	void addToWaitingList(size_t txIdx);
	vector<size_t> getWaitingList();
	void setClient(size_t txIdx, ServerSocket* client);
	void setCommand(size_t txIdx, vector<string> command);
};

} /* namespace std */

#endif /* TRANSACTION_H_ */
