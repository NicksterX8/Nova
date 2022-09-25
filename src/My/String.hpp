#ifndef MY_STRING_H
#define MY_STRING_H

#include <string.h>
#include <string>
#include "MyUtils.hpp"

namespace MY_NAMESPACE_NAME {

struct AllocatedString {
    char* str;

    AllocatedString(size_t size) {
        str = new char[size];
    }

    AllocatedString(const char* _str) {
        int len = strlen(_str);
        str = new char[len+1];
        memcpy(str, _str, len+1);
    }

    ~AllocatedString() {
        delete[] str;
    }

    operator char*() {
        return str;
    }
};

struct StringView {
    const char* str;

    StringView(const char* _str) : str(_str) {}

    StringView(const std::string& _str) {
        str = _str.data();
    }

    operator char*() {
        return (char*)str;
    }

    AllocatedString operator+(const char* rhs) {
        int len1 = strlen(str);
        int len2 = strlen(rhs);

        AllocatedString result = AllocatedString(len1 + len2 + 1);
        memcpy(result.str, str, len1);
        memcpy(result.str + len1, rhs, len2+1);
        return result;
    }
};

inline AllocatedString str_add(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    AllocatedString result = AllocatedString(lenA + lenB + 1);
    memcpy(result.str, str_a, lenA);
    memcpy(result.str + lenA, str_b, lenB+1);
    return result;
}

}

#endif