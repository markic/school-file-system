// File: file.h

#pragma once

typedef unsigned long BytesCnt;

class KernelFile;

class File {
public:
	char write (BytesCnt, char* buffer); 
	BytesCnt read (BytesCnt, char* buffer);
	
	char seek (BytesCnt);
	BytesCnt filePos();
	
	char eof ();
	BytesCnt getFileSize ();
	
	char truncate ();
	~File(); // closes file

private:
	friend class FS;
	friend class KernelFS;
	File ();  // file can be created only with open
	KernelFile *myImpl;
};