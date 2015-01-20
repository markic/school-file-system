// File: kernelfile.cpp

#include "kernelfile.h"
#include "fs.h"
#include "kernelfs.h"

#define MAX_DC 511

extern KernelFS *kernelFS; // fs.cpp

KernelFile::KernelFile() 
{
	eofPosition = eofIndexCluster = firstIndexCluster = 0;
	size = entryNo = entryCluster = cachePosition = cachedCluster = 0;
	eofDataCluster = 1;
	openedByThread = 0;
	parent = 0;
	
	cache = -1;
	isOpen = false;
}

void KernelFile::open(int _cache)
{
	isOpen = true;
	cursorIndexCluster = firstIndexCluster;
	cursorDataCluster = 1;
	cursorPosition = 0;

	openedByThread = GetCurrentThreadId();

	cache = _cache;
}

// Writes 2048 or less and update entry
char KernelFile::flush()
{
	if(!cachePosition) return 1;

	if(!writeCluster(cachePosition, writeCache)){ 
		cursorPosition = eofPosition;
		memset(writeCache,0,ClusterSize);
		cachePosition = 0;
		return 0; // no free
	}
		
	//-------------------- Update Entry -------------------- 
	unsigned long *ui = (unsigned long *)writeCache;
		
	kernelFS->partArr[cache]->readCluster(entryCluster, writeCache);
	int disp = 20 * entryNo + 8; // entry + 2x32b
	ui[disp/4 + 3] = firstIndexCluster;
	ui[disp/4 + 4] = size;
	kernelFS->partArr[cache]->writeCluster(entryCluster, writeCache);	
		
	memset(writeCache,0,ClusterSize);
	cachePosition = 0;
	return 1;
}

//---------------- Helper method for writing 2KB or less ----------------
char KernelFile::writeCluster(BytesCnt b, char* buffer)
{
	unsigned long bytesWritten = 0;

	bool updateEof = false;
	if(filePosition() == size) updateEof = true;
	
	// create data, fill, add to index.
	unsigned long curData = getCurrentDataCluster();
	if(!curData)
	{
		curData = kernelFS->getFreeCluster(cache);
		if(!curData) return 0; // no free

		while(!addDataToIndex(curData, cursorIndexCluster))
			if(!allocIndexCluster()) return 0;
	} 

	bytesWritten += fillDataCluster(curData,b,buffer);
	
	if(cursorPosition == ClusterSize)
	{
		// Data filled
		cursorPosition = 0;
		curData = getNextDataCluster();
		if(cursorDataCluster++ == MAX_DC)
			cursorIndexCluster = allocIndexCluster();	
		if(!curData)
			eofDataCluster = cursorDataCluster;
		
		unsigned long temp = b - bytesWritten; // write in next DC
		if(temp > 0){	
			for(unsigned long i = 0; i < temp; i++) buffer[i] = buffer[bytesWritten++];
			writeCluster(temp, buffer);
		}
	}

	if(updateEof)// if not - overwrite
	{ 
		eofPosition = cursorPosition;
		size = size + b; 
	}
	if(cursorPosition > eofPosition && cursorDataCluster == eofDataCluster)
	{
		size = size + cursorPosition - eofPosition;
		eofPosition = cursorPosition;
	}
	if(cursorDataCluster > eofDataCluster)
	{
		size = size + (cursorDataCluster - eofDataCluster )* ClusterSize - ClusterSize;
		eofDataCluster = cursorDataCluster;
	}
	
	return 1;
}

//-------------------------------- Write -------------------------------
char KernelFile::write(BytesCnt b, char* buffer)
{ 
	mutex.wait();
	if(!isOpen){ mutex.signal(); return 0; }
	if(!b){ mutex.signal(); return 1; }

	if(!firstIndexCluster){
		eofIndexCluster = cursorIndexCluster = firstIndexCluster = kernelFS->getFreeCluster(cache);
		if(!firstIndexCluster) return 0; // no free cluster
	}

	unsigned long globalDisp = 0;
	
	while(1)
	{
		writeCache[cachePosition++] = buffer[globalDisp++];
		if(cachePosition == ClusterSize)
			if(!flush()){ mutex.signal(); return 0; }

		if(globalDisp == b) break;
	}
		
	mutex.signal();
	return 1; 
}

//-------------------------------- Read --------------------------------
BytesCnt KernelFile::read(BytesCnt b, char* buffer)
{ 
	mutex.wait();
	if(!isOpen){ mutex.signal(); return 0; }
	if(!firstIndexCluster){ mutex.signal(); return 0; }
	if(eof()){ mutex.signal(); return 0; }

	unsigned long curData = getCurrentDataCluster();
	if(!curData){ mutex.signal(); return 0; }

	unsigned long globalDisp = 0;

	if(cachedCluster != curData)	
		kernelFS->partArr[cache]->readCluster(cachedCluster = curData, readCache);
			
	while(1)
	{
		if(globalDisp == b) break;	
		buffer[globalDisp++] = readCache[cursorPosition++];
		if(eof()){ mutex.signal(); return globalDisp; }
		
		if(cursorPosition == ClusterSize)
		{ 
			// go to next DC
			curData = getNextDataCluster();
			cursorPosition = 0;

			if(cursorDataCluster++ == MAX_DC)
			{ 
				// go to next IC
				unsigned long t = getNextIndexCluster();
				if(!t){ mutex.signal(); return globalDisp; }
				cursorIndexCluster = t;
				cursorDataCluster = 1;
				curData = getCurrentDataCluster();
				if(!curData){ mutex.signal(); return globalDisp; }
			}

			if(!curData) { mutex.signal(); return globalDisp; }//no more DC
			kernelFS->partArr[cache]->readCluster(cachedCluster = curData, readCache); //next DC
		}
	}	
	
	mutex.signal();
	return globalDisp; 
}

//-------------------------------- Seek --------------------------------
char KernelFile::seek(BytesCnt b)
{
	mutex.wait();
	if(!isOpen){ mutex.signal(); return 0; }
	if(!firstIndexCluster){ mutex.signal(); return 0; }
	if(b > size){ mutex.signal(); return 0; }

	flush();
	
	cursorPosition = b % ClusterSize;
	cursorDataCluster = b / ClusterSize + 1; 
	
	unsigned long curIndex = cursorDataCluster / MAX_DC;
	cursorIndexCluster = firstIndexCluster;
	
	while(curIndex)
	{
		cursorDataCluster -= MAX_DC;
		cursorIndexCluster = getNextIndexCluster();
		curIndex--;
	}

	mutex.signal();
	return 1; 
}

BytesCnt KernelFile::filePos()
{
	mutex.wait();
	if(!isOpen){ mutex.signal(); return 0; }
	if(!firstIndexCluster){ mutex.signal(); return 0; }

	flush();
	unsigned long pos = filePosition();
	
	mutex.signal();
	return pos; 
}

char KernelFile::eof()
{
	mutex.wait();
	
	if(!isOpen){ mutex.signal(); return 1; }
	if(size == filePos()){ mutex.signal(); return 2; } 
	
	mutex.signal();
	return 0; 
}

BytesCnt KernelFile::getFileSize()
{ 
	mutex.wait();
	flush(); 
	mutex.signal();
	return size; 
}

//-------------------------------- Truncate --------------------------------
char KernelFile::truncate()
{
	mutex.wait();
	
	if(!isOpen){ mutex.signal(); return 0; }
	if(!firstIndexCluster){ mutex.signal(); return 0; }
	if(eof()){ mutex.signal(); return 0; }

	char buf[ClusterSize];
	unsigned long *ui = (unsigned long *)buf;

	unsigned long nextIndex;

	// clear all data clusters after cursorData
 	kernelFS->partArr[cache]->readCluster(cursorIndexCluster, buf);
	nextIndex = ui[0];
	ui[0] = 0;

	for(unsigned long i = cursorDataCluster; i < 511; i++)
		if(ui[i+1])
		{
			kernelFS->addFreeCluster(cache,ui[i+1]);
			ui[i+1] = 0;
		}

	kernelFS->partArr[cache]->writeCluster(cursorIndexCluster, buf);

	// clear all index clusters after cursorIndex
	while( nextIndex )
	{	
		unsigned long old = nextIndex;
		kernelFS->partArr[cache]->readCluster(nextIndex, buf);
		nextIndex = ui[0];

		for(unsigned long i = 0; i < 511; i++)
		{
			if(!ui[i+1]) break;	
			kernelFS->addFreeCluster(cache,ui[i+1]);
		}
	
		kernelFS->addFreeCluster(cache, old);
	}

	// clear current Data
	unsigned long b = ClusterSize - cursorPosition;
	char *zeroBuf = new char[b];
	memset(zeroBuf,0, b);

	fillDataCluster(getCurrentDataCluster(), b, buf);
	
	eofPosition = cursorPosition = ClusterSize - b;
	eofDataCluster = cursorDataCluster;
	eofIndexCluster = cursorIndexCluster;

	size = filePosition();

	//-------------------- Update Entry -------------------- 
	kernelFS->partArr[cache]->readCluster(entryCluster, buf);
	int disp = 20 * entryNo + 8; // entry + 2x32b
	ui[disp/4 + 4] = size;
	kernelFS->partArr[cache]->writeCluster(entryCluster, buf);	

	mutex.signal();
	return 1;
}

void KernelFile::close()
{
	mutex.wait();

	flush();
	isOpen = false;
	openedByThread = 0;
	
	kernelFS->semForFormatOrUnmount[cache]->signal();
	kernelFS->semCantDeleteFile[cache]->signal();
	kernelFS->semCantUndeclare[cache]->signal();
	kernelFS->semBlockedOnBanker[cache]->signal();

	mutex.signal();
}
//-------------------------------- Helper Methods --------------------------------
// Adds b from buf to ClusterNo at cursorPosition, returns number of bytes written
unsigned long KernelFile::fillDataCluster(unsigned long ClusterNo, unsigned long b, char *buf)
{
	char data[ClusterSize];
	unsigned long res = 0;
	
	kernelFS->partArr[cache]->readCluster(ClusterNo, data);
	
	while(cursorPosition < ClusterSize && res < b)
		data[cursorPosition++] = buf[res++];
	
	kernelFS->partArr[cache]->writeCluster(ClusterNo, data);
	return res;
}

// Adds ClusterNo to index cluster, returns 0 if there is no space in index
char KernelFile::addDataToIndex(unsigned long ClusterNo, unsigned long index)
{
	char indexBuf[ClusterSize];
	unsigned long *ui = (unsigned long *)indexBuf;

	kernelFS->partArr[cache]->readCluster(index, indexBuf);

	for(int i = 0; i < MAX_DC; i++)
	{
		if(ui[i+1] == ClusterNo) return 2; // already in index
		if(ui[i+1] == 0){
			ui[i+1] = ClusterNo;
			kernelFS->partArr[cache]->writeCluster(index, indexBuf);
			return 1;
		}
	}
	return 0; // no space - alloc new index
}

// Gets next index cluster after cursorIndexCluster or allocates new if next cluster do not exists
// Returns 0 if there are no free clusters
unsigned long KernelFile::allocIndexCluster()
{
	char indexBuf[ClusterSize];
	unsigned long *ui = (unsigned long *)indexBuf;

	unsigned long res = 0;

	kernelFS->partArr[cache]->readCluster(cursorIndexCluster, indexBuf);
	if(!ui[0]) // create new
	{
		res = ui[0] = kernelFS->getFreeCluster(cache);
		if(res == 0) return 0;
		eofDataCluster = cursorDataCluster = 1;
		kernelFS->partArr[cache]->writeCluster(cursorIndexCluster, indexBuf);
		cursorIndexCluster = eofIndexCluster = res;
	}
	else
		cursorIndexCluster = ui[0]; // move to next index - overwrite
	
	cursorDataCluster = 1;
	
	return res;
}

// Gets next index cluster, if exists
unsigned long KernelFile::getNextIndexCluster()
{
	char indexBuf[ClusterSize];
	unsigned long *ui = (unsigned long *)indexBuf;

	kernelFS->partArr[cache]->readCluster(cursorIndexCluster, indexBuf);
	return ui[0];
}

// Gets data cluster at cursorDataCluster
unsigned long KernelFile::getCurrentDataCluster()
{
	if(!cursorDataCluster) return 0;
	if(cursorDataCluster > MAX_DC) return 0;
	
	char indexBuf[ClusterSize];
	unsigned long *ui = (unsigned long *)indexBuf;
	kernelFS->partArr[cache]->readCluster(cursorIndexCluster, indexBuf);

	return ui[cursorDataCluster];
}

// Gets next data cluster at cursorData + 1 in current or next index
unsigned long KernelFile::getNextDataCluster()
{
	char indexBuf[ClusterSize];
	unsigned long *ui = (unsigned long *)indexBuf;
	kernelFS->partArr[cache]->readCluster(cursorIndexCluster, indexBuf);
	
	if(cursorDataCluster != MAX_DC) return ui[cursorDataCluster + 1];

	if(!ui[0]) return 0;

	kernelFS->partArr[cache]->readCluster(ui[0], indexBuf);
	return ui[1];	
}

// Position without flush
unsigned long KernelFile::filePosition()
{
	unsigned long pos = cursorPosition;
	unsigned long curIndex = firstIndexCluster;

	char buf[ClusterSize];
	unsigned long *ui = (unsigned long *)buf;

	while(curIndex)
	{
		if(cursorIndexCluster == curIndex)// add DCs
		{			
			if(cursorDataCluster > 1)
				pos += (cursorDataCluster - 1) * ClusterSize;
			break;
		}
		else
			pos += MAX_DC * ClusterSize; // add full IC

		kernelFS->partArr[cache]->readCluster(curIndex, buf);
		curIndex = ui[0];
	}
	
	return pos; 
}

// Delete file - on FS::delete only
KernelFile::~KernelFile() { }