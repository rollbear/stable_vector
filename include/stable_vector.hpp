#ifndef STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED
#define STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED

#include <vector>
#include <utility>
#include <bit>
#include <ranges>

template <typename T>
class stable_vector
{
    union element;
    struct block;
public:
    stable_vector() = default;
    stable_vector(stable_vector&& v) noexcept
        : size_(v.size_)
        , end_(v.end_)
        , blocks_(std::move(v.blocks_))
    {
        v.size_ = 0;
        v.end_ = nullptr;
    }
    stable_vector(const stable_vector& v)
        requires std::is_copy_constructible_v<T>
    {
        blocks_.reserve(v.blocks_.size());
        try {
            for (const auto &element: v) {
                push_back(element);
            }
        }
        catch (...)
        {
            delete_all();
            throw;
        }
    }
    template <std::ranges::range R>
    stable_vector(const R& r)
        requires std::is_constructible_v<T, std::ranges::range_reference_t<R>>
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
        requires std::is_copy_constructible_v<T>
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
                if (!blocks_.empty())
                {
                    blocks_.back().end_ = end_;
                }
                end_ = old_end;
                blocks_ = std::move(old_blocks);
                throw ;
            }
            if (old_size != 0U)
            {
                old_blocks.back().end_ = old_end;
            }
        }
        return *this;
    }
    stable_vector& operator=(stable_vector&& v) noexcept
    {
        delete_all();
        size_ = std::exchange(v.size_, 0);
        end_ = std::exchange(v.end_, nullptr);
        blocks_ = std::move(v.blocks_);
        return *this;
    }
    T& push_back(const T& t)
        requires std::is_copy_constructible_v<T>
    {
        return grow(t);
    }
    T& push_back(T&& t)
        requires std::is_move_constructible_v<T>
    {
        return grow(std::move(t));
    }
    template <typename ... Ts>
    T& emplace_back(Ts&& ... ts)
        requires std::is_constructible_v<T, Ts...>
    {
        return grow(std::forward<Ts>(ts)...);
    }
    void pop_back() noexcept
    {
        --end_;
        std::destroy_at(&end_->obj);
        if (end_ == blocks_.back().begin_)
        {
            blocks_.back().end_ = end_;
            blocks_.pop_back();
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
    T& operator[](std::size_t idx) noexcept
    {
        return element_at(idx);
    }
    [[nodiscard]]
    const T& operator[](std::size_t idx) const noexcept
    {
        return element_at(idx);
    }
    [[nodiscard]]
    T& front() noexcept { return blocks_.front().begin_->obj; }
    const T& front() const noexcept { return blocks_.front().begin_->obj; }
    T& back() noexcept { return std::prev(end_)->obj; }
    const T& back() const noexcept { return std::prev(end_)->obj; }
    [[nodiscard]]
    bool empty() const noexcept { return size_ == 0;}
    [[nodiscard]]
    std::size_t size() const noexcept { return size_; }
    void clear() { delete_all(); end_ = nullptr; size_ = 0; }
    template <typename TT>
    class iterator_t
    {
        template <typename> friend class iterator_t;
        element* current_element;
        const block* current_block;
        friend class stable_vector<T>;
        iterator_t(element* e, const block* b) : current_element(e), current_block(b) {}
    public:
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator_t() = default;
        T& operator*() const noexcept { return current_element->obj; }
        T* operator->() const noexcept { return &current_element->obj; }
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
    };
    using iterator = iterator_t<T>;
    using const_iterator = iterator_t<const T>;
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
        requires std::is_nothrow_move_assignable_v<T>
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
        requires std::is_nothrow_move_assignable_v<T>
    {
        const auto e = end();
        auto rv = ie;
        while (ie != e)
        {
            *ib = std::move(*ie);
            ++ie; ++ib;
        }
        bool at_end = false;
        while (!empty() && end_ != ib.current_element)
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
private:
    T& element_at(std::size_t idx) const noexcept
    {
        //              14
        //              13
        //              12
        //              11
        //           6  10
        //           5   9
        //       2   4   8
        //   0   1   3   7
        const auto block_id = std::bit_width(idx + 1) - 1;
        const auto block_offset = idx - (1U << block_id) + 1;
        return blocks_[block_id].begin_[block_offset].obj;
    }
    void delete_all() noexcept
    {
        if (!blocks_.empty())
        {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                auto &b = blocks_.back();
                b.end_ = end_;
            }
            blocks_.clear();
        }

    }
    template <typename ... Ts>
    T& grow(Ts&& ... ts)
    {
        const auto old_end = end_;
        if (empty() || end_ == blocks_.back().end_)
        {
            const std::size_t size = 1 << blocks_.size();
            end_ = blocks_.emplace_back(size).begin_;
            if (blocks_.size() > 1) {
                blocks_[blocks_.size() - 2].last_ = false;
            }
        }
        try {
            new (&end_->obj) T(std::forward<Ts>(ts)...);
        }
        catch (...)
        {
            if (blocks_.back().begin_ == end_)
            {
                blocks_.back().end_ = end_;
                blocks_.pop_back();
                if (!blocks_.empty()) {
                    blocks_.back().last_ = true;
                }
                end_ = old_end;
            }
            throw;
        }
        ++size_;
        return (end_++)->obj;

    }
    union element
    {
        element() {}
        ~element() {}
        T obj;
    };
    struct block {
        element* begin_;
        element* end_;
        bool last_ = true;

        block(std::size_t n)
        : begin_(new element[n])
        , end_(begin_ + n)
        {}
        block(block&& r)
        : begin_(std::exchange(r.begin_, nullptr))
        , end_(std::exchange(r.end_, nullptr))
        , last_(r.last_)
        {

        }
        ~block()
        {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                while (end_ != begin_) {
                    --end_;
                    std::destroy_at(&end_->obj);
                }
            }
            delete[] begin_;
        }
    };
    std::size_t size_ = 0;
    element* end_ = nullptr;
    std::vector<block> blocks_;
};


#endif //STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED
