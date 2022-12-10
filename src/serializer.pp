#include <stddef.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <unordered_map>

struct Stream {};

struct DataFormat;

void*  read(DataFormat& format, Stream& out);
size_t write(DataFormat& format, Stream& in);

struct DataFormat {
    struct Type {
        size_t size = 0;
        bool untyped = false;
    };

    template<typename T>
    Type makeType() {
        return {sizeof(T), true};
    }

    struct ReadData {
        void* data;
        size_t size;
        Type type;
    };

    template<typename T>
    struct ReadDataT {
        T* data;
        size_t size;
    };

    struct WriteData {
        size_t bytesWritten;
    };

    struct DeferredValue {
        std::string valueName;
    };
    template<typename T>
    struct DeferredValueT {
        std::function<ReadDataT<T>(Stream& stream)> get;

        ReadDataT<T> operator()(Stream& stream) {
            return get(stream);
        }
    };

    struct Field {
        std::string name;
        DeferredValue size;
        Type type;

        Field(std::string name, DeferredValue size, Type type) : name(name), size(size), type(type) {

        }
    };

    template<typename Return, typename T>
    using Monad = std::function<Return(const T&)>;

    using Action = std::function<void()>;

    struct Condition {
        DeferredValue value;
        Monad<bool, void*> condition;
        Action then;
    };

    template<typename T>
    struct ConditionT {
        DeferredValueT<T> value;
        Monad<bool, T> condition;
        Action then;
    };

    struct FieldValue {
        void* value;
        FieldValue* valueSize;

    };

    std::vector<Field> fields;
    std::vector<Condition> conditions;

    Field getField(std::string name) {
        // do it
    }
    
    void add(const char* name, size_t size) {
        
        numFields++;
    }

    template<class T>
    void add(const char* name, size_t count = 1) {
        numFields++;
    }

    template<class T>
    void add(std::string name, DeferredValue deferredSize) {
        fields.push_back(Field(name, deferredSize, makeType<T>()));
    }

    DeferredValue get(std::string fieldName) {
        return {[&](auto stream){ return this->read(fieldName, stream); }};
    }

    template<typename T>
    void conditional(DeferredValueT<T> value, std::function<bool(const T&)> condition, std::function<void()> then) {
        
    }
    
    // read

    ReadData read(std::string field, Stream& stream) {

    }
    
    // write

    WriteData write(std::string field, Stream& stream) {

    }

};

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

DataFormat myFormat() {
    #define ADDN(type, name, size) f.add<type>(TOSTRING(name), size)
    #define ADD1(type, name) ADDN(type, name, 1)
    #define VALUEOF(field) f.get(TOSTRING(field))
    #define CONDITION(value, condition, then) f.conditional(VALUEOF(value), [](auto value){ return (condition); }, []()then);

    DataFormat f = {};

    ADDN(char, tester, VALUEOF(numEntities));
    ADD1(int, numEntities);
    ADDN(double, entities, VALUEOF(numEntities));
    CONDITION(numEntities, numEntities == 0, {
        
    });

    f.add<int>("numEntities");
    f.add<char>("entities", f.get("numEntities"));
    f.conditional(f.get("numEntities"), [](auto value){ return value == 0; }, [](){

    });

    return f;
}

void* get(const DataFormat& format, const char* name) {

}

int main() {


}