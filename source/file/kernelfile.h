// File: kernelfile.h

#pragma once

#include "mutex.h"
#include "part.h"

typedef unsigned long BytesCnt;

class File;

class KernelFile {
public:
	KernelFile();
	~KernelFile();

	char write (BytesCnt b, char* buffer); 
	BytesCnt read (BytesCnt b, char* buffer);
	
	char seek (BytesCnt b);
	char truncate ();
	
	BytesCnt filePos();
	BytesCnt getFileSize ();
	
	char eof ();
	void close();
	void open(int _cache);

	friend class KernelFS;
	friend class File;
private:
	File *parent;
	Mutex mutex;
	DWORD openedByThread;

	int cache;
	bool isOpen;

	unsigned long eofPosition;
	unsigned long eofDataCluster;
	unsigned long eofIndexCluster;

	unsigned long cursorPosition;
	unsigned long cursorDataCluster;
	unsigned long cursorIndexCluster;

	unsigned long firstIndexCluster;
	unsigned long size;
	
	unsigned long entryNo;
	unsigned long entryCluster;
	
	// Private helper methods
	char writeCluster(BytesCnt b, char* buffer);
	char addDataToIndex(unsigned long ClusterNo, unsigned long index);
	unsigned long allocIndexCluster();
	unsigned long getNextIndexCluster();

	unsigned long fillDataCluster(unsigned long ClusterNo, unsigned long b, char *buf);
	unsigned long getCurrentDataCluster();
	unsigned long getNextDataCluster();
	unsigned long filePosition();

	// cache for write
	unsigned long cachePosition;
	char writeCache[ClusterSize];
	char flush();
	// cache for read
	unsigned long cachedCluster;
	char readCache[ClusterSize];
};