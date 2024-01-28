#ifndef __FUNCTION_HPP__
#define __FUNCTION_HPP__

#include <utils/StdLib.hpp>
#include <utils/cppUtils.hpp>

class BaseCallable {
public:
    virtual ~BaseCallable() {}
    virtual void call() const = 0;
    virtual BaseCallable* clone() const = 0;
};


template<typename T>
class Callable : public BaseCallable {
    T callable;

public:
    Callable(T callable) : callable(callable) {}

    void call() const override {
        callable();
    }

    BaseCallable* clone() const override {
        return new Callable<T>(callable);
    }
};


class Function {
    BaseCallable* callable;

public:
    Function() : callable(nullptr) {}

    Function(const Function& other) {
        if (other.callable) {
            // The challenge is to clone `other.callable`.
            // This requires a mechanism in `BaseCallable` and its derived classes
            // to clone themselves.
            callable = other.callable->clone();
        } else {
            callable = nullptr;
        }
    }

    template<typename T>
    Function(T lambda) : callable(new Callable<T>(lambda)) {}

    ~Function() {
        delete callable;
    }
    Function& operator=(const Function& other) = delete; // Copy assignment

    Function(Function&& other) : callable(other.callable) {
        other.callable = nullptr;
    }

    Function& operator=(Function&& other) {
        if (this != &other) {
            delete callable;
            callable = other.callable;
            other.callable = nullptr;
        }
        return *this;
    }

    void operator()() const {
        if (callable) {
            callable->call();
        }
    }
};


#endif // __FUNCTION_HPP__
