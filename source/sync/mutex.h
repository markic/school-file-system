// File: mutex.h

#pragma once

#include <windows.h>

class Mutex {
public:
	Mutex();

	void wait();
	void signal();
		
	~Mutex();
	
private:
	CRITICAL_SECTION mutex;
};