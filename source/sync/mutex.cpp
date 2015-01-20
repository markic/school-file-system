// File: mutex.cpp

#include "mutex.h"

Mutex::Mutex() {
	InitializeCriticalSection(&mutex);
}
	 
void Mutex::wait() {
	EnterCriticalSection(&mutex);
}

void Mutex::signal() {
	LeaveCriticalSection(&mutex);
}
	  
Mutex::~Mutex() {
	DeleteCriticalSection(&mutex); 
}
