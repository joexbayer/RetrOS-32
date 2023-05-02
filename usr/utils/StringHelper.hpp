#ifndef STRING_HELPER_H
#define STRING_HELPER_H

#include <util.h>
#include "cppUtils.hpp"

class String {
private:
    char* m_data;
    int m_length;

public:
    String(const char* str) {
        m_length = strlen(str);
        m_data = new char[m_length + 1];
        memcpy(m_data, str, strlen(str));
    }

    ~String() {
        delete[] m_data;
    }

    const char* getData() const {
        return m_data;
    }

    int getLength() const {
        return m_length;
    }

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
        for (int i = 0; i < m_length; i++) {
            if (m_data[i] == delimiter) {
                return i;
            }
        }
        return -1;
    }

    int startsWith(const String& prefix) const {
        int prefixLen = prefix.getLength();
        if (prefixLen > m_length) {
            return 0;
        }
        return strncmp(m_data, prefix.getData(), prefixLen) == 0;
    }

    int endsWith(const String& suffix) const {
        int suffixLen = suffix.getLength();
        if (suffixLen > m_length) {
            return 0;
        }
        return strncmp(m_data + (m_length - suffixLen), suffix.getData(), suffixLen) == 0;
    }

    void trimWhitespace() {
        int start = 0;
        int end = m_length - 1;
        while (isspace(m_data[start])) {
            start++;
        }
        while (end > start && isspace(m_data[end])) {
            end--;
        }
        if (end >= start) {
            char* newData = new char[end - start + 2];
            memcpy(newData, m_data + start, end - start + 1);
            newData[end - start + 1] = '\0';
            delete[] m_data;
            m_data = newData;
            m_length = end - start + 1;
        } else {
            delete[] m_data;
            m_data = new char[1];
            m_data[0] = '\0';
            m_length = 0;
        }
    }

    String concat(const String& str) const {
        int len1 = m_length;
        int len2 = str.getLength();
        char* result = new char[len1 + len2 + 1];
        memcpy(result, m_data, strlen(m_data));
        memcpy(result + len1, str.getData(), str.getLength());
        return String(result);
    }
};

#endif  // STRING_HELPER_H