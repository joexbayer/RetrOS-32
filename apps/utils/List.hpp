#ifndef __LIST_HPP__
#define __LIST_HPP__

//#include <libc.h>
#include "cppUtils.hpp"

template <typename T>
class List {
private:
    T* data;
    int capacity;
    int size;

    void resize(int new_capacity) {
        T* new_data = (T*)malloc(new_capacity * sizeof(T));
        for (int i = 0; i < size; ++i) {
            new_data[i] = data[i];
        }
        free(data);
        data = new_data;
        capacity = new_capacity;
    }

public:
    List() : data(nullptr), capacity(0), size(0) {}

    ~List() {
        delete[] data;
    }

    void push(const T& value) {
        if (size == capacity) {
            resize(capacity == 0 ? 1 : capacity * 2);
        }
        data[size++] = value;
    }

    void pop_back() {
        if (size > 0) {
            --size;
        }
    }

    T& operator[](int index) {
        return data[index];
    }

    const T& operator[](int index) const {
        return data[index];
    }

    int get_size() const {
        return size;
    }

    int get_capacity() const {
        return capacity;
    }

    bool is_empty() const {
        return size == 0;
    }

    void clear() {
        size = 0;
    }
};


#endif // __LIST_HPP__