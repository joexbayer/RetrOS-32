/**
 * @file StringHelper.hpp
 * @author Joe Bayer (joexbayer)
 * @brief A string helper class 
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef STRING_HELPER_H
#define STRING_HELPER_H

//#include <libc.h>
//#include "cppUtils.hpp"
//#include <lib/syscall.h>
//#include <lib/printf.h>

class StringList;

class String {
private:
    /* Helper function to copy data */
    void copy_data(const char* str, int length) {
        m_length = length;
        m_data = new char[m_length + 1];
        strncpy(m_data, str, m_length);
        m_data[m_length] = '\0';
    }

public:
    int m_length;
    char* m_data;

    String(const char* str) {
        m_length = strlen(str);
        copy_data(str, m_length);
    }

    String() {
        m_length = 0;
        m_data = new char[1];
        m_data[0] = '\0';
    }

    String(const String& other) {
        copy_data(other.m_data, other.m_length);
    }

    String& operator=(const String& other) {
        if (this != &other) {
            delete[] m_data;
            copy_data(other.m_data, other.m_length);
        }
        return *this;
    }

    /* Split */
    StringList* split(char delimiter) const; // Forward declaration

    ~String() {
        delete[] m_data;
    }

    void setString(const char* str) {
        delete[] m_data;
        copy_data(str, strlen(str));
    }

    const char* getData() const {
        return m_data;
    }

    int getLength() const {
        return m_length;
    }

    bool includes(const char* str) const {
        int len = strlen(str);
        if (len > m_length) return false;
        for (int i = 0; i <= m_length - len; ++i) {
            if (strncmp(m_data + i, str, len) == 0) return true;
        }
        return false;
    }

    /**
     * @brief Get a substring of the string
     * @note Will create a new String object
     * @param start The start index
     * @param end The end index
     * @return A new String object
     */
    String substring(int start, int end) const {
        int len = end - start;
        char* result = new char[len + 1];
        for (int i = 0; i < len; i++) {
            result[i] = m_data[start + i];
        }
        result[len] = '\0';
        return String(result);
    }

    int indexOf(char delimiter) const {
        for (int i = 0; i < m_length; ++i) {
            if (m_data[i] == delimiter) {
                return i;
            }
        }
        return -1;
    }

    bool startsWith(const String& prefix) const {
        int prefixLen = prefix.getLength();
        if (prefixLen > m_length) {
            return false;
        }
        return strncmp(m_data, prefix.getData(), prefixLen) == 0;
    }

    bool endsWith(const String& suffix) const {
        int suffixLen = suffix.getLength();
        if (suffixLen > m_length) {
            return false;
        }
        return strncmp(m_data + (m_length - suffixLen), suffix.getData(), suffixLen) == 0;
    }

    void concat(const char* str) {
        int len2 = strlen(str);
        char* result = new char[m_length + len2 + 1];
        strncpy(result, m_data, m_length);
        strncpy(result + m_length, str, len2);
        result[m_length + len2] = '\0';
        delete[] m_data;
        m_data = result;
        m_length += len2;
    }

    static int strlen(const char* str) {
        int len = 0;
        while (str[len] != '\0') {
            ++len;
        }
        return len;
    }

    static void strncpy(char* dest, const char* src, int n) {
        int i = 0;
        while (i < n && src[i] != '\0') {
            dest[i] = src[i];
            ++i;
        }
        while (i < n) {
            dest[i] = '\0';
            ++i;
        }
    }

    static int strncmp(const char* str1, const char* str2, int n) {
        int i = 0;
        while (i < n && str1[i] != '\0' && str2[i] != '\0') {
            if (str1[i] != str2[i]) return 1;
            ++i;
        }
        return 0;
    }

    static int strcmp(const char* str1, const char* str2) {
        int i = 0;
        while (str1[i] != '\0' && str2[i] != '\0') {
            if (str1[i] != str2[i]) return 1;
            ++i;
        }
        return 0;
    }
};

class StringList {
public:
    StringList(int size) {
        m_size = size;
        m_data = new String[size];
    }

    ~StringList() {
        delete[] m_data;
    }

    String& operator[](int index) {
        return m_data[index];
    }

    const String& operator[](int index) const {
        return m_data[index];
    }

    String get(int index) const {
        return m_data[index];
    }

    int getSize() const {
        return m_size;
    }

    void clear() {
        m_size = 0;
    }

private:
    String* m_data;
    int m_size;
};

/**
 * @brief Split a string by a delimiter
 * @note Will create a new StringList object
 * @param delimiter The delimiter to split by
 * @return A new StringList object

StringList* String::split(char delimiter) const {
    int count = 0;
    for (int i = 0; i < m_length; ++i) {
        if (m_data[i] == delimiter) {
            ++count;
        }
    }

    StringList* result = new StringList(count + 1);
    int start = 0;
    int index = 0;
    for (int i = 0; i < m_length; ++i) {
        if (m_data[i] == delimiter) {
            result->operator[](index) = substring(start, i);
            start = i + 1;
            ++index;
        }
    }
    result->operator[](result->getSize() - 1) = substring(start, m_length);
    return result;
}
 */

#endif  // STRING_HELPER_H