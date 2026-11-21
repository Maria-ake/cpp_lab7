#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

template <typename T, std::size_t BlockSize = 100>
class PoolAllocator
{
    struct Pool
    {
        std::size_t slotSize;
        std::vector<char*> chunks;
        std::vector<void*> freeSlots;

        explicit Pool(std::size_t sz) : slotSize(sz) {}

        ~Pool()
        {
            for (char* c : chunks)
            {
                ::operator delete(c);
            }
        }

        void* allocate()
        {
            if (freeSlots.empty())
            {
                char* chunk = static_cast<char*>(::operator new(BlockSize * slotSize));
                chunks.push_back(chunk);
                for (std::size_t i = 0; i < BlockSize; ++i)
                {
                    freeSlots.push_back(chunk + i * slotSize);
                }
            }
            void* p = freeSlots.back();
            freeSlots.pop_back();
            return p;
        }

        void deallocate(void* p) noexcept
        {
            freeSlots.push_back(p);
        }
    };

    std::shared_ptr<Pool> pool_;

    template <typename U, std::size_t B>
    friend class PoolAllocator;

public:
    using value_type = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    PoolAllocator() : pool_(std::make_shared<Pool>(sizeof(T))) {}

    template <typename U>
    PoolAllocator(const PoolAllocator<U, BlockSize>&)
        : pool_(std::make_shared<Pool>(sizeof(T)))
    {}

    PoolAllocator(const PoolAllocator&) noexcept = default;
    PoolAllocator& operator=(const PoolAllocator&) noexcept = default;

    template <typename U>
    struct rebind
    {
        using other = PoolAllocator<U, BlockSize>;
    };

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n == 0) return nullptr;
        if (n == 1) return static_cast<T*>(pool_->allocate());
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        if (!p) return;
        if (n == 1)
        {
            pool_->deallocate(p);
        }
        else
        {
            ::operator delete(p);
        }
    }

    bool operator==(const PoolAllocator& other) const noexcept
    {
        return pool_ == other.pool_;
    }

    template <typename U>
    bool operator==(const PoolAllocator<U, BlockSize>&) const noexcept
    {
        return false;
    }

    template <typename U>
    bool operator!=(const PoolAllocator<U, BlockSize>& other) const noexcept
    {
        return !(*this == other);
    }
};

template <typename K, typename V, std::size_t BlockSize = 100>
using MyUnorderedMap = std::unordered_map<
    K,
    V,
    std::hash<K>,
    std::equal_to<K>,
    PoolAllocator<std::pair<const K, V>, BlockSize>
>;

template <typename K, std::size_t BlockSize = 100>
using MyUnorderedSet = std::unordered_set<
    K,
    std::hash<K>,
    std::equal_to<K>,
    PoolAllocator<K, BlockSize>
>;