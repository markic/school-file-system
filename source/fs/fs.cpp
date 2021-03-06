// File: fs.cpp

#include "fs.h"
#include "kernelfs.h"

KernelFS *kernelFS = new KernelFS();

FS::FS(){}

char FS::mount(Partition* partition) { return kernelFS->mount(partition); }

char FS::unmount(char part){ return kernelFS->unmount(part); }

char FS::format(char part){ return kernelFS->format(part); }

char FS::readRootDir(char part, EntryNum n, Directory &d){ return kernelFS->readRootDir(part, n, d); }

char FS::doesExist(char* fname){ return kernelFS->doesExist(fname); }

char FS::declare(char* fname, int mode){ return kernelFS->declare(fname, mode); }

File* FS::open(char* fname){ return kernelFS->open(fname); }

char FS::deleteFile(char* fname){return kernelFS->deleteFile(fname); }

FS::~FS(){}