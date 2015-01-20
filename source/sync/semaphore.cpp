// File: semaphore.cpp

#include "semaphore.h"

Semaphore::Semaphore(int initValue) {
	semaphore = CreateSemaphore(0, value = initValue, MAX_SEM_INIT_VALUE, 0);
}

void Semaphore::wait() {
	value--;
	WaitForSingleObject(semaphore, INFINITE);		
}
void Semaphore::signal(int releaseCount) {
	ReleaseSemaphore(semaphore, releaseCount, 0 );
	value++;
}

Semaphore::~Semaphore() {
	CloseHandle(semaphore);
}
