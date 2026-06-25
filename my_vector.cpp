#include <cstddef>
#include <initializer_list>
#include <new>
#include <iterator>


template <typename T>
const T& min(const T& a, const T& b) { return (a < b) ? a : b; }

template <typename T>
class my_vector {
private:
    T* data_;
    std::size_t capacity_;
    std::size_t size_;

public:
    my_vector(const my_vector& other);
    my_vector(my_vector&& other) noexcept;
    my_vector& operator=(const my_vector& other);
    my_vector& operator=(my_vector&& other) noexcept;
    ~my_vector() noexcept;

    my_vector();
    my_vector(const std::initializer_list<T>& list);
    my_vector(std::size_t size);
    my_vector(std::size_t size, const T& value);

    template <typename Iterator>
    my_vector(Iterator begin, Iterator end);

    my_vector& operator=(const std::initializer_list<T>& list);
};

template <typename T>
my_vector<T>::my_vector(const my_vector<T>& other)
    : data_{nullptr}
    , capacity_{other.capacity_}
    , size_{other.size_}
{
    if (capacity_ == 0) { return; } // if the capacity is 0, return the default object

    data_ = static_cast<T*>(::operator new (capacity_ * sizeof(T)));
    std::size_t constructed = 0;
    try {
        for (std::size_t i = 0; i < size_; ++i) {
            new (data_ + i) T(other.data_[i]);
            ++constructed;
        }

        return;
    } catch (...) {
        for (std::size_t i = 0; i < constructed; ++i) {
            data_[i].~T();
        }

        ::operator delete(data_);
        throw;
    }
}

template <typename T>
my_vector<T>::my_vector(my_vector<T>&& other) noexcept
    : data_{other.data_}
    , capacity_{other.capacity_}
    , size_{other.size_}
{
    other.data_     = nullptr;
    other.capacity_ = 0;
    other.size_     = 0;
}

template <typename T>
my_vector<T>& my_vector<T>::operator=(const my_vector<T>& other) {
    if (this == &other) { return *this; }

    if (other.size_ == 0) { // placement new (0) -> UB
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }

        size_ = 0;
        return *this;
    }

    if (capacity_ < other.size_) {
        T* new_data_ = static_cast<T*>(::operator new(other.capacity_ * sizeof(T)));
        std::size_t constructed = 0;
        try {
            for (std::size_t i = 0; i < other.size_; ++i) {
                new (new_data_ + i) T(other.data_[i]);
                ++constructed;
            }

            for (std::size_t i = 0; i < size_; ++i) {
                data_[i].~T();
            }

            ::operator delete (data_);
            data_     = new_data_;
            capacity_ = other.capacity_;
            size_     = other.size_;
            new_data_ = nullptr;
            return *this;
        } catch (...) {
            for (std::size_t i = 0; i < constructed; ++i) {
                new_data_[i].~T();
            }

            ::operator delete (new_data_);
            new_data_ = nullptr;
            throw;
        }
    }

    for (std::size_t i = 0; i < min(size_, other.size_); ++i) {
        data_[i] = other.data_[i];
    }

    for (std::size_t i = other.size_; i < size_; ++i) {
        data_[i].~T();
    }
    
    std::size_t constructed = 0;
    for (std::size_t i = size_; i < other.size_; ++i) {
        try {
            new (data_ + i) T(other.data_[i]);
            ++constructed;
        } catch (...) {
            size_ += constructed;
            throw;
        }
    }

    size_ = other.size_;
    return *this;
}

template <typename T>
my_vector<T>& my_vector<T>::operator=(my_vector<T>&& other) noexcept {
    if (this == &other) { return *this; }

    for (std::size_t i = 0; i < size_; ++i) {
        data_[i].~T();
    }
    
    ::operator delete (data_);
    data_           = other.data_;
    other.data_     = nullptr;
    capacity_       = other.capacity_;
    other.capacity_ = 0;
    size_           = other.size_;
    other.size_     = 0;
    return *this;
}

template <typename T>
my_vector<T>::~my_vector() noexcept {
    for (std::size_t i = 0; i < size_; ++i) {
        data_[i].~T();
    }

    ::operator delete (data_);
    data_     = nullptr;
    capacity_ = 0;
    size_     = 0;
}

template <typename T>
my_vector<T>::my_vector()
    : data_{nullptr}
    , size_{0}
    , capacity_{0}
{
}

template <typename T>
my_vector<T>::my_vector(const std::initializer_list<T>& list)
    : size_{list.size()}
    , capacity_{list.size()}
    , data_{nullptr}
{
    if (capacity_ == 0) return;
    std::size_t constructed{0};
    try {
        data_ = static_cast<T*>(::operator new(list.size() * sizeof(T)));
        for (const auto& elem : list) {
            new (data_ + constructed) T(elem);
            ++constructed;
        }
    } catch (...) {
        while (constructed != 0) {
            --constructed;
            (data_ + constructed)->~T();
        }
        
        ::operator delete (data_);
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        throw;
    }

}

template <typename T>
my_vector<T>::my_vector(std::size_t size)
    : size_{0}
    , capacity_{size}
    , data_{nullptr}
{
    data_ = static_cast<T*>(::operator new (size * sizeof(T)));
    std::size_t constructed{0};
    try {
        while (constructed < size) {
            new (data_ + constructed) T();
        }

        size_ = size;
    } catch (...) {
        while (constructed != 0) {
            --constructed;
            (data_ + constructed)->~T();
        }

        ::operator delete (data_);
        data_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        throw;
    }
}

template <typename T>
my_vector<T>::my_vector(std::size_t size, const T& value)
    : size_{size}
    , capacity_{size}
    , data_{nullptr}
{
    data_ = static_cast<T*>(::operator new (size * sizeof(T)));
    std::size_t constructed{0};
    try {
        while (constructed < size) {
            new (data_ + constructed) T(value);
        }

        size_ = size;
    } catch (...) {
        while (constructed != 0) {
            --constructed;
            (data_ + constructed)->~T();
        }

        ::operator delete (data_);
        data_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        throw;
    }
}

template <typename T>
template <typename Iterator>
my_vector<T>::my_vector(Iterator begin, Iterator end)
    : capacity_{static_cast<std::size_t>(end - begin)}
    , size_{static_cast<std::size_t>(end - begin)}
    , data_{nullptr}
{
    std::size_t constructed{0};
    try {
        using ValueType = std::iterator_traits<begin>::value_type;
        std::size_t size = static_cast<std::size_t>(end - begin);
        data_ = static_cast<T*>(::operator new (size * sizeof(ValueType)));
        while (constructed < size) {
            const auto& elem = *(begin + constructed);
            new (data_ + constructed) T(elem);
            ++constructed;
        }
    } catch (...) {
        while (constructed != 0) {
            --constructed;
            (data_ + constructed)->~T();
        }

        ::operator delete (data_);
        data_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        throw;
    }
}

template <typename T>
my_vector<T>& my_vector<T>::operator=(const std::initializer_list<T>& list) {
    if (list.size() == 0) {
        for (std::size_t i = 0; i < size_; ++ i) {
            data_[i].~T();
        }

        size_ = 0;
        return *this;
    }

    if (capacity_ < list.size()) {
        
    }
}