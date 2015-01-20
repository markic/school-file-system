// File: kernelfs.h

#pragma once

#include <map>
#include <utility>
#include <string>

#include "fs.h"
#include "mutex.h"
#include "semaphore.h"

class File;
class KernelFile;
class Partition;

typedef std::map<std::string,KernelFile*> stringFileMap;
typedef std::multimap<std::string, DWORD> stringThreadIDMap;
typedef std::map<std::string, KernelFile*>::iterator stringFileMapIt;
typedef std::multimap<std::string, DWORD>::iterator stringThreadIDMapIt;
typedef std::multimap<DWORD, KernelFile*> threadIdFileMap;
typedef std::multimap<DWORD, KernelFile*>::iterator threadIdFileMapIt;

class KernelFS {
public:
	friend class KernelFile;
	
	KernelFS();
	~KernelFS();
	
	char mount(Partition* partition); 
	char unmount(char part);
	char format(char part);

	char readRootDir(char part, EntryNum n, Directory &d); 
	char doesExist(char* fname);
	
	File* open(char* fname);     
	char deleteFile(char* fname);
	
	char declare(char* fname, int mode);

private:
	Partition *partArr[26];
	stringFileMap FilesHash[26];
	stringThreadIDMap Declared[26];
	
	Mutex globalMutex;
	Mutex *mutex[26];
	Semaphore *semForFormatOrUnmount[26];
	Semaphore *semCantOpenFiles[26];
	Semaphore *semCantDeleteFile[26];
	Semaphore *semCantUndeclare[26];
	Semaphore *semBlockedOnBanker[26];

	bool fileCanBeOpened(std::string hashEntry, int cache);
	bool declaredByOtherThread(std::string hashEntry, int cache);
	bool banker(int cache);

	unsigned long lastEntryCluster[26];
	unsigned long lastEntryPos[26];

	unsigned long getFreeCluster(int cache);
	void addFreeCluster(int cache, unsigned long CluterNo);
};