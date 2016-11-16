/*
 * Util.h
 *
 *  Created on: Oct 20, 2016
 *      Author: duclv
 *      Description: Utilities functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

#ifndef UTIL_H_
#define UTIL_H_

using namespace std;

class Util {
public:
	// compute the number of row ids selected
	static size_t rowSelectedSize(vector<bool>* vRowId) {
		size_t count = 0;
		for (bool ele : *vRowId) {
			if (ele) ++count;
		}
		return count;
	}

	static inline void printLoading(size_t row) {
		if (row % 800000 == 0) cout << endl;
		if (row % 10000 == 0) cout << ".";
	}
};

#endif /* UTIL_H_ */
