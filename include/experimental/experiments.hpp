
/*
template<typename T>
class ValueList;

template<typename T>
struct VirtualList {
    friend class ValueList<T>;
    std::function<T*(int)> _get;

    VirtualList(const std::function<T*(int)>& _get) : _get(_get) {}

    T* get(int index) const {
        return _get(index);
    }

    void set(int index, const T& value) const {
        *get(index) = value;
    }

    T& operator[](int index) const {
        return *_get(index);
    }
};

template<typename T>
struct VirtualInput {
    std::function<T(int)> _get;

    VirtualInput(const std::function<T(int)>& getter) : _get(getter) {}

    VirtualInput(const T* pointer) {
        if (pointer)
            _get = [pointer](int index){
                return pointer[index];
            };
    }

    T get(int index) const {
        return _get(index);
    }

    T operator[](int index) const {
        return _get(index);
    }

    operator bool() const {
        return (bool)(_get);
    }
};

template<typename T>
struct VirtualOutput {
    std::function<void(int, T)> _set;
public:

    struct Proxy {
        const std::function<void(int, T)>* setter;
        int index;
        
        Proxy(const VirtualOutput<T>* output, int index) : setter(&output._set), index(index) {}

        void operator=(const T& value) {
            const auto& set = setter;
            set(index, value);
        }
    };

    VirtualOutput(const std::function<void(int, T)>& setter) : _set(setter) {}

    VirtualOutput(T* pointer) {
        if (pointer)
            _set = [pointer](int index, T value){
                pointer[index] = value;
            }
    }

    void set(int index, const T& value) const {
        _set(index, value);
    }

    Proxy operator[](int index) const {
        return Proxy(this, index);
    }

    operator bool() const {
        return (bool)(_set);
    }
};

template<typename T>
struct VirtualValueList {
    //static_assert(!std::is_const<T>::value, "Const doesn't make sense for value types!");

    std::function<T(int)> _get;
    std::function<void(int, T set) _set;

    VirtualValueList(const std::function<T(int)>& _get) : _get(_get) {}

    VirtualValueList(VirtualList<T> refList) {
        auto refGetter = refList._get;
        _get = [refGetter](int index)->T {
            return *refGetter(index);
        };
    }

    VirtualValueList(VirtualValueList<std::remove_const<T>::type> mutList) {
        auto mutGetter = muttList._get;
        _get = [mutGetter](int index)->T {
            return mutGetter(index);
        };
    }

    T get(int index) const {
        return _get(index);
    }

    T operator[](int index) const {
        return _get(index);
    }

    void set(int index, T value) const {
        return _set(index, value);
    }
};


struct GenericVirtualList {
    std::function<void*(int)> _get;

    GenericVirtualList(const std::function<void*(int)>& _get) : _get(_get) {}

    template<typename T>
    GenericVirtualList(const std::function<T*(int)>& otherGet) {
        _get = [otherGet](int index){
            return otherGet(index);
        };
    }

    template<typename T>
    GenericVirtualList(const VirtualList<T>& otherList) {
        auto& otherGet = otherList._get;
        _get = [otherGet](int index){
            return otherGet(index);
        };
    }

    void* get(int index) const {
        return _get(index);
    }

    void* operator[](int index) const {
        return _get(index);
    }
};


//using GenericList = RefList<void>;

struct ConstGenericVirtualList {
    std::function<const void*(int index)> _get;

    ConstGenericVirtualList(GenericVirtualList list) : _get(list._get) {}

    template<typename T>
    ConstGenericVirtualList(const std::function<const T*(int)>& otherGet) {
        _get = [otherGet](int index){
            return otherGet(index);
        };
    }

    template<typename T>
    ConstGenericVirtualList(const VirtualList<T>& otherList) {
        auto& otherGet = otherList._get;
        _get = [otherGet](int index){
            return otherGet(index);
        };
    }

    template<typename T>
    ConstGenericVirtualList(const VirtualValueList<T>& otherList) {
        auto& otherGet = otherList._get;
        _get = [otherGet](int index){
            return otherGet(index);
        };
    }

    const void* get(int index) const {
        return _get(index);
    }

    const void* operator[](int index) const {
        return _get(index);
    }
};
*/

template<typename T>
struct VirtualOutput {
private:
    void* data;
    size_t stride;
public:

    VirtualOutput(std::nullptr_t null) : data(nullptr), stride(0) {}

    VirtualOutput(T* array) : data(array), stride(sizeof(T)) {}

    VirtualOutput(void* data, size_t stride) : data(data), stride(stride) {}

    T& operator[](int index) {
        assert(index >= 0 && "no negative array indices");
        return *(T*)((char*)data + (size_t)index*stride);
    }

    operator bool() const {
        return data != nullptr;
    }
};