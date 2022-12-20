#ifndef STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED
#define STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED

#include <utility>
#include <bit>
#include <ranges>
#include <memory_resource>
#include <vector>

template <typename T>
class stable_vector
{
    union element;
    struct block;
    template <typename>
    class iterator_t;
public:
    using allocator_type = std::pmr::polymorphic_allocator<T>;
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = iterator_t<value_type>;
    using const_iterator = iterator_t<const value_type>;

    stable_vector() = default;
    explicit stable_vector(allocator_type allocator) : allocator_(allocator) {}
    stable_vector(stable_vector&& v) noexcept
        : allocator_(v.allocator_)
        , size_(v.size_)
        , end_(v.end_)
        , blocks_(std::move(v.blocks_))
    {
        v.size_ = 0;
        v.end_ = nullptr;
    }
    stable_vector(const stable_vector& source)
        requires std::is_copy_constructible_v<value_type>
    {
        blocks_.reserve(source.blocks_.size());
        try {
            for (const auto &item: source) {
                push_back(item);
            }
        }
        catch (...)
        {
            delete_all();
            throw;
        }
    }
    template <std::ranges::range R>
    explicit stable_vector(const R& r)
        requires std::is_constructible_v<value_type, std::ranges::range_reference_t<R>>
    {
        try {
            for (auto &v : r) {
                emplace_back(v);
            }
        }
        catch (...)
        {
            delete_all();
            throw;
        }
    }
    ~stable_vector()
    {
        delete_all();
    }
    stable_vector& operator=(const stable_vector& v)
        requires std::is_copy_constructible_v<value_type>
    {
        if (&v != this)
        {
            auto old_blocks = std::move(blocks_);
            auto old_end = std::exchange(end_, nullptr);
            auto old_size = std::exchange(size_, 0);
            blocks_.clear();
            try {
                blocks_.reserve(v.blocks_.size());
                end_ = nullptr;
                for (const auto& e : v)
                {
                    push_back(e);
                }
            }
            catch (...)
            {
                size_ = old_size;
                while (!blocks_.empty())
                {
                    auto& last_block = blocks_.back();
                    if constexpr (!std::is_trivially_destructible_v<value_type>) {
                        if (!last_block.last_) {
                            end_ = last_block.end_;
                        }
                        while (end_ != last_block.begin_)
                        {
                            --end_;
                            std::destroy_at(end_);
                        }
                    }
                    auto idx = blocks_.size() - 1;
                    allocator_.deallocate(last_block.begin_,  1U << idx);
                    blocks_.pop_back();
                }
                end_ = old_end;
                std::swap(blocks_, old_blocks);
                throw ;
            }
            while (!old_blocks.empty())
            {
                auto &back = old_blocks.back();
                if (!back.last_)
                {
                    old_end = back.end_;
                }
                if (!std::is_trivially_destructible_v<value_type>)
                {
                    while (old_end != back.begin_)
                    {
                        --old_end;
                        std::destroy_at(old_end);
                    }
                }
                auto idx = old_blocks.size() - 1;
                allocator_.deallocate(back.begin_, 1U << idx);
                old_blocks.pop_back();
            }
        }
        return *this;
    }
    stable_vector& operator=(stable_vector&& v) noexcept
    {
        delete_all();
        size_ = std::exchange(v.size_, 0);
        end_ = std::exchange(v.end_, nullptr);
        std::swap(blocks_, v.blocks_);
        v.blocks_.clear();
        return *this;
    }
    reference& push_back(const_reference t)
        requires std::is_copy_constructible_v<value_type>
    {
        return grow(t);
    }
    reference& push_back(value_type&& t)
        requires std::is_move_constructible_v<value_type>
    {
        return grow(std::move(t));
    }
    template <typename ... Ts>
    reference& emplace_back(Ts&& ... ts)
        requires std::is_constructible_v<value_type, Ts...>
    {
        return grow(std::forward<Ts>(ts)...);
    }
    void pop_back() noexcept
    {
        --end_;
        std::destroy_at(end_);
        if (end_ == blocks_.back().begin_)
        {
            blocks_.pop_back();
            allocator_.deallocate(end_, 1U << blocks_.size());
            if (blocks_.empty())
            {
                end_ = nullptr;
            }
            else
            {
                blocks_.back().last_ = true;
                end_ = blocks_.back().end_;
            }
        }
        --size_;
    }
    [[nodiscard]]
    reference operator[](std::size_t idx) noexcept
    {
        return element_at(idx);
    }
    [[nodiscard]]
    const_reference operator[](std::size_t idx) const noexcept
    {
        return element_at(idx);
    }
    [[nodiscard]]
    reference front() noexcept { return *blocks_.front().begin_; }
    const_reference front() const noexcept { return *blocks_.front().begin_; }
    reference back() noexcept { return *std::prev(end_); }
    const_reference back() const noexcept { return *std::prev(end_); }
    [[nodiscard]]
    bool empty() const noexcept { return size_ == 0;}
    [[nodiscard]]
    std::size_t size() const noexcept { return size_; }
    void clear() { delete_all(); end_ = nullptr; size_ = 0; }
    iterator begin() noexcept { if (empty()) return { nullptr, nullptr }; auto& b = blocks_.front(); return { b.begin_, &b }; }
    const_iterator begin() const noexcept { if (empty()) return { nullptr, nullptr }; auto& b = blocks_.front(); return { b.begin_, &b }; }
    const iterator cbegin() const noexcept { if (empty()) return { nullptr, nullptr }; auto& b = blocks_.front(); return { b.begin_, &b }; }
    iterator end() noexcept { if (empty()) return { nullptr, nullptr }; auto& b = blocks_.back(); return { end_, &b }; }
    const_iterator end() const noexcept { if (empty()) return { nullptr, nullptr }; auto& b = blocks_.back(); return { end_, &b }; }
    const_iterator cend() const noexcept { if (empty()) return { nullptr, nullptr }; auto& b = blocks_.back(); return { end_, &b }; }
    auto rbegin() noexcept { return std::reverse_iterator(end());}
    auto rbegin() const noexcept { return std::reverse_iterator(end());}
    auto rend() noexcept { return std::reverse_iterator(begin());}
    auto rend() const noexcept { return std::reverse_iterator(begin());}
    iterator erase(iterator pos) noexcept
        requires std::is_nothrow_move_assignable_v<value_type>
    {
        auto e = end();
        if (pos != e) {
            auto i = pos;
            auto prev = i++;
            while (i != e) {
                *prev = std::move(*i);
                ++i;
                ++prev;
            }
            pop_back();
        }
        return pos;
    }
    iterator erase(iterator ib, iterator ie) noexcept
        requires std::is_nothrow_move_assignable_v<value_type>
    {
        const auto e = end();
        auto rv = ie;
        while (ie != e)
        {
            *ib = std::move(*ie);
            ++ie; ++ib;
        }
        bool at_end = false;
        while (!empty() && end_ != ib.operator->())
        {
            at_end = at_end | (rv == end());
            pop_back();
        }

        if (at_end)
        {
            rv = end();
        }
        return rv;
    }
    allocator_type get_allocator() const noexcept
    {
        return allocator_;
    }
private:
    reference element_at(std::size_t idx) const noexcept
    {
        //              14
        //              13
        //              12
        //              11
        //           6  10
        //           5   9
        //       2   4   8
        //   0   1   3   7
        const auto block_id = static_cast<std::size_t>(std::bit_width(idx + 1)) - 1;
        const auto block_offset = idx - (1U << block_id) + 1;
        return blocks_[block_id].begin_[block_offset];
    }
    void delete_all() noexcept
    {
        while (!blocks_.empty())
        {
            auto& last_block = blocks_.back();
            if (!last_block.last_)
            {
                end_ = last_block.end_;
            }
            if constexpr (!std::is_trivially_destructible_v<value_type>) {
                while (end_ != last_block.begin_)
                {
                    --end_;
                    std::destroy_at(end_);
                }
            }
            auto idx = blocks_.size() - 1;
            allocator_.deallocate(blocks_.back().begin_, 1U << idx);
            blocks_.pop_back();
        }

    }
    template <typename ... Ts>
    reference grow(Ts&& ... ts)
    {
        const auto old_end = end_;
        if (empty() || end_ == blocks_.back().end_)
        {
            const std::size_t size = 1 << blocks_.size();
            end_ = allocator_.allocate(size);
            blocks_.push_back({end_, end_ + size});
            if (blocks_.size() > 1) {
                blocks_[blocks_.size() - 2].last_ = false;
            }
        }
        try {
            std::uninitialized_construct_using_allocator<value_type>(end_,
                                                                     allocator_,
                                                                     std::forward<Ts>(ts)...);
        }
        catch (...)
        {
            auto& last_block = blocks_.back();
            if (last_block.begin_ == end_)
            {
                auto begin = last_block.begin_;
                blocks_.pop_back();
                allocator_.deallocate(begin, 1U << blocks_.size());
                if (!blocks_.empty()) {
                    blocks_.back().last_ = true;
                }
                end_ = old_end;
            }
            throw;
        }
        ++size_;
        return *end_++;

    }
    struct block {
        pointer begin_;
        pointer end_;
        bool last_ = true;
    };
    [[no_unique_address]] allocator_type allocator_;
    std::size_t size_ = 0;
    pointer end_ = nullptr;
    std::pmr::vector<block> blocks_{allocator_};
};

template <typename T> template <typename TT>
class stable_vector<T>::iterator_t
{
public:
    using value_type = T;
    using reference = TT&;
    using pointer = TT*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    iterator_t() = default;
    reference operator*() const noexcept { return *current_element; }
    pointer operator->() const noexcept { return current_element; }
    iterator_t& operator++() noexcept
    {
        ++current_element;
        if (current_element == current_block->end_ && !current_block->last_)
        {
            ++current_block;
            current_element = current_block->begin_;
        }
        return *this;
    }
    iterator_t operator++(int) noexcept
    {
        auto copy = *this;
        ++*this;
        return copy;
    }
    iterator_t& operator--() noexcept
    {
        if (current_element == current_block->begin_)
        {
            --current_block;
            current_element = current_block->end_;
        }
        --current_element;
        return *this;
    }
    iterator_t operator--(int) noexcept
    {
        auto copy = *this;
        --*this;
        return copy;
    }
    bool operator==(iterator_t rh) const noexcept
    {
        return current_element == rh.current_element;
    }
    operator iterator_t<const TT>() const noexcept { return { current_element, current_block }; }

    iterator_t(pointer e, const block* b) : current_element(e), current_block(b) {}
private:
    template <typename> friend class iterator_t;
    pointer current_element;
    const block* current_block;

};


#endif //STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED
