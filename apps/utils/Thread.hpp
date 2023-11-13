#ifndef __THREAD_LIB_HPP
#define __THREAD_LIB_HPP

#include <lib/syscall.h>

typedef void (*ThreadFunc)(void*);

class Thread {
public:
    Thread(ThreadFunc func, int flags) {
        this->mFunc = func;
        this->flags = flags;
    }

    int start(void* arg) {
        mThreadId = thread_create(reinterpret_cast<void*>(mFunc), arg, flags);
        return mThreadId;
    }

    static int join(int threadId) {
        return -1;
    }

private:
    ThreadFunc mFunc;
    int flags;
    int mThreadId;
};

class Mutex {
public:
    Mutex() : mLocked(0) {}

    void lock() {
        while (__sync_lock_test_and_set(&mLocked, 1)) {
            yield();
        }
    }

    void unlock() {
        __sync_lock_release(&mLocked);
    }

private:
    volatile int mLocked;
};




#endif // !__THREAD_LIB_HPP