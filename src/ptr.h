// ptr.h
//

#include <memory>

//! Wrapper for lazy shareable pointer
template<typename T> class ptr
{
public:
    //! default construction
    ptr()
        : _value(new T())
    {}
    //! implicit construction from value
    ptr(T value)
        : _value(new T(value))
    {}
    //! value assignment
    ptr<T>& operator=(T value)
    {
        _value = new T(value);
    }
    //! implicit cast to const value ref
    operator T const&() const
    {
        return *_value.get();
    }

protected:
    std::shared_ptr<T> _value;
};
