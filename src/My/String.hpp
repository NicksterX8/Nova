#ifndef MY_STRING_H
#define MY_STRING_H

#include <string.h>
#include <string>
#include "MyUtils.hpp"
#include "../utils/Log.hpp"
#include <memory>

namespace MY_NAMESPACE_NAME {

inline char* duplicateStr(const char* str) {
    int strSize = strlen(str)+1;
    char* copy = (char*)MY_malloc(strSize);
    memcpy(copy, str, strSize);
    return copy;
}

struct String {
    char* str;

    String() : str(nullptr) {}

    String(size_t size) {
        str = new char[size];
    }

    String(const char* _str) {
        str = duplicateStr(_str);
    }

    static String take(char* _str) {
        String string;
        string.str = _str;
        return string;
    }

    ~String() {
        MY_free(str);
    }

    operator char*() {
        return str;
    }

    operator const char*() const {
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
};

/*
inline AllocatedString str_add(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    //AllocatedString result = AllocatedString(lenA + lenB + 1);
    char* result = new char[lenA + lenB + 1];
    memcpy(result, str_a, lenA);
    memcpy(result + lenA, str_b, lenB+1);
    LogInfo("Allocated str for str_add : %s", result);
    return AllocatedString::take(result);
}
*/
}

namespace My {

//#define STR_ADD(lhs, rhs) (My::string_result = strcat(strcpy((char*)malloc(strlen(lhs) + strlen(rhs) + 1), lhs), rhs)); defer(free(My::string_result))

//#define STR_ADD2(var, lhs, rhs) char* var = strcat(strcpy((char*)malloc(strlen(lhs) + strlen(rhs) + 1), lhs), rhs); defer {free(var);}

inline String str_add(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    char* result = new char[lenA + lenB + 1];
    memcpy(result, str_a, lenA);
    memcpy(result + lenA, str_b, lenB+1);
    return My::String::take(result);
}

inline char* str_add_manual(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    char* result = new char[lenA + lenB + 1];
    memcpy(result, str_a, lenA);
    memcpy(result + lenA, str_b, lenB+1);
    return result;
}

}

#endif