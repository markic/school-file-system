// File: kernelfs.cpp

#include "fs.h"
#include "kernelfs.h"
#include "file.h"
#include "kernelfile.h"
#include "part.h"

KernelFS::KernelFS()
{
	for(int i = 0; i < 26; i++) 
		partArr[i] = 0;	
}

char KernelFS::mount(Partition* partition)
{ 
	globalMutex.wait();
	
	for(int i = 0; i < 26; i++)
		if(partArr[i] == 0)
		{		
			mutex[i] = new Mutex();
			globalMutex.signal();
			mutex[i]->wait();
			
			partArr[i] = partition;			
			semForFormatOrUnmount[i] = new Semaphore(0);
			semCantOpenFiles[i] = new Semaphore(0);
			semCantDeleteFile[i] = new Semaphore(0);
			semCantUndeclare[i] = new Semaphore(0);
			semBlockedOnBanker[i] = new Semaphore(0);	
	
			// fill map, create file, find lastEntry Cluster:Position - read from disk	
			char buf[ClusterSize];
			unsigned long *ui = (unsigned long*) buf;
			lastEntryCluster[i] = 0;		

			while(1)
			{
				partArr[i]->readCluster(lastEntryCluster[i], buf);
				
				for(lastEntryPos[i] = 0; lastEntryPos[i] < 102; lastEntryPos[i]++)
				{
					int disp = 20 * lastEntryPos[i] + 8; // entry + 2x32b
					if(ui[disp/4] == 0) break;
			
					KernelFile *kf = new KernelFile();			
					std::string hashEntry;
					
					for(int j = 0; j < 11; j++)
						if(buf[disp++]) hashEntry += buf[disp-1];
					
					FilesHash[i].insert(std::pair<std::string,KernelFile*>(hashEntry,kf));
					
					kf->cache = i; disp++;
					kf->firstIndexCluster = ui[disp/4];
					kf->size = ui[disp/4 + 1];
					
					kf->entryNo = lastEntryPos[i];
					kf->entryCluster = lastEntryCluster[i];
					
					kf->eofIndexCluster = kf->firstIndexCluster;
					kf->eofDataCluster = (kf->size / ClusterSize + 1) % 511;
					kf->eofPosition = kf->size % ClusterSize;

				} // end of finding entry for

				if(ui[0]) lastEntryCluster[i] = ui[0];
				break;
			} // end of while				

			mutex[i]->signal();
			return (char)(i + 65); 
		} // end of if(partArr[i] == 0) 
	
	globalMutex.signal();
	return 0; // no free letter?
}

char KernelFS::unmount(char part)
{
	globalMutex.wait();
	int cache = ((int)part - 65);
	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists
	globalMutex.signal();
	mutex[cache]->wait();
	
	// block until all files are closed and undeclared
	for(stringFileMapIt it = FilesHash[cache].begin(); it != FilesHash[cache].end(); it++)
	{
			if(it->second->isOpen || !Declared[cache].empty())
			{ 	
				delete semForFormatOrUnmount[cache];
				semForFormatOrUnmount[cache] = new Semaphore(0);
				
				mutex[cache]->signal(); 
				semForFormatOrUnmount[cache]->wait();
				mutex[cache]->wait(); 
				
				it = FilesHash[cache].begin(); 
			}
	}
	
	partArr[cache] = 0;
	lastEntryCluster[cache] = lastEntryPos[cache] = 0;
	FilesHash[cache].clear();

	semCantOpenFiles[cache]->signal();
	mutex[cache]->signal();
	return 1;
}

char KernelFS::format(char part)
{
	globalMutex.wait();
	int cache = ((int)part - 65);
	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists
	globalMutex.signal();
	mutex[cache]->wait();
	
	// block until all files are closed and undeclared
	for(stringFileMapIt it = FilesHash[cache].begin(); it != FilesHash[cache].end(); it++)
	{
		if(it->second->isOpen || !Declared[cache].empty())
		{ 
			delete semForFormatOrUnmount[cache];
			semForFormatOrUnmount[cache] = new Semaphore(0);

			mutex[cache]->signal(); 
			semForFormatOrUnmount[cache]->wait();
			mutex[cache]->wait(); 

			it = FilesHash[cache].begin(); 
		}
	}	

	lastEntryCluster[cache] = lastEntryPos[cache] = 0;
	FilesHash[cache].clear();

	// init root
	char buf[ClusterSize] = "";
	unsigned long *ui = (unsigned long *)buf;
	ui[0] = 0;
	ui[1] = 1;
	
	partArr[cache]->writeCluster(0, buf);

	// init all free pointers	
	ui[1] = 0;
	for(unsigned long i = 1; i < partArr[cache]->getNumOfClusters()-1; i++) {
		ui[0] = i+1;
		partArr[cache]->writeCluster(i, buf);
	}
	
	ui[0] = 0;
	partArr[cache]->writeCluster(partArr[cache]->getNumOfClusters()-1, buf);

	semCantOpenFiles[cache]->signal();
	mutex[cache]->signal();
	return 1; 
}

char KernelFS::readRootDir(char part, EntryNum n, Directory &d)
{
	globalMutex.wait();
	int cache = ((int)part - 65);
	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists
	globalMutex.signal();
	mutex[cache]->wait();

	unsigned long startCluster = 0; // root
	int count = 0; // number of entries read
	
	char buf[ClusterSize];
	unsigned long *ui = (unsigned long*) buf;

	// find cluster with n-th entry
	for(unsigned long i = 0; i < n / 102; i++)
	{	
		partArr[cache]->readCluster(startCluster, buf);
		startCluster = ui[0];
		if(startCluster == 0){ mutex[cache]->signal(); return 0; }
	} 
                        
	while(count < 64)
	{	
		partArr[cache]->readCluster(startCluster, buf);

		for(unsigned long i = n; i < 102 *(startCluster + 1); i++)
		{
			Entry e;
			int disp = 20 * i + 8; // entry + 2x32b

			for(int j = 0; j < 8; j++)
				e.name[j] = buf[disp++]; // 8, 28

			if(e.name[0] == 0x00) continue;

			e.ext[0] = buf[disp++];
			e.ext[1] = buf[disp++];
			e.ext[2] = buf[disp++]; // 18, 38
			e.reserved = 0x00; ++disp; // 20, 40
			e.firstIndexCluster = ui[disp/4]; // 5,10
			e.size = ui[disp/4 + 1]; // 6,11

			d[count++] = e;;
			if(count == 64) break;
		}
		if(count == 64) break;
			
		startCluster = ui[0];
		if(startCluster == 0) break;
	}
	
	mutex[cache]->signal();
	return count; 
}

char KernelFS::doesExist(char* fname)
{
	globalMutex.wait();
	int cache = ((int)fname[0] - 65);
	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists
	globalMutex.signal();
	mutex[cache]->wait();
	
	// forming filename
	char filename[9] = "";
	int i;
	for(i = 0; i < 8; i++)
	{
		filename[i] = fname[i+3];
		if(filename[i] == '.'){
			filename[i] = '\0';
			break;
		}
	}

	// forming extension
	char ext[4] = "";
	ext[0] = fname[++i + 3];
	ext[1] = fname[++i + 3];
	ext[2] = fname[++i + 3];
	ext[3] = '\0';

	std::string hashEntry(filename);
	hashEntry += ext;

	if(FilesHash[cache].find(hashEntry) != FilesHash[cache].end())
	{
		mutex[cache]->signal(); 
		return 1;
	}
	
	mutex[cache]->signal(); 
	return 0;
}

char KernelFS::declare(char* fname, int mode)
{
	globalMutex.wait();
	int cache = ((int)fname[0] - 65);
	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists
	globalMutex.signal();
	mutex[cache]->wait();

	// forming filename
	char filename[9] = "";
	int i;
	for(i = 0; i < 8; i++){
		filename[i] = fname[i+3];
		if(filename[i] == '.'){
			filename[i] = '\0';
			break;
		}
	}

	// forming extension
	char ext[4] = "";
	ext[0] = fname[++i + 3];
	ext[1] = fname[++i + 3];
	ext[2] = fname[++i + 3];
	ext[3] = '\0';

	std::string hashEntry(filename);
	hashEntry += ext;
	
	if(mode)// declare
	{
		Declared[cache].insert(std::pair<std::string, DWORD>(hashEntry,GetCurrentThreadId()));
	}
	else
	{ 
		// block if file is not closed
		for(std::map<std::string, KernelFile*>::iterator it = FilesHash[cache].begin(); it != FilesHash[cache].end(); it++)
			if(it->first == hashEntry)
				if(it->second->isOpen)
				{ 
					delete semCantUndeclare[cache];
					semCantUndeclare[cache] = new Semaphore(0);
					mutex[cache]->signal(); 
					semCantUndeclare[cache]->wait();
					mutex[cache]->wait(); 
					it = FilesHash[cache].begin();
				}						
		
		// file is not declared anymore
		for(std::multimap<std::string, DWORD>::iterator it = Declared[cache].begin(); it != Declared[cache].end();){
			if(it->first == hashEntry && it->second == GetCurrentThreadId())
				Declared[cache].erase(it++);	
			else
				++it;
		}
		
		semForFormatOrUnmount[cache]->signal();
		semCantDeleteFile[cache]->signal();

	}
	mutex[cache]->signal();
	return 1; 
} 

File* KernelFS::open(char* fname)
{ 
	globalMutex.wait();
	int cache = ((int)fname[0] - 65);
	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists
	
	while(semForFormatOrUnmount[cache]->val() < 0) // need to format or unmount
	{ 
		delete semCantOpenFiles[cache];
		semCantOpenFiles[cache] = new Semaphore(0);
		globalMutex.signal();
		semCantOpenFiles[cache]->wait();
		globalMutex.wait();
	}

	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists after unmount
	
	globalMutex.signal();
	mutex[cache]->wait();
	
	
	// forming filename
	char filename[9] = "";
	int i;
	for(i = 0; i < 8; i++)
	{
		filename[i] = fname[i+3];
		if(filename[i] == '.'){
			filename[i] = '\0';
			break;
		}
	}
	
	// forming extension
	char ext[4] = "";
	ext[0] = fname[++i + 3];
	ext[1] = fname[++i + 3];
	ext[2] = fname[++i + 3];
	ext[3] = '\0';

	std::string hashEntry(filename);
	hashEntry += ext;

	// Check if file is declared by current thread
	if (!fileCanBeOpened(hashEntry, cache)) 
	{ 
		mutex[cache]->signal(); 
		return 0; 
	}

	// Find if file is created already
	for(stringFileMapIt it = FilesHash[cache].begin(); it != FilesHash[cache].end(); it++)
		if(it->first == hashEntry)
		{
			// found - open if banker say it can be
			delete semBlockedOnBanker[cache];
			semBlockedOnBanker[cache] = new Semaphore(0);
			KernelFile* kf = it->second;
			
			mutex[cache]->signal();	
			while(kf->isOpen) Sleep(5);
			mutex[cache]->wait(); 
			
			File *file = new File();
			file->myImpl = kf;
			file->myImpl->parent = file;
			
			file->myImpl->open(cache);

			while(!banker(cache))
			{ 
				mutex[cache]->signal();	
				semBlockedOnBanker[cache]->wait();
				mutex[cache]->wait();	
			}
			mutex[cache]->signal();
			return it->second->parent;	
		}		

	
	//-------------------- Create File -------------------- 
	File *newFile = new File();     
	KernelFile *kf = newFile->myImpl = new KernelFile();
	newFile->myImpl->parent = newFile;
	newFile->myImpl->open(cache);

	FilesHash[cache].insert(std::pair<std::string,KernelFile*>(hashEntry,kf));

	// find free entry on root
	char buf[ClusterSize];
	unsigned long *ui = (unsigned long *)buf;
	unsigned long root = lastEntryCluster[cache]; // current searching free entry cluster
	bool entryFound = false;

	while(entryFound == false)
	{
		partArr[cache]->readCluster(root, buf);
	
		for(unsigned long i = 0; i < 102; i++)
		{
			int disp = 20 * i + 8; // entry + 2x32b

			if(buf[disp] == 0x00) // free entry found
			{		
				lastEntryCluster[cache] = root;
				lastEntryPos[cache] = i;
				
				for(int j = 0; j < 8; j++)
					buf[disp++] = filename[j];

				buf[disp++] = ext[0];
				buf[disp++] = ext[1];
				buf[disp++] = ext[2];
				disp++;
				ui[disp/4] = 0; // firstIndexCluster
				ui[disp/4 + 1] = 0; // size

				entryFound = true;
				break;
			} // if entry found end
		} // passed through whole cluster
	
		if(!entryFound)
		{ 
			if(ui[0]){ root = ui[0]; continue; }
			root = getFreeCluster(cache);
			if(!root) { mutex[cache]->signal(); return 0; }
			continue; 
		}
		
		partArr[cache]->writeCluster(root, buf);

	} // end of while

	kf->entryCluster = root;
	kf->entryNo = lastEntryPos[cache];
	
	mutex[cache]->signal();
	return newFile; 
}

char KernelFS::deleteFile(char* fname)
{ 
	globalMutex.wait();
	int cache = ((int)fname[0] - 65);
	if(!partArr[cache]){ globalMutex.signal(); return 0; } // does not exists
	globalMutex.signal();
	mutex[cache]->wait();

	// forming filename
	char filename[9] = "";
	int i;
	for(i = 0; i < 8; i++)
	{
		filename[i] = fname[i+3];
		if(filename[i] == '.'){
			filename[i] = '\0';
			break;
		}
	}

	// forming extension
	char ext[4] = "";
	ext[0] = fname[++i + 3];
	ext[1] = fname[++i + 3];
	ext[2] = fname[++i + 3];
	ext[3] = '\0';

	std::string hashEntry(filename);
	hashEntry += ext;

	// block while file is open or declared by some thread except this
	for(stringFileMapIt it = FilesHash[cache].begin(); it != FilesHash[cache].end(); it++)
		if(it->first == hashEntry)
			if(it->second->isOpen || declaredByOtherThread(hashEntry, cache))
			{ 
				delete semCantDeleteFile[cache];
				semCantDeleteFile[cache] = new Semaphore(0);
				mutex[cache]->signal(); 
				semCantDeleteFile[cache]->wait();
				mutex[cache]->wait(); 

				it = FilesHash[cache].begin(); 
			}
	
	declare(fname,0); // undeclare deleted file for this thread
	KernelFile *kf;

	//-------------------- Remove From Hash -------------------- 
	for(stringFileMapIt it = FilesHash[cache].begin(); it != FilesHash[cache].end();)
	{
		if(it->first == hashEntry)
		{
			kf = it->second;
			FilesHash[cache].erase(it++);
		}		
		else
			++it;
	}

	//------------------------ Find Entry ------------------------
	unsigned long entryCluster = kf->entryCluster; 
	unsigned long entryNo = kf->entryNo;

	char buf[ClusterSize];
	unsigned long *ui = (unsigned long*) buf;
	partArr[cache]->readCluster(entryCluster, buf);

	//------------------------ Compact --------------------------- 
	if(lastEntryPos[cache] == entryNo){ // there is just one entry
		int disp = 20 * entryNo + 8;
		for(int k = 0; k < 20; k++)
			buf[disp++] = 0;

		partArr[cache]->writeCluster(entryCluster, buf);
	}
	else
	{
		int disp1 = 20 * entryNo + 8;
		int disp2 = 20 * lastEntryPos[cache] + 8;
		
		if(lastEntryCluster[cache] == entryCluster) // last and toDelete in same cluster
		{ 	
			for(int k = 0; k < 20; k++)
			{
				buf[disp1++] = buf[disp2];
				buf[disp2++] = 0;
			}

		}
		else
		{
			char lastClusterBuf[ClusterSize] = "";
			partArr[cache]->readCluster(lastEntryCluster[cache], lastClusterBuf);

			for(int k = 0; k < 20; k++)
			{
				buf[disp1++] = lastClusterBuf[disp2];
				lastClusterBuf[disp2++] = 0;
			}
			partArr[cache]->writeCluster(lastEntryCluster[cache], lastClusterBuf);
		}
			
		partArr[cache]->writeCluster(entryCluster, buf);
	}


	// free last root cluster
	if(lastEntryCluster[cache] && !lastEntryPos[cache])
	{
		addFreeCluster(cache, lastEntryCluster[cache]);

		while(1)
		{
			if(ui[0] == 0) break;
			partArr[cache]->readCluster(entryCluster, buf);

			if(ui[0] == lastEntryCluster[cache])
			{
				lastEntryCluster[cache] = ui[0];
				ui[0] = 0;
				partArr[cache]->writeCluster(entryCluster, buf);
				break;
			}
			else entryCluster = ui[0];
		}
	}
	
	if( lastEntryPos[cache]-- == 0) lastEntryPos[cache] = 0;

	//-------------------- Free Index and Data Clusters --------------------
	unsigned long index = kf->firstIndexCluster;
	while(index)
	{
		partArr[cache]->readCluster(index, buf);

		for(unsigned long i = 0; i < 511; i++)
			if(ui[i+1]) 
				addFreeCluster(cache,ui[i+1]);
			
		unsigned long temp = ui[0];
		addFreeCluster(cache,index);
		index = temp;
	}

	delete kf;
	mutex[cache]->signal();
	return 1; 
}

//-------------------- Private Helper Methods -------------------- 

unsigned long KernelFS::getFreeCluster(int cache)
{
	unsigned long res = 0;
	unsigned long temp = 0;
	
	char root[ClusterSize];
	char free[ClusterSize];
	unsigned long *uiRoot = (unsigned long *)root;
	unsigned long *uiFree = (unsigned long *)free;
	
	partArr[cache]->readCluster(0, root);
	res = uiRoot[1];

	if(res == 0) return 0; // no more free

	partArr[cache]->readCluster(res, free);
	temp = uiFree[0];
	uiFree[0] = 0x00;
	uiRoot[1] = temp;

	partArr[cache]->writeCluster(res, free);
	
	partArr[cache]->writeCluster(0, root);
	return res;
}

void KernelFS::addFreeCluster(int cache, unsigned long ClusterNo)
{
	unsigned long res = 0;
	
	char root[ClusterSize];
	char newFree[ClusterSize] = "";
	unsigned long *uiRoot = (unsigned long *)root;
	unsigned long *uiNewFree = (unsigned long *)newFree;

	partArr[cache]->readCluster(0, root);
	uiNewFree[0] = uiRoot[1];
	uiRoot[1] = ClusterNo;
	
	partArr[cache]->writeCluster(ClusterNo, newFree);
	partArr[cache]->writeCluster(0, root);
}

// Synchronization 
// Check if file is declared by thread which calls open
bool KernelFS::fileCanBeOpened(std::string hashEntry, int cache)
{
	for(stringThreadIDMapIt it = Declared[cache].begin(); it != Declared[cache].end(); it++)
		if(it->first == hashEntry)
			if(it->second == GetCurrentThreadId()) return true;
		
	return false; // not declared
}

// Check if some other thread has declared this file
bool KernelFS::declaredByOtherThread(std::string hashEntry, int cache)
{
	for(stringThreadIDMapIt it = Declared[cache].begin(); it != Declared[cache].end(); it++)
		if(it->first == hashEntry)
			if(it->second != GetCurrentThreadId()) return true;

	return false; // not declared
}

// For banker algorithm 
bool KernelFS::banker(int cache)
{
	threadIdFileMap filesDeclaredByThread; // all declared in system
	threadIdFileMap filesNeededToFinish;
	std::map<KernelFile*,DWORD> allFiles;
	std::map<DWORD,short> threads; // threads in system

	// pass through all threads which declared files
	for(stringThreadIDMapIt i = Declared[cache].begin(); i != Declared[cache].end(); i++)
			for(stringFileMapIt j = FilesHash[cache].begin(); j != FilesHash[cache].end(); j++)
				if(i->first == j->first) // pass through ALL declared files by threadID
				{
					filesDeclaredByThread.insert(std::pair<DWORD,KernelFile*>(i->second,j->second));
					threads.insert(std::pair<DWORD,short>(i->second,0));
					allFiles.insert(std::pair<KernelFile*,DWORD>(j->second,j->second->openedByThread));
					if(i->second == j->second->openedByThread) continue;
					filesNeededToFinish.insert(std::pair<DWORD,KernelFile*>(i->second,j->second));
				}	
	
	std::map<DWORD,short>::iterator curThread = threads.begin();
	

	// pass through all threads, 
	// and foreach find if she can allocate all declared files 
	// which she needs to complete execution
	while(curThread != threads.end())
	{
		int filesNeededCount = 0;
			
		for (threadIdFileMapIt i = filesNeededToFinish.begin();i != filesNeededToFinish.end(); i++)
			if(i->first == curThread->first) // pass through all declared by this thread
			{	
				filesNeededCount++;
				// file is free, take it
				if( !allFiles[i->second] )
				{ 
					allFiles[i->second] = curThread->first;
					filesNeededCount--;
				}
			}	

		if(filesNeededCount > 0) // there are files or file that other thread holds open.
		{
			// thread cannot finish, restore "taken" files
			for(threadIdFileMapIt j = filesNeededToFinish.begin(); j != filesNeededToFinish.end(); j++)
				if(j->first == curThread->first) allFiles[j->second] = 0;

			// try other
			++curThread;
			continue;
		}

		// thread can finish
		// mark thread files as free
		for(threadIdFileMapIt j = filesDeclaredByThread.begin(); j != filesDeclaredByThread.end(); j++)
			if(j->first == curThread->first) allFiles[j->second] = 0;

		// remove finishing thread
		threads.erase(curThread++);
		if(curThread == threads.end()) return true; // all threads can finish
		curThread = threads.begin();
	}
						
	return false;
}
KernelFS::~KernelFS()
{
	for(int i = 0; i < 26; i++) 
		if(partArr[i])
		{
			delete mutex[i];
			delete semForFormatOrUnmount[i];
			delete semCantOpenFiles[i];
			delete semCantDeleteFile[i];
			delete semCantUndeclare[i];
			delete semBlockedOnBanker[i];
		}
}
