#pragma once
//
// cp/core/small_vector.h
//
// 极简定长内联向量：前 N 个元素存于对象内联数组（零堆分配），超出再落堆。
// 专为 interval_set 内部使用（元素为可平凡复制的 interval）。
// 指针迭代器；insert/erase 对扩容安全（先把 pos 转成下标再操作）。
//
#include <cstddef>
#include <utility>

namespace ECFlow {

template <class T, std::size_t N>
class small_vector {
public:
    using iterator       = T*;
    using const_iterator = const T*;

    small_vector() = default;

    small_vector(const small_vector& o) { assign_from(o.data_, o.size_); }
    small_vector(small_vector&& o) noexcept { move_from(o); }

    small_vector& operator=(const small_vector& o) {
        if (this != &o) { clear_storage(); assign_from(o.data_, o.size_); }
        return *this;
    }
    small_vector& operator=(small_vector&& o) noexcept {
        if (this != &o) { clear_storage(); move_from(o); }
        return *this;
    }
    ~small_vector() { if (heap()) delete[] data_; }

    std::size_t size()  const { return size_; }
    bool        empty() const { return size_ == 0; }

    T&       operator[](std::size_t i)       { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }
    T&       front()       { return data_[0]; }
    const T& front() const { return data_[0]; }
    T&       back()        { return data_[size_ - 1]; }
    const T& back()  const { return data_[size_ - 1]; }

    iterator       begin()       { return data_; }
    iterator       end()         { return data_ + size_; }
    const_iterator begin() const { return data_; }
    const_iterator end()   const { return data_ + size_; }

    void clear() { size_ = 0; }

    void push_back(const T& v) {
        ensure(size_ + 1);
        data_[size_++] = v;
    }

    // 在 pos 处插入 v；返回指向插入元素的指针（扩容安全）。
    iterator insert(iterator pos, const T& v) {
        std::size_t idx = (std::size_t)(pos - data_);
        ensure(size_ + 1);
        for (std::size_t i = size_; i > idx; --i) data_[i] = data_[i - 1];
        data_[idx] = v;
        ++size_;
        return data_ + idx;
    }

    // 删除单个元素；返回其后继位置。
    iterator erase(iterator pos) { return erase(pos, pos + 1); }

    // 删除 [first,last)；返回指向 first 位置的指针。
    iterator erase(iterator first, iterator last) {
        std::size_t a = (std::size_t)(first - data_);
        std::size_t b = (std::size_t)(last - data_);
        std::size_t n = b - a;
        for (std::size_t i = b; i < size_; ++i) data_[i - n] = data_[i];
        size_ -= n;
        return data_ + a;
    }

    friend bool operator==(const small_vector& x, const small_vector& y) {
        if (x.size_ != y.size_) return false;
        for (std::size_t i = 0; i < x.size_; ++i) if (!(x.data_[i] == y.data_[i])) return false;
        return true;
    }
    friend bool operator!=(const small_vector& x, const small_vector& y) { return !(x == y); }

private:
    T           inline_[N];
    T*          data_ = inline_;
    std::size_t size_ = 0;
    std::size_t cap_  = N;

    bool heap() const { return data_ != inline_; }

    void ensure(std::size_t need) {
        if (need <= cap_) return;
        std::size_t nc = cap_ * 2;
        if (nc < need) nc = need;
        T* p = new T[nc];
        for (std::size_t i = 0; i < size_; ++i) p[i] = data_[i];
        if (heap()) delete[] data_;
        data_ = p; cap_ = nc;
    }

    void clear_storage() {
        if (heap()) delete[] data_;
        data_ = inline_; cap_ = N; size_ = 0;
    }

    void assign_from(const T* src, std::size_t n) {
        if (n > N) { data_ = new T[n]; cap_ = n; } else { data_ = inline_; cap_ = N; }
        for (std::size_t i = 0; i < n; ++i) data_[i] = src[i];
        size_ = n;
    }

    void move_from(small_vector& o) {
        if (o.heap()) {                 // 偷堆指针
            data_ = o.data_; cap_ = o.cap_; size_ = o.size_;
            o.data_ = o.inline_; o.cap_ = N; o.size_ = 0;
        } else {                        // 内联只能逐个复制
            data_ = inline_; cap_ = N; size_ = o.size_;
            for (std::size_t i = 0; i < size_; ++i) data_[i] = o.data_[i];
            o.size_ = 0;
        }
    }
};

} // namespace ECFlow
