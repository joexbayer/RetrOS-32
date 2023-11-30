#ifndef STRING_HELPER_H
#define STRING_HELPER_H

#include <util.h>
#include "cppUtils.hpp"
#include <lib/syscall.h>
#include <lib/printf.h>

class String {
private:

public:
    char* m_data;
    int m_length;
    String(const char* str) {
        m_length = String::strlen(str);
        m_data = new char[m_length+1];
        String::strncpy(m_data, str, m_length+1);
    }

    void setString(const char* str)
    {
        int len = strlen(str);
        for(int i = 0; i < len; i++){
            m_data[i] = str[i];
        }
        m_data[len] = '\0';
    }

    ~String() {
        delete m_data;
    }

    const char* getData() const {
        return m_data;
    }

    int getLength() const {
        return m_length;
    }

    int includes(const char* str){
        int len = strlen(str);
        if(len > m_length) return 0;
        for(int i = 0; i < m_length - len; i++){
            if(strncmp(m_data + i, str, len) == 0) return 1;
        }
        return 0;
    }

    static int strlen(const char* str) {
        int len = 0;
        while (str[len] != '\0') {
            len++;
        }
        return len;
    }

    static int strncpy(char* dest, const char* src, int n) {
        int i = 0;
        while (i < n && src[i] != '\0') {
            dest[i] = src[i];
            i++;
        }
        while (i < n) {
            dest[i] = '\0';
            i++;
        }
        return i;
    }

    static int strcmp(const char* str1, const char* str2){
        int i = 0;
        while(str1[i] != '\0' && str2[i] != '\0'){
            if(str1[i] != str2[i]) return 1;
            i++;
        }
        return 0;
    }

    String* substring(int start, int end) const {
        int len = end - start;
        char* result = new char[len + 1];
        for (int i = 0; i < len; i++) {
            result[i] = m_data[start + i];
        }
        result[len] = '\0';
        return new String(result);
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
            delete m_data;
            m_data = newData;
            m_length = end - start + 1;
        } else {
            delete m_data;
            m_data = new char[1];
            m_data[0] = '\0';
            m_length = 0;
        }
    }

    void concat(const char* str) {
        int len1 = m_length;
        int len2 = strlen(str);
        char* result = new char[len1 + len2 + 1];
        strncpy(result, m_data, len1);
        strncpy(result + len1, str, len2);
        delete m_data;
        result[len1 + len2] = '\0';
        m_data = result;
        m_length = len1 + len2;
    }
};

#endif  // STRING_HELPER_H