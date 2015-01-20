# school-file-system
File system with indexed allocation. Multiple threads supported. Used Banker's algorithm for deadlock avoidance and resource allocation. Developed in C++.

This was a project for the course ”Operating systems 2” on my faculty.


Department of Computer Engineering and Information Theory.


School of Electrical Engineering, University of Belgrade, Serbia.


Developed by Marin Markić. No licence. November - December 2012.
- marinmarkic@mail.com


### How to use file system: (see example)

 - File must be declared before it can be opened, and it has to be undeclared before closing. If file open leads to unsafe system state, thread will be blocked on open operation until requirements are met. The Banker's algorithm for resource allocation and deadlock avoidance is used to check on system state. 

 ### File system (class FS):
```C++
	static char mount(Partition* partition); // Mounts partition, returns new partition letter or 0 in case of error
	static char unmount(char part);  // Unmounts partition with given letter, returns 1 Success, 0 fail
	static char format(char part); 
	static char readRootDir(char part, EntryNum n, Directory &d); // Reads data from entry of partition to directory
	static char doesExist(char* fname); // pass absolute file path
	static char declare(char* fname, int mode); 
	// if mode is 1 thread declares usage of file, if its 0 thread undeclares file.
	
	static File* open(char* fname); 
	static char deleteFile(char* fname);
```
File:
 ```C++
	char write (BytesCnt, char* buffer); // writes bytes count from buffer to disk[cursor]
	BytesCnt read (BytesCnt, char* buffer); // reads bytes count from disk[cursor] to buffer
	
	char seek (BytesCnt); // moves cursor to position
	BytesCnt filePos(); // returns cursor position
	
	char eof (); // is cursor on last byte 
	BytesCnt getFileSize (); // returns number of bytes of file on disk
	
	char truncate (); // deletes part of file aftrer cursor
	~File(); // closes file
```