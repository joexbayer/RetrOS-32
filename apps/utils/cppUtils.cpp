/**
 * @file cppUtils.cpp
 * @author Joe Bayer (joexbayer)
 * @brief A collection of useful C++ utilities
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdint.h>
#include "cppUtils.hpp"
#include <lib/syscall.h>

extern "C" {
/* Minimal runtime stubs to satisfy freestanding C++ without libstdc++. */
void *__dso_handle = 0;

int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle)
{
	(void)func;
	(void)arg;
	(void)dso_handle;
	return 0;
}

void __cxa_pure_virtual(void)
{
	/* Trap for accidental pure virtual calls. */
	while (1) yield();
}
}

void *operator new(size_t size)
{
    return malloc(size);
}
 
void *operator new[](size_t size)
{
    return malloc(size);
}
 
void operator delete(void *p)
{
    free(p);
}
 
void operator delete[](void *p)
{
    free(p);
}

void operator delete(void* p, size_t index)
{
    void* ptr = (void*)((uint32_t)p);
    free(ptr);
}


template <typename T>
class UniquePtr {
private:
    T* ptr;
public:
    explicit UniquePtr(T* p = nullptr) : ptr(p) {}

    ~UniquePtr() { 
        delete ptr; 
    }

    T& operator*() const { 
        return *ptr; 
    }
    
    T* operator->() const { 
        return ptr; 
    }

    UniquePtr(UniquePtr& other) = delete;  // Disallow copying
    UniquePtr& operator=(UniquePtr& other) = delete;

    UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) {  // Move constructor
        other.ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {  // Move assignment
        if (this != &other) {
            delete ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
};

template <typename T>
class SharedPtr {
private:
    T* ptr;
    int* count;

public:
    explicit SharedPtr(T* p = nullptr) : ptr(p), count(new int(1)) {}

    ~SharedPtr() { 
        (*count)--;
        if (*count == 0) {
            delete ptr;
            delete count;
        }
    }

    T& operator*() const { 
        return *ptr; 
    }
    
    T* operator->() const { 
        return ptr; 
    }

    SharedPtr(const SharedPtr& other) : ptr(other.ptr), count(other.count) {  // Copy constructor
        (*count)++;
    }

    SharedPtr& operator=(const SharedPtr& other) {  // Copy assignment
        if (this != &other) {
            (*count)--;
            if (*count == 0) {
                delete ptr;
                delete count;
            }
            ptr = other.ptr;
            count = other.count;
            (*count)++;
        }
        return *this;
    }
};
