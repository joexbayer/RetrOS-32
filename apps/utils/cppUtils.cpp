#include <stdint.h>
#include "cppUtils.hpp"
#include <lib/syscall.h>

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

void operator delete(void* p, unsigned int index)
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
