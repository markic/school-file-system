// File: kernelexception.h

#pragma once

#include <exception>
#include <string>

using namespace std;

class KernelException : public exception {
public:
	KernelException(const string message = "Undefined Exception"): _message(message) {}
	const char* what() { return _message.c_str(); }
	~KernelException() {}

private:
	string _message;
};