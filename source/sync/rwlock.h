//File: rwlock.h

#pragma once

#include "semaphore.h"
#include "mutex.h"

class RWLock {
public:
	RWLock();
	
	void readLock();
	void readUnlock();
	
	void writeLock();
	void writeUnlock();

	~RWLock();

private:
	int readCount;
	Semaphore write;
	Mutex mutex;
};
