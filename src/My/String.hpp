#ifndef MY_STRING_H
#define MY_STRING_H

#include <string.h>
#include <string>
#include "MyInternals.hpp"
#include "../utils/Log.hpp"
#include "Vec.hpp"
#include "Iteration.hpp"
#include "Exp.hpp"

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

}

namespace My {

inline String str_add(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    char* result = new char[lenA + lenB + 1];
    memcpy(result, str_a, lenA);
    memcpy(result + lenA, str_b, lenB+1);
    return My::String::take(result);
}

inline char* str_add_alloc(const char* str_a, const char* str_b) {
    int lenA = strlen(str_a);
    int lenB = strlen(str_b);

    char* result = new char[lenA + lenB + 1];
    memcpy(result, str_a, lenA);
    memcpy(result + lenA, str_b, lenB+1);
    return result;
}

struct StringBuffer {
    Vec<char> buffer;

    static StringBuffer Empty() {
        return {Vec<char>("\0", 2)}; // buffer with 2 nul bytes
    }

    static StringBuffer WithCapacity(int capacity) {
        auto buffer = Vec<char>::WithCapacity(capacity);
        buffer.push("\0", 2);
        return {buffer};
    }

    static StringBuffer FromStr(const char* str) {
        int strSize = strlen(str)+1;
        auto buffer = Vec<char>::WithCapacity(1 + strSize);
        buffer.push('\0');
        buffer.push(str, strSize);
        return {buffer};
    }

    inline void destroy() {
        buffer.destroy();
    }

    // @param length should be equal to strlen(str)+1 (include nul byte)
    inline bool push(const char* str, int size) {
        return buffer.push(str, size);
    }

    inline bool push(char c) {
        return buffer.push(c);
    }

    char* getLastStr() const {
        if (buffer.size < 2) return nullptr; // a size of 1 is just nul byte, so that's unusable as well
        char* last = &buffer[buffer.size-2]; // get pointer to char right before nul byte
        // and move backwards from there
        while (*last != '\0' && last != &buffer[0]) {
            last--;
        }
        return last+1;
    }

    inline void appendToLast(char c) {
        if (buffer.empty()) {
            buffer.push(c);
        } else {
            buffer.back() = c; // replace nul byte with char and replace nul byte
        }
        buffer.push('\0');
    }

    inline void appendToLast(const char* str) {
        int length = strlen(str);
        if (buffer.empty()) {
            buffer.pop(); // remove nul byte
            buffer.push(str, length+1);
        } else {
            buffer.reserve(buffer.size + length);
            memcpy(&buffer[buffer.size-1], str, (length+1) * sizeof(char));
            buffer.size += length;
        }
    }

    inline void popLastChar() {
        buffer.pop();
        buffer.back() = '\0';
    }

    inline void endStr() {
        appendToLast('\0');
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

    iterator begin() const { return {buffer.data}; } // pointer to first character past start nul byte
    iterator end() const { return {buffer.data + buffer.size-1}; /* pointer to char past final nul byte */ }

    iterator rbegin() const { return {getLastStr()-1}; }
    iterator rend() const { return {buffer.data}; }
};

#define FOR_MY_STRING_BUFFER(name, strbuf, code) {\
        char* name = &strbuf.buffer[1];\
        while (name < &strbuf.buffer[strbuf.buffer.size-1]) {\
            code;\
            while (*name != '\0') {name++;}; name++;\
        };\
        } 
#define FOR_MY_STRING_BUFFER_REVERSE(name, strbuf, code) {\
    char* name = strbuf.getLastStr();\
    while (name > strbuf.buffer.data) {\
        code;\
        name -= 2;\
        while (*name != '\0') {name--;}; name++;\
    };\
    }

}

#endif