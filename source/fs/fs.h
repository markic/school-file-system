// File: fs.h

#pragma once

typedef unsigned long BytesCnt;
typedef unsigned long EntryNum;

const unsigned long ENTRYCNT=64; 
const unsigned int FNAMELEN=8;
const unsigned int FEXTLEN=3;

struct Entry {
	char name[FNAMELEN];
	char ext[FEXTLEN];
	char reserved;
	unsigned long firstIndexCluster;
	unsigned long size;
};

typedef Entry Directory[ENTRYCNT];


class KernelFS;
class Partition;
class File;

class FS {
public:
	~FS ();
	static char mount(Partition* partition); // Mounts partition, returns new partition letter or 0 in case of error
	static char unmount(char part);  // Unmounts partition with given letter, returns 1 Success, 0 fail
	static char format(char part); 
	static char readRootDir(char part, EntryNum n, Directory &d); // Reads data from entry of partition to directory
	static char doesExist(char* fname); // pass absolute file path
	static char declare(char* fname, int mode); 
	// declares that current thread wants to use file if mode is 1, else undeclare 
	
	static File* open(char* fname); 
	static char deleteFile(char* fname);

protected:
	FS ();
	static KernelFS *myImpl;
};