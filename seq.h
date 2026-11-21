#pragma once

#include <compare>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include "pool_allocator.h"

template <typename T>
class contiguous_iterator
{
private:
    T* ptr_ = nullptr;

public:
    using iterator_category = std::contiguous_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    contiguous_iterator() = default;
    explicit contiguous_iterator(T* ptr) : ptr_(ptr) {}

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    contiguous_iterator& operator++() { ++ptr_; return *this; }
    contiguous_iterator operator++(int) { auto tmp = *this; ++ptr_; return tmp; }
    contiguous_iterator& operator--() { --ptr_; return *this; }
    contiguous_iterator operator--(int) { auto tmp = *this; --ptr_; return tmp; }

    contiguous_iterator& operator+=(difference_type n) { ptr_ += n; return *this; }
    contiguous_iterator& operator-=(difference_type n) { ptr_ -= n; return *this; }

    friend contiguous_iterator operator+(contiguous_iterator it, difference_type n) { it += n; return it; }
    friend contiguous_iterator operator+(difference_type n, contiguous_iterator it) { it += n; return it; }
    friend contiguous_iterator operator-(contiguous_iterator it, difference_type n) { it -= n; return it; }
    friend difference_type operator-(const contiguous_iterator& lhs, const contiguous_iterator& rhs)
    {
        return lhs.ptr_ - rhs.ptr_;
    }

    reference operator[](difference_type n) const { return *(ptr_ + n); }
    auto operator<=>(const contiguous_iterator& other) const = default;
};

template <typename T, typename Allocator = std::allocator<T>>
class Seq
{
    using AllocTraits = std::allocator_traits<Allocator>;

    T* d = nullptr;
    std::size_t n = 0;
    std::size_t cap = 0;
    Allocator alloc_;

    void reallocate(std::size_t new_cap)
    {
        T* nd = AllocTraits::allocate(alloc_, new_cap);
        for (std::size_t i = 0; i < n; ++i)
        {
            AllocTraits::construct(alloc_, nd + i, std::move(d[i]));
            AllocTraits::destroy(alloc_, d + i);
        }
        if (d)
        {
            AllocTraits::deallocate(alloc_, d, cap);
        }
        d = nd;
        cap = new_cap;
    }

    void grow()
    {
        std::size_t nc = cap ? (cap * 3 / 2 + 1) : 2;
        reallocate(nc);
    }

public:
    using value_type     = T;
    using allocator_type = Allocator;
    using iterator       = contiguous_iterator<T>;
    using const_iterator = contiguous_iterator<const T>;

    Seq() = default;

    explicit Seq(std::size_t count, const T& value = T())
    {
        if (count > 0)
        {
            reserve(count);
            for (std::size_t i = 0; i < count; ++i)
            {
                AllocTraits::construct(alloc_, d + i, value);
            }
            n = count;
        }
    }

    ~Seq()
    {
        clear();
        if (d)
        {
            AllocTraits::deallocate(alloc_, d, cap);
        }
    }

    Seq(const Seq& o)
        : alloc_(AllocTraits::select_on_container_copy_construction(o.alloc_))
    {
        if (o.n > 0)
        {
            d = AllocTraits::allocate(alloc_, o.n);
            cap = o.n;
            for (std::size_t i = 0; i < o.n; ++i)
            {
                AllocTraits::construct(alloc_, d + i, o.d[i]);
            }
            n = o.n;
        }
    }

    Seq& operator=(const Seq& o)
    {
        if (this != &o)
        {
            Seq tmp(o);
            std::swap(d, tmp.d);
            std::swap(n, tmp.n);
            std::swap(cap, tmp.cap);
            std::swap(alloc_, tmp.alloc_);
        }
        return *this;
    }

    Seq(Seq&& o) noexcept
        : d(o.d), n(o.n), cap(o.cap), alloc_(std::move(o.alloc_))
    {
        o.d = nullptr;
        o.n = 0;
        o.cap = 0;
    }

    Seq& operator=(Seq&& o) noexcept
    {
        if (this != &o)
        {
            clear();
            if (d)
            {
                AllocTraits::deallocate(alloc_, d, cap);
            }
            d = o.d;
            n = o.n;
            cap = o.cap;
            alloc_ = std::move(o.alloc_);
            o.d = nullptr;
            o.n = 0;
            o.cap = 0;
        }
        return *this;
    }

    void clear() noexcept
    {
        for (std::size_t i = 0; i < n; ++i)
        {
            AllocTraits::destroy(alloc_, d + i);
        }
        n = 0;
    }

    void reserve(std::size_t new_cap)
    {
        if (new_cap > cap)
        {
            reallocate(new_cap);
        }
    }

    template <typename... Args>
    T& emplace_back(Args&&... args)
    {
        if (n >= cap) grow();
        AllocTraits::construct(alloc_, d + n, std::forward<Args>(args)...);
        return d[n++];
    }

    void push_back(const T& v) { emplace_back(v); }
    void push_back(T&& v) { emplace_back(std::move(v)); }

    std::size_t size() const noexcept { return n; }
    bool empty() const noexcept { return n == 0; }
    T* data() noexcept { return d; }
    const T* data() const noexcept { return d; }

    T& back() { return d[n - 1]; }
    const T& back() const { return d[n - 1]; }

    T& operator[](std::size_t i) { return d[i]; }
    const T& operator[](std::size_t i) const { return d[i]; }

    iterator begin() { return iterator(d); }
    iterator end() { return iterator(d + n); }
    const_iterator begin() const { return const_iterator(d); }
    const_iterator end() const { return const_iterator(d + n); }
};

template <typename T, std::size_t BlockSize = 100>
using MyVector = Seq<T, PoolAllocator<T, BlockSize>>;