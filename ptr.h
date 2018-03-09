// ptr.h
//

//! Wrapper for deep-copyable pointer
template<typename T> class ptr
{
public:
    ptr()
        : _value(new T())
    {}
    ptr(T value)
        : _value(new T(value))
    {}
    ptr(ptr<T> const& other)
        : _value(other._value ? new T(*other._value) : nullptr)
    {}
    ptr(ptr<T>&& other)
        : _value(other._value)
    {
        other._value = nullptr;
    }
    ~ptr() { delete _value; }

    ptr<T>& operator=(T value)
    {
        delete _value;
        _value = new T(value);
    }
    ptr<T>& operator=(ptr<T> const& other)
    {
        delete _value;
        _value = other._value ? new T(*other._value) : nullptr;
        return *this;
    }
    ptr<T>& operator=(ptr<T>&& other)
    {
        delete _value;
        _value = other._value;
        other._value = nullptr;
        return *this;
    }
    operator T&()
    {
        return *_value;
    }
    operator T const&() const
    {
        return *_value;
    }
    bool operator==(ptr<T> const& other) const
    {
        return _value == other._value || (_value && other._value && *_value == *other._value);
    }
    bool operator!=(ptr<T> const& other) const
    {
        return !(this == other);
    }

protected:
    T* _value;
};
