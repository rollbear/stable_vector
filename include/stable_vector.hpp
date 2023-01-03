#ifndef STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED
#define STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED

#include <utility>
#include <bit>
#include <ranges>
#include <memory_resource>
#include <vector>

template <
    typename T,
    typename Alloc = std::allocator<T>
>
class stable_vector
{
    struct block;

    template <typename>
    class iterator_t;
public:
    using allocator_type = Alloc;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = iterator_t<value_type>;
    using const_iterator = iterator_t<const value_type>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    stable_vector() = default;

    explicit stable_vector(allocator_type allocator);

    stable_vector(stable_vector&& v) noexcept;

    stable_vector(stable_vector&& v, allocator_type alloc);

    stable_vector(const stable_vector& source)
        requires std::is_copy_constructible_v<value_type>;

    template <std::ranges::range R>
    explicit stable_vector(const R& r, allocator_type alloc = allocator_type{})
        requires std::is_constructible_v<value_type, std::ranges::range_reference_t<R>>;

    explicit stable_vector(std::initializer_list<value_type> v,
                           allocator_type alloc = allocator_type{})
        requires std::is_copy_constructible_v<value_type>;

    template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    stable_vector(Iterator i, Sentinel e, allocator_type alloc=allocator_type{})
        requires std::is_constructible_v<T, typename std::iterator_traits<Iterator>::value_type>;

    ~stable_vector();

    auto operator=(const stable_vector& v) -> stable_vector&
        requires std::is_copy_constructible_v<value_type>;

    auto operator=(stable_vector&& v) noexcept -> stable_vector&
        requires (std::allocator_traits<allocator_type>::is_always_equal::value
                 || std::is_copy_constructible_v<value_type>);

    auto push_back(const_reference t) -> reference
        requires std::is_copy_constructible_v<value_type>;

    auto push_back(value_type&& t) -> reference
        requires std::is_move_constructible_v<value_type>;

    template <typename ... Ts>
    auto emplace_back(Ts&& ... ts) -> reference
        requires std::is_constructible_v<value_type, Ts...>;

    void pop_back() noexcept;

    [[nodiscard]]
    auto operator[](std::size_t idx) noexcept -> reference;

    [[nodiscard]]
    auto operator[](std::size_t idx) const noexcept -> const_reference;

    [[nodiscard]]
    auto front() noexcept -> reference;

    [[nodiscard]]
    auto front() const noexcept -> const_reference;

    [[nodiscard]]
    auto back() noexcept -> reference;

    [[nodiscard]]
    auto back() const noexcept -> const_reference;

    [[nodiscard]]
    auto empty() const noexcept -> bool;

    [[nodiscard]]
    auto size() const noexcept -> std::size_t;

    void clear() noexcept;

    [[nodiscard]]
    auto begin() noexcept -> iterator;

    [[nodiscard]]
    auto begin() const noexcept -> const_iterator;

    [[nodiscard]]
    auto cbegin() const noexcept -> const_iterator;

    [[nodiscard]]
    auto end() noexcept -> iterator;

    auto end() const noexcept -> const_iterator;

    [[nodiscard]]
    auto cend() const noexcept -> const_iterator;

    [[nodiscard]]
    auto rbegin() noexcept -> reverse_iterator;

    [[nodiscard]]
    auto rbegin() const noexcept -> const_reverse_iterator;

    [[nodiscard]]
    auto rend() noexcept -> reverse_iterator;

    [[nodiscard]]
    auto rend() const noexcept -> const_reverse_iterator;

    auto erase(iterator pos) noexcept -> iterator
        requires std::is_nothrow_move_assignable_v<value_type>;
    auto erase(iterator ib, iterator ie) noexcept -> iterator
        requires std::is_nothrow_move_assignable_v<value_type>;

    [[nodiscard]]
    auto get_allocator() const noexcept -> allocator_type;
private:
    auto element_at(std::size_t idx) const noexcept -> reference;
    void delete_all() noexcept;
    template <typename Vector>
    static
    void delete_all(Vector& blocks, pointer end) noexcept;

    template <typename ... Ts>
    auto grow(Ts&& ... ts) -> reference;

    void shrink();

    using block_allocator = typename allocator_traits::template rebind_alloc<block>;

    [[no_unique_address]] allocator_type allocator_;
    std::size_t size_ = 0;
    pointer end_ = nullptr;
    std::vector<block, block_allocator> blocks_{allocator_};
};


template <typename T, typename Alloc> template <typename TT>
class stable_vector<T, Alloc>::iterator_t
{
    friend class stable_vector<T, Alloc>;
    using block = typename stable_vector<T, Alloc>::block;
public:
    using value_type = T;
    using reference = TT&;
    using pointer = TT*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    iterator_t() = default;

    [[nodiscard]]
    auto operator*() const noexcept -> reference;

    auto operator->() const noexcept -> pointer;

    auto operator++() noexcept -> iterator_t&;
    auto operator++(int) noexcept -> iterator_t;
    auto operator--() noexcept -> iterator_t&;
    auto operator--(int) noexcept -> iterator_t;

    friend auto operator==(iterator_t lh, iterator_t rh) noexcept -> bool
    {
        return lh.current_element == rh.current_element;
    }

    operator iterator_t<const TT>() const noexcept;

private:
    iterator_t(pointer e, const block* b);

    template <typename> friend class iterator_t;
    pointer current_element;
    const block* current_block = nullptr;

};

template <typename T, typename Alloc>
struct stable_vector<T, Alloc>::block
{
    pointer begin_;
    pointer end_;
    bool last_ = true;
};

template <typename T, typename Alloc>
stable_vector<T, Alloc>::stable_vector(allocator_type allocator)
: allocator_(allocator)
{
}

template <typename T, typename Alloc>
stable_vector<T, Alloc>::stable_vector(stable_vector&& v) noexcept
    : allocator_(std::move(v.allocator_))
    , size_(std::exchange(v.size_, 0))
    , end_(std::exchange(v.end_, nullptr))
    , blocks_(std::move(v.blocks_))
{
}

template <typename T, typename Alloc>
stable_vector<T, Alloc>::stable_vector(stable_vector&& v, allocator_type alloc)
    : allocator_(std::move(alloc))
{
    if constexpr (!std::allocator_traits<allocator_type>::is_always_equal::value)
    {
        if (allocator_ != v.get_allocator())
        {
            blocks_.reserve(v.blocks_.size());
            for (auto&& eleme : v)
            {
                push_back(std::move(eleme));
            }
            return;
        }
    }
    size_ = std::exchange(v.size_, 0);
    end_ = std::exchange(v.end_, nullptr);
    blocks_= std::move(v.blocks_);
}

template <typename T, typename Alloc>
stable_vector<T, Alloc>::stable_vector(const stable_vector& source)
requires std::is_copy_constructible_v<T>
    : allocator_(std::allocator_traits<allocator_type>::select_on_container_copy_construction(
    source.get_allocator()))
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

template <typename T, typename Alloc>
stable_vector<T, Alloc>::stable_vector(std::initializer_list<value_type> v,
                                       allocator_type alloc)
requires std::is_copy_constructible_v<T>
    : stable_vector(v.begin(), v.end(), alloc)
{
}


template <typename T, typename Alloc> template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
stable_vector<T, Alloc>::stable_vector(Iterator i, Sentinel e, allocator_type alloc)
requires std::is_constructible_v<T, typename std::iterator_traits<Iterator>::value_type>
: allocator_(alloc)
{
    try {
        while (i != e)
        {
            emplace_back(*i++);
        }
    }
    catch (...)
    {
        delete_all();
        throw;
    }

}

template <typename T, typename Alloc> template <std::ranges::range R>
stable_vector<T, Alloc>::stable_vector(const R& r, allocator_type alloc)
requires std::is_constructible_v<value_type, std::ranges::range_reference_t<R>>
    : stable_vector(std::begin(r), std::end(r), alloc)
{
}

template <typename T, typename Alloc>
stable_vector<T, Alloc>::~stable_vector()
{
    delete_all();
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::operator=(const stable_vector& v) -> stable_vector&
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
            delete_all(blocks_, end_);
            size_ = old_size;
            end_ = old_end;
            std::swap(blocks_, old_blocks);
            throw ;
        }
        delete_all(old_blocks, old_end);
    }
    return *this;
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::operator=(stable_vector&& v) noexcept -> stable_vector&
requires (std::allocator_traits<Alloc>::is_always_equal::value
|| std::is_copy_constructible_v<T>)
{
    if constexpr (!typename std::allocator_traits<allocator_type>::is_always_equal{})
    {
        if (get_allocator() != v.get_allocator())
        {
            return operator=(v); // copy;
        }
    }
    delete_all();
    size_ = std::exchange(v.size_, 0);
    end_ = std::exchange(v.end_, nullptr);
    std::swap(blocks_, v.blocks_);
    v.blocks_.clear();
    return *this;
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::push_back(const_reference t) -> reference
requires std::is_copy_constructible_v<T>
{
    return grow(t);
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::push_back(value_type&& t) -> reference
requires std::is_move_constructible_v<T>
{
    return grow(std::move(t));
}


template <typename T, typename Alloc> template <typename ... Ts>
auto stable_vector<T, Alloc>::emplace_back(Ts&& ... ts) -> reference
requires std::is_constructible_v<T, Ts...>
{
    return grow(std::forward<Ts>(ts)...);
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::pop_back() noexcept -> void
{
    --size_;
    --end_;
    std::destroy_at(end_);
    shrink();
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::operator[](std::size_t idx) noexcept -> reference
{
    return element_at(idx);
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::operator[](std::size_t idx) const noexcept -> const_reference
{
    return element_at(idx);
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::front() noexcept -> reference
{
    return *blocks_.front().begin_;
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::front() const noexcept -> const_reference
{
    return *blocks_.front().begin_;
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::back() noexcept -> reference
{
    return *std::prev(end_);
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::back() const noexcept -> const_reference
{
    return *std::prev(end_);
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::empty() const noexcept -> bool
{
    return size_ == 0;
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::size() const noexcept -> std::size_t
{
    return size_;
}

template <typename T, typename Alloc>
void stable_vector<T, Alloc>::clear() noexcept
{
    delete_all();
    end_ = nullptr;
    size_ = 0;
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::begin() noexcept -> iterator
{
    if (empty()) {
        return {nullptr, nullptr};
    }
    auto& b = blocks_.front();
    return { b.begin_, &b };
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::begin() const noexcept -> const_iterator
{
    if (empty()) {
        return {nullptr, nullptr};
    }
    auto& b = blocks_.front();
    return { b.begin_, &b };
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::cbegin() const noexcept -> const_iterator
{
    if (empty()) {
        return {nullptr, nullptr};
    }
    auto& b = blocks_.front();
    return { b.begin_, &b };
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::end() noexcept -> iterator
{
    if (empty()) {
        return {nullptr, nullptr};
    }
    auto& b = blocks_.back(); return { end_, &b };
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::end() const noexcept -> const_iterator
{
    if (empty()) {
        return {nullptr, nullptr};
    }
    auto& b = blocks_.back();
    return { end_, &b };
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::cend() const noexcept -> const_iterator
{
    if (empty()) {
        return {nullptr, nullptr};
    }
    auto& b = blocks_.back();
    return { end_, &b };
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::rbegin() noexcept -> reverse_iterator
{
    return std::reverse_iterator(end());
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::rbegin() const noexcept -> const_reverse_iterator
{
    return std::reverse_iterator(end());
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::rend() noexcept -> reverse_iterator
{
    return std::reverse_iterator(begin());
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::rend() const noexcept -> const_reverse_iterator
{
    return std::reverse_iterator(begin());
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::erase(iterator pos) noexcept -> iterator
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

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::erase(iterator ib, iterator ie) noexcept -> iterator
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

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::get_allocator() const noexcept -> allocator_type
{
    return allocator_;
}

template <typename T, typename Alloc>
auto stable_vector<T, Alloc>::element_at(std::size_t idx) const noexcept -> reference
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

template <typename T, typename Alloc>
void stable_vector<T, Alloc>::delete_all() noexcept
{
    delete_all(blocks_, end_);
}

template <typename T, typename Alloc> template <typename Vector>
void stable_vector<T, Alloc>::delete_all(Vector& blocks, pointer end) noexcept
{
    allocator_type allocator = blocks.get_allocator();
    while (!blocks.empty())
    {
        auto& last_block = blocks.back();
        if (!last_block.last_)
        {
            end = last_block.end_;
        }
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            while (end != last_block.begin_)
            {
                --end;
                std::destroy_at(end);
            }
        }
        const auto idx = blocks.size() - 1;
        allocator.deallocate(last_block.begin_, 1U << idx);
        blocks.pop_back();
    }
}

template <typename T, typename Alloc> template <typename ... Ts>
auto stable_vector<T, Alloc>::grow(Ts&& ... ts) -> reference
{
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
        shrink();
        throw;
    }
    ++size_;
    return *end_++;
}

template <typename T, typename Alloc>
void stable_vector<T, Alloc>::shrink()
{
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

}

template <typename T, typename Alloc> template <typename TT>
auto stable_vector<T, Alloc>::iterator_t<TT>::operator++() noexcept -> iterator_t&
{
    ++current_element;
    if (current_element == current_block->end_ && !current_block->last_)
    {
        ++current_block;
        current_element = current_block->begin_;
    }
    return *this;
}

template <typename T, typename Alloc> template <typename TT>
auto stable_vector<T, Alloc>::iterator_t<TT>::operator++(int) noexcept -> iterator_t
{
    auto copy = *this;
    ++*this;
    return copy;
}

template <typename T, typename Alloc> template <typename TT>
auto stable_vector<T, Alloc>::iterator_t<TT>::operator--() noexcept -> iterator_t&
{
    if (current_element == current_block->begin_)
    {
        --current_block;
        current_element = current_block->end_;
    }
    --current_element;
    return *this;
}

template <typename T, typename Alloc> template <typename TT>
auto stable_vector<T, Alloc>::iterator_t<TT>::operator--(int) noexcept -> iterator_t
{
    auto copy = *this;
    --*this;
    return copy;
}

template <typename T, typename Alloc> template <typename TT>
stable_vector<T, Alloc>::iterator_t<TT>::operator iterator_t<const TT>() const noexcept
{
    return { current_element, current_block };
}

template <typename T, typename Alloc> template <typename TT>
auto stable_vector<T, Alloc>::iterator_t<TT>::operator*() const noexcept -> reference
{
    return *current_element;
}

template <typename T, typename Alloc> template <typename TT>
auto stable_vector<T, Alloc>::iterator_t<TT>::operator->() const noexcept -> pointer
{
    return current_element;
}

template <typename T, typename Alloc> template <typename TT>
stable_vector<T, Alloc>::iterator_t<TT>::iterator_t(pointer e, const block* b)
: current_element(e)
, current_block(b)
{
}

namespace pmr
{
template <typename T>
using stable_vector = ::stable_vector<T, std::pmr::polymorphic_allocator<T>>;
}

template <typename T, typename Alloc, typename A2>
stable_vector(stable_vector<T, Alloc>, A2) -> stable_vector<T, Alloc>;

template <std::ranges::range R, typename A = std::allocator<typename R::value_type>>
stable_vector(R, A = {}) -> stable_vector<typename R::value_type, A>;
#endif //STABLE_VECTOR_STABLE_VECTOR_HPP_INCLUDED
