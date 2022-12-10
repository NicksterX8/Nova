#ifndef MY_STRING_H
#define MY_STRING_H

#include <string.h>
#include <string>
#include "MyInternals.hpp"
#include "../utils/Log.hpp"
#include "Vec.hpp"
#include "Iteration.hpp"
#include "Exp.hpp"

MY_CLASS_START

inline char* strdup(const char* str) {
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

struct StringSizePair {
    char* str;
    int size;
};

struct StringBuffer {
    Vec<char> buffer;

    static StringBuffer Empty() {
        return {Vec<char>::Empty()}; // buffer with 2 nul bytes
    }

    static StringBuffer WithCapacity(int capacity) {
        return {Vec<char>::WithCapacity(capacity)};
    }

    static StringBuffer FromStr(const char* str) {
        int strSize = strlen(str)+1;
        auto buffer = Vec<char>::WithCapacity(strSize);
        buffer.push(str, strSize);
        return {buffer};
    }

    inline void destroy() {
        buffer.destroy();
    }

    // @param length should be equal to strlen(str)+1 (include nul byte)
    inline void push(const char* str, int size) {
        buffer.push(str, size);
    }
    inline void push(const char* str) {
        buffer.push(str, strlen(str)+1);
    }

    inline void pushFront(const char* str) {
        buffer.pushFront(str, strlen(str)+1);
    }

    inline void pushUnterminatedStr(const char* str, int length) {
        buffer.push(str, length);
        buffer.push('\0');
    }

    char* lastStr(int* size=nullptr) const {
        if (buffer.size == 0) return nullptr; // no string to get
        char* last = &buffer[buffer.size-1]; // get pointer to first char  behing nul byte
        // and move backwards from there
        while (last > buffer.data && *(last-1) != '\0') {
            last--;
        }
        if (size) *size = last - &buffer[buffer.size-1];
        return last;
    }

    inline void appendToLast(char c) {
        if (buffer.size == 0) {
            buffer.push(c);
        } else {
            buffer.back() = c; // replace nul byte with char and replace nul byte
        }
        buffer.push('\0');
    }

    inline void appendToLast(const char* str, int size) {
        if (buffer.size > 0) {
            buffer.pop(); // pop off nul byte
        }
        buffer.push(str, size);
    }

    void appendToLast(const char* str) {
        appendToLast(str, strlen(str)+1);
    }

    inline void popLastChar() {
        buffer.pop();
        buffer.back() = '\0';
    }

    inline void endLastStr() {
        buffer.push('\0');
    }

    inline void operator+=(const char* str) {
        push(str, strlen(str)+1);
    }

    // returns true if the string buffer is empty, meaning it contains no characters other than the default terminating character.
    // If the string buffer is empty, it is not safe to pop a character
    bool empty() const {
        // needs atleast chars 2 (atleast 1 actual 1 and one nul byte)
        return buffer.size < 2;
    }

    struct iterator {
        char* str;

        static char* forward(char* str) {
            while (*str != '\0') { ++str; }
            return ++str;
        }

        static char* backward(char* str) {
            str -= 2;
            while (*str != '\0') { --str; }
            return ++str;
        }

        iterator operator--() {
            str--;
            while (*str != '\0') { --str; }
            return *this;
        }

        iterator operator++() {
            str++;
            while (*str != '\0') { ++str; }
            return *this;
        }

        char* operator*() const {
            return str+1;
        }

        bool operator!=(iterator rhs) const {
            return str != rhs.str;
        }
    };

    template<typename T>
    void forEachStr(const T& callback) const {
        char* str = buffer.data;
        while (str <= &buffer.back()) {
            callback(str);
            while (*str != '\0') {str++;}; str++;
        };
    }

    template<typename T>
    void forEachStrReverse(const T& callback) const {
        char* str = lastStr();
        if (!str) return;
        for (;;) {
            callback(str);
            if (str >= buffer.data + 2) {
                str -= 2;
                while (str >= buffer.data && *str != '\0') {str--;}; str++;
            } else {
                break;
            }
        };
    }

    /*
    iterator begin() const { return {buffer.data}; } // pointer to first character past start nul byte
    iterator end() const { return {buffer.data + buffer.size-1}; / pointer to char past final nul byte / }

    iterator rbegin() const { return {lastStr()-1}; }
    iterator rend() const { return {buffer.data}; }
    */
};

#define FOR_MY_STRING_BUFFER(name, strbuf, code) {\
    char* name = &strbuf.buffer[0];\
    while (name <= &strbuf.buffer.back()) {\
        code;\
        while (*name != '\0') {name++;}; name++;\
    };\
    }
#define FOR_MY_STRING_BUFFER_REVERSE(name, strbuf, code) {\
    char* name = strbuf.buffer.data + strbuf.buffer.size-1;\
    while (name > strbuf.buffer.data) {\
        if (*(name-1) == '\0') {code;}\
        name--;\
    }\
    if (strbuf.buffer.size > 0) { name = &strbuf[0]; code; }\
    }

MY_CLASS_END

#endif