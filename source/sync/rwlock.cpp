//File: rwlock.cpp

#pragma once

#include "rwlock.h"
#include "semaphore.h"
#include "mutex.h"


RWLock::RWLock():readCount(0), write(1) {}

void RWLock::readLock() {
	mutex.wait();
	if(++readCount == 1) write.wait();
	mutex.signal();

}

void RWLock::readUnlock() {
	mutex.wait();
	if(--readCount == 0) write.signal();
	mutex.signal();

}

void RWLock::writeLock() {	write.wait(); }

void RWLock::writeUnlock() { write.signal(); }

RWLock::~RWLock() {}
