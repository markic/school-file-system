//File: semaphore.h

#pragma once

#include <windows.h>

#define MAX_SEM_INIT_VALUE 30

class Semaphore {
public:
	Semaphore(int initValue = 0);

	void wait();
	void signal(int releaseCount = 1);
	int val() const { return value; }

	~Semaphore();

private:
	HANDLE semaphore;
	int value;
};