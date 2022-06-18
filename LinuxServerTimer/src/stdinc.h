
#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/types.h>


template<typename T>
void safe_delete(T*& a)
{
	delete a;
	a = nullptr;
}
