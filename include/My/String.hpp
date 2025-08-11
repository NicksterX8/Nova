#ifndef MY_STRING_H
#define MY_STRING_H

#include <string.h>
#include <string>
#include "MyInternals.hpp"
#include "../utils/Log.hpp"
#include "Iteration.hpp"
#include "memory/StackAllocate.hpp"

#include <memory>
#include <stdexcept>

template<typename ... Args>
std::string string_format(const std::string& format, Args... args) {
    if (format.empty()) return "";
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    if (LLVM_UNLIKELY(size_s <= 0)) {
        LogError("Failed formatting.");
        return {};
    }
    auto size = (size_t)size_s;
    StackAllocate<char, 128> buf{(int)size};
    std::snprintf((char*)buf, size, format.c_str(), args...);
    return std::string((char*)buf, (char*)buf + size - 1); // We don't want the '\0' inside
}

// specialize for no args
inline std::string string_format(const std::string& str) {
    return str;
}


MY_CLASS_START

inline char* strdup(const char* str) {
    if (!str) return nullptr;
    int strSize = strlen(str)+1;
    char* copy = (char*)MY_malloc(strSize);
    memcpy(copy, str, strSize);
    return copy;
}

inline bool streq(const char* a, const char* b) {
    if (a == b) return true;
    if (a == nullptr || b == nullptr) return false; 
    for (;;) {
        if (*a == *b) {
            if (*a == '\0') return true; // only need to check one since we know they are equal
        } else {
            return false;
        }
        a++; b++;
    }
    return true;
}

inline bool streq(const char* a, const char* b, int lenA, int lenB) {
    if (a == b) return true;
    if (lenA != lenB) return false; // not entirely necessary, but probably best for performance for a lot of cases
    if (a == nullptr || b == nullptr) return false; 
    
    for (int i = 0; i < lenA; i++) { // lenA is already known to be equal to lenB
        if (a[i] != b[i]) return false;
    }
    return true;
}

// compare string equality ignoring case
inline bool streq_case(const char* a, const char* b, int lenA, int lenB) {
    if (a == b) return true;
    if (lenA != lenB) return false; // not entirely necessary, but probably best for performance for a lot of cases
    if (a == nullptr || b == nullptr) return false; 
    
    for (int i = 0; i < lenA; i++) { // lenA is already known to be equal to lenB
        if (std::tolower(a[i]) != std::tolower(b[i])) return false;
    }

    return true;
}

struct CString {
    char* str;

    CString() : str(nullptr) {}

    explicit CString(size_t size) {
        str = (char*)MY_malloc(size * sizeof(char));
    }

    CString(const char* _str) {
        str = strdup(_str);
    }

    // copy constructor
    CString(const CString& other)
    : str(strdup(other.str)) {}

    CString(CString&& other) {
        this->str = other.str;
        other.str = nullptr;
    }

    // copy assignment operator
    CString& operator=(const CString& other) {
        if (&other != this) {
            // first free this
            MY_free(str);
            // then copy
            str = strdup(other.str);
        }
        return *this;
    }

    static CString From(const char* str) {
        return CString(str);
    }

    static CString WithCapacity(size_t capacity) {
        return CString(capacity);
    }

    static CString MakeUsing(char* str) {
        CString string;
        string.str = str;
        return string;
    }

    ~CString() {
        MY_free(str);
    }

    operator char*() {
        return str;
    }

    operator const char*() const {
        return str;
    }

    char& operator[](int i) {
        return str[i];
    }
    const char& operator[](int i) const {
        return str[i];
    }

    void operator+=(const char* other) {
        int currentLength = (int)strlen(str)+1;
        int addedLength = (int)strlen(other);
        char* newStr = (char*)MY_realloc(str, currentLength + addedLength + 1); // add 1 for null byte
        if (newStr) {
            str = newStr;
            char* lastChar = str + currentLength;
            memcpy(lastChar, other, addedLength+1); // add 1 for null byte
        } // do nothing in case of memory fail
    }
};

inline CString str_add(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    char* result = (char*)MY_malloc((lenA + lenB + 1) * sizeof(char));
    memcpy(result, str_a, lenA);
    memcpy(result + lenA, str_b, lenB+1);
    return My::CString::MakeUsing(result);
}

inline char* str_add_alloc(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    char* result = (char*)MY_malloc((lenA + lenB + 1) * sizeof(char));
    memcpy(result, str_a, lenA);
    memcpy(result + lenA, str_b, lenB+1);
    return result;
}

MY_CLASS_END

#endif