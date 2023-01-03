#include <stable_vector.hpp>

#include <catch2/catch_test_macros.hpp>


#include <memory>
#include <algorithm>

struct immobile {
    immobile& operator=(immobile&&) = delete;
};
static_assert(!std::is_copy_constructible_v<stable_vector<std::unique_ptr<int>>>,
    "A vector of non-copyable types is not copy constructible");
static_assert(!std::is_copy_assignable_v<stable_vector<std::unique_ptr<int>>>,
    "A vector of non-copyable types is not copy assignable");
static_assert(std::is_move_constructible_v<stable_vector<immobile>>,
    "A vector of non-movable types is move_constructible");
static_assert(std::is_move_constructible_v<stable_vector<immobile>>,
    "A vector of non-movable types is move assignable");

static_assert(!std::is_invocable_v<decltype([](auto&& x) -> decltype(std::declval<stable_vector<std::unique_ptr<int>>&>().push_back(x)){}), std::unique_ptr<int>&>,
    "An lvalue of a move-only type cannot be push_back:ed");
static_assert(std::is_invocable_v<decltype([]<typename T>(T&& x) -> decltype(std::declval<stable_vector<std::unique_ptr<int>>&>().push_back(std::forward<T>(x))){}), std::unique_ptr<int>&&>,
              "An rvalue of a move-only type can be push_back:ed");

TEST_CASE("grow")
{
    for (size_t size = 0; size != 100; ++size)
    {
        stable_vector<size_t> v;
        for (size_t n = 0; n != size; ++n)
        {
            v.push_back(n);
        }
        size_t n = 0;
        for (auto& e : v)
        {
            REQUIRE(n == e);
            ++n;
        }
        for (auto i = v.rbegin(); i != v.rend(); ++i)
        {
            --n;
            REQUIRE(*i == n);
        }
    }
}

TEST_CASE("a default constructed vector is empty")
{
    stable_vector<int> v;
    REQUIRE(v.empty());
    REQUIRE(v.size() == 0);
    REQUIRE(v.begin() == v.end());
}

TEST_CASE("pushed elements can be accessed using operator[]")
{
    GIVEN("a vector with elements")
    {
        stable_vector<size_t> v;
        for (size_t i = 0; i < 32; ++i) {
            v.push_back(i);
        }
        WHEN("accessed as const instance")
        {
            const auto& vc = v;
            THEN("all elements are readable")
            {
                for (size_t i = 0; i < 32; ++i) {
                    REQUIRE(vc[i] == i);
                }
            }
        }
        AND_WHEN("accessed as non-const instance")
        {
            THEN("all elements are modifiable")
            {
                for (size_t i = 0; i < 32; ++i)
                {
                    v[i]++;
                    REQUIRE(v[i] == i+1);
                }
            }
        }
    }
}

TEST_CASE("front returrns the 1st element")
{
    stable_vector<int> v{1,2,3,4};
    REQUIRE(v.front() == 1);
    REQUIRE(&v.front() == &v[0]);
    REQUIRE(&std::as_const(v).front() == &v[0]);
}

TEST_CASE("bac returns the last element")
{
    stable_vector<int> v{1,2,3,4};
    REQUIRE(v.back() == 4);
    REQUIRE(&v.back() == &v[3]);
    REQUIRE(&std::as_const(v).back() == &v[3]);
}

TEST_CASE("pushed elements can be visited with forward iteration")
{
    GIVEN("a vector with elements")
    {
        stable_vector<size_t> v;
        for (size_t i = 0; i < 32; ++i)
        {
            v.push_back(i);
        }
        WHEN("looped over a constant instance")
        {
            const auto& vc = v;
            THEN("all elements are readable in the order pushed")
            {
                for (size_t i = 0; auto &elem: vc)
                {
                    REQUIRE(&elem == &vc[i]);
                    ++i;
                }
            }
        }
        AND_WHEN("looped with explicit const iterators")
        {
            auto i = v.cbegin();
            auto e = v.cend();
            THEN("all elements are readable in the order pushed")
            {
                for (size_t n = 0; i != e;)
                {
                    REQUIRE(*i++ == n++);
                }
            }
        }
        AND_WHEN("looped over a non-constant instance")
        {
            THEN("all elements are modifiable in the order pushed")
            {
                for (size_t i = 0; auto& elem : v)
                {
                    elem+= 1;
                    REQUIRE(v[i] == i+1U);
                    ++i;
                }
            }
        }
    }
}

TEST_CASE("pushed elements can be visited with backward iteration")
{
    GIVEN("a vector with elements")
    {
        stable_vector<int> v;
        for (int i = 0; i < 32; ++i)
        {
            v.push_back(i);
        }
        WHEN("looped over a constant instance")
        {
            const auto& vc = v;
            THEN("all elements are readable in the order pushed")
            {
                size_t i = 31;
                const auto e = vc.rend();
                for (auto iter = vc.rbegin(); iter != e; ++iter)
                {
                    auto& elem = *iter;
                    REQUIRE(&elem == &vc[i]);
                    --i;
                }
            }
        }
        AND_WHEN("looped over a non-constant instance")
        {
            THEN("all elements are modifiable in the order pushed")
            {
                size_t i = 31;
                const auto e = v.rend();
                for (auto iter = v.rbegin(); iter != e; ++iter)
                {
                    auto& elem = *iter;
                    REQUIRE(&elem == &v[i]);
                    --i;
                }
            }
        }
        AND_WHEN("begin is reachable from end")
        {
            auto e = v.end();
            auto b = v.begin();
            int i = 31;
            while (e != b)
            {
                e--;
                REQUIRE(i-- == *e);
            }
        }
    }
}

TEST_CASE("a const_iterator can be constructed from an iterator")
{
    using V = stable_vector<int>;
    V v{1,2,3};
    V::iterator i = v.begin();
    V::const_iterator ci = i;
    STATIC_REQUIRE(!std::is_constructible_v<V::iterator, V::const_iterator>);
    STATIC_REQUIRE(!std::is_assignable_v<V::iterator, V::const_iterator>);
    REQUIRE(i == ci);
    REQUIRE(&*i == &*ci);
}

TEST_CASE("copy constructor allocates new objects, copied from the original")
{
    GIVEN("a vector with elements")
    {
        stable_vector<int> orig;
        for (int i = 0; i < 32; ++i)
        {
            orig.push_back(i);
        }
        WHEN("a copy is made")
        {
            auto copy = orig;
            THEN("copy has same size as orig")
            {
                REQUIRE(orig.size() == copy.size());
            }
            AND_THEN("all elements have equal values but unique addresses")
            {
                auto i_orig = orig.begin();
                auto e_orig = orig.end();
                auto i_copy = copy.begin();
                while (i_orig != e_orig)
                {
                    REQUIRE(*i_orig == *i_copy);
                    REQUIRE(&*i_orig != &*i_copy);
                    ++i_orig;
                    ++i_copy;
                }
            }
        }
    }
}

TEST_CASE("copy assignment allocates new objects, copied from the original")
{
    GIVEN("two vectors with elements")
    {
        stable_vector<int> orig;
        stable_vector<int> copy;
        for (int i = 0; i < 32; ++i)
        {
            orig.push_back(i);
            copy.push_back(-i);
        }
        copy.push_back(0);
        WHEN("a copy assignment is made")
        {
            copy = orig;
            THEN("copy has same size as orig")
            {
                REQUIRE(orig.size() == copy.size());
            }
            AND_THEN("all elements have equal values but unique addresses")
            {
                auto i_orig = orig.begin();
                auto e_orig = orig.end();
                auto i_copy = copy.begin();
                while (i_orig != e_orig)
                {
                    REQUIRE(*i_orig == *i_copy);
                    REQUIRE(&*i_orig != &*i_copy);
                    ++i_orig;
                    ++i_copy;
                }
            }
        }
    }
}

TEST_CASE("move constructor moves the actual blocks of objects and leaves the source empty")
{
    std::vector<void*> addresses;
    stable_vector<int> source;
    for (int i = 0; i < 32; ++i)
    {
        addresses.push_back(&source.push_back(i));
    }
    WHEN("a move is made")
    {
        auto old_size = source.size();
        auto dest = std::move(source);
        THEN("copy has same size as orig had")
        {
            REQUIRE(old_size == dest.size());
        }
        AND_THEN("orig is empty")
        {
            REQUIRE(source.empty());
            REQUIRE(source.size() == 0);
            REQUIRE(source.begin() == source.end());
        }
        AND_THEN("all elements have the same addresses as before")
        {
            for (size_t i = 0; i != addresses.size(); ++i)
            {
                REQUIRE(&dest[i] == addresses[i]);
            }
        }
    }
}

TEST_CASE("move assignment moves tha actial block of objects and leaves the source empty")
{
    GIVEN("two vectors with elements")
    {
        std::vector<void*> addresses;
        stable_vector<int> source;
        stable_vector<int> dest;
        for (int i = 0; i < 32; ++i)
        {
            addresses.push_back(&source.push_back(i));
            dest.push_back(-i);
        }
        dest.push_back(0);
        WHEN("a move assignment is made")
        {
            auto orig_source_size = source.size();
            dest = std::move(source);
            THEN("dest has same size as source had")
            {
                REQUIRE(orig_source_size == dest.size());
            }
            AND_THEN("source is empty")
            {
                REQUIRE(source.empty());
                REQUIRE(source.size() == 0);
                REQUIRE(source.begin() == source.end());
            }
            AND_THEN("all elements in dest have the same addresses as before")
            {
                for (size_t i = 0; i != addresses.size(); ++i)
                {
                    REQUIRE(&dest[i] == addresses[i]);
                }
            }
        }
    }
}

struct throw_on_copy
{
    int throw_;
    std::shared_ptr<int> p = std::make_shared<int>(3);
    throw_on_copy(int b) : throw_(b) {}
    throw_on_copy(const throw_on_copy& orig) : throw_(orig.throw_), p(orig.p) { if (throw_ < 0) { throw "foo"; }}
    throw_on_copy& operator=(const throw_on_copy& orig) { if (orig.throw_ < 0) throw "foo"; p = orig.p; return *this; }
};

TEST_CASE("element throwing during copy construction deallocates and throws from constructor")
{
    for (int i = 0; i < 16; ++i)
    {
        stable_vector<throw_on_copy> src;
        for (int j = 0; j < i; ++j)
        {
            src.emplace_back(0);
        }
        src.emplace_back(-1);
        REQUIRE_THROWS(stable_vector(src));
    }
}

TEST_CASE("element throwing during copy assign leaves dest in previous state and throws")
{
    stable_vector<throw_on_copy> src, dest;
    src.emplace_back(0);dest.emplace_back(1);
    src.emplace_back(0);dest.emplace_back(2);
    src.emplace_back(-1);dest.emplace_back(3);
    src.emplace_back(0);dest.emplace_back(4);

    REQUIRE_THROWS(dest = src);

    REQUIRE(dest[0].throw_ == 1);
    REQUIRE(dest[1].throw_ == 2);
    REQUIRE(dest[2].throw_ == 3);
    REQUIRE(dest[3].throw_ == 4);
    REQUIRE(src[0].throw_ == 0);
    REQUIRE(src[1].throw_ == 0);
    REQUIRE(src[2].throw_ == -1);
    REQUIRE(src[3].throw_ == 0);
}

TEST_CASE("pop_back removes the last element")
{
    stable_vector<int> v;
    for (int i = 0; i < 32; ++i)
    {
        v.push_back(i);
    }
    int i = 31;
    while (!v.empty())
    {
        REQUIRE(v.back() == i);
        REQUIRE(std::as_const(v).back() == i);
        v.pop_back();
        --i;
    }
}

TEST_CASE("throwing constructor on push_back leaves the vector as it was and throws")
{
    for (int i = 0; i < 32; ++i)
    {
        stable_vector<throw_on_copy> v;

        for (int j = 0; j < i; ++j)
        {
            v.push_back(j);
        }
        REQUIRE_THROWS(v.push_back(-1));
        for (int j = 0; j < i; ++j)
        {
            REQUIRE(v[size_t(j)].throw_ == j);
        }
    }
}

template <typename T>
struct convertible
{
    T t;
    operator const T&() const { return t; }
};

TEST_CASE("a vector can be constructed from a compatible range")
{
    convertible<int> src[]{{1},{2},{3},{4},{5},{6}};
    stable_vector<int> v(src);
}

TEST_CASE("an element that throws during range construction deallocates and throws from constructor")
{
    throw_on_copy source[]{0,1,2,3,4,-1,0};
    REQUIRE_THROWS(stable_vector<throw_on_copy>(source));
}

TEST_CASE("single iterator erase move assigns elements one closer to begin")
{
    GIVEN("a vector with data")
    {
        stable_vector<std::unique_ptr<size_t>> v;
        for (size_t i = 0; i != 10; ++i) {
            v.push_back(std::make_unique<size_t>(i));
        }
        WHEN("erasing an element in the middle")
        {
            auto it = std::find_if(v.begin(), v.end(),
                                  [](const auto &i) { return *i == 3; });
            it = v.erase(it);
            THEN("the returned iterator refers to the element that was after the erased")
            {
                REQUIRE(**it == 4U);
            }
            AND_THEN("the size is reduced by one")
            {
                REQUIRE(v.size() == 9);
            }
            AND_THEN("elements before the erase position are untouched")
            {
                REQUIRE(*v[0] == 0);
                REQUIRE(*v[1] == 1);
                REQUIRE(*v[2] == 2);
            }
            AND_THEN("elements after the erased position are moved one step closer to begin")
            {
                REQUIRE(*v[3] == 4);
                REQUIRE(*v[4] == 5);
                REQUIRE(*v[5] == 6);
                REQUIRE(*v[6] == 7);
                REQUIRE(*v[7] == 8);
                REQUIRE(*v[8] == 9);
            }
        }
        AND_WHEN("erasing the end iterator")
        {
            auto i = v.erase(v.end());
            THEN("the size remains as before")
            {
                REQUIRE(v.size() == 10);
            }
            AND_THEN("the returned iterator is the end iterator")
            {
                REQUIRE(i == v.end());
            }
            AND_THEN("all data is intact")
            {
                for (size_t n = 0; n != 10; ++n)
                {
                    REQUIRE(*v[n] == n);
                }
            }
        }
        AND_WHEN("erasing one before the end iterator")
        {
            auto pos = std::prev(v.end());
            auto it = v.erase(pos);
            THEN("the size is shrunk by one")
            {
                REQUIRE(v.size() == 9);
            }
            AND_THEN("the returned iterator is the new end()")
            {
                REQUIRE(it == v.end());
            }
            AND_THEN("the remaining elements are intact as before")
            {
                for (size_t i = 0; i != 9; ++i)
                {
                    REQUIRE(*v[i] == i);
                }
            }
        }
    }
}

TEST_CASE("erase range")
{
    GIVEN("a vector with data")
    {
        stable_vector<std::unique_ptr<size_t>> v;
        for (size_t i = 0; i != 10; ++i)
        {
            v.push_back(std::make_unique<size_t>(i));
        }
        WHEN("erasing end(),end()")
        {
            auto it = v.erase(v.end(), v.end());
            THEN("the vector has the same size")
            {
                REQUIRE(v.size() == 10);
            }
            AND_THEN("the returned iterator is the end iterator")
            {
                REQUIRE(it == v.end());
            }
            AND_THEN("the elemenents are untouched")
            {
                for (size_t i = 0; i != 10; ++i)
                {
                    REQUIRE(*v[i] == i);
                }
            }
        }
        AND_WHEN("erasing begin(),begin()")
        {
            auto it = v.erase(v.begin(), v.begin());
            THEN("the vector has the same size")
            {
                REQUIRE(v.size() == 10);
            }
            AND_THEN("the returned iterator is begin()")
            {
                REQUIRE(it == v.begin());
            }
            AND_THEN("the elements are untouched")
            {
                for (size_t i = 0; i != 10; ++i)
                {
                    REQUIRE(*v[i] == i);
                }
            }
        }
        AND_WHEN("erasing an empty range in the middle")
        {
            auto pos = std::next(v.begin(), 5);
            auto ri = v.erase(pos, pos);
            THEN("the vector has the same size")
            {
                REQUIRE(v.size() == 10);
            }
            AND_THEN("the returned iterator is the argument iterator")
            {
                REQUIRE(ri == pos);
            }
            AND_THEN("the elements are untouched")
            {
                for (size_t i = 0; i != 10; ++i)
                {
                    REQUIRE(*v[i] == i);
                }
            }
        }
        AND_WHEN("erasing begin(),end()")
        {
            auto it = v.erase(v.begin(), v.end());
            THEN("the vector becomes empty")
            {
                REQUIRE(v.empty());
                REQUIRE(v.size() == 0);
                REQUIRE(v.begin() == v.end());
            }
            AND_THEN("the returned iterator is end()")
            {
                REQUIRE(it == v.end());
            }
        }
        AND_WHEN("erasing a range of size 5 in the middle")
        {
            auto it = std::next(v.begin(), 2); // 2
            auto e = std::next(it,5); // 7
            auto ri = v.erase(it,e);
            THEN("the size is reduced by 5")
            {
                REQUIRE(v.size() == 5);
            }
            AND_THEN("the elements before the erased range are untouched")
            {
                REQUIRE(*v[0] == 0);
                REQUIRE(*v[1] == 1);
            }
            AND_THEN("the elements after the erased range are moved towards begin")
            {
                REQUIRE(*v[2] == 7);
                REQUIRE(*v[3] == 8);
                REQUIRE(*v[4] == 9);
            }
            AND_THEN("the returned iterator is the given end of range iterator")
            {
                REQUIRE(ri == e);
            }
        }
    }
}

struct counting_memory_resource : std::pmr::memory_resource
{
    size_t current_allocations = 0;
    size_t current_allocated_bytes = 0;
    size_t allocations = 0;
    size_t allocated_bytes = 0;
    size_t deallocations = 0;
    size_t deallocated_bytes = 0;
private:
    void* do_allocate(size_t bytes, size_t alignment) override
    {
        void* addr = operator new(bytes, std::align_val_t(alignment));
        ++allocations; ++current_allocations;
        allocated_bytes += bytes; current_allocated_bytes+= bytes;
        return addr;
    }
    void do_deallocate(void* addr, size_t bytes, size_t alignment) noexcept override
    {
        ++deallocations; --current_allocations;
        deallocated_bytes+= bytes; current_allocated_bytes -= bytes;
        operator delete(addr, std::align_val_t(alignment));
    }
    bool do_is_equal(const std::pmr::memory_resource& rh) const noexcept override
    {
        return &rh == this;
    }
};

TEST_CASE("clear removes all elements and deallocates all data")
{
    counting_memory_resource mem;
    {
        pmr::stable_vector<int> v({1, 2, 3, 4}, &mem);

        REQUIRE(mem.current_allocations > 0);
        auto old_allocations = mem.current_allocations;
        v.clear();
        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
        REQUIRE(v.begin() == v.end());
        REQUIRE(std::as_const(v).begin() == std::as_const(v).end());
        REQUIRE(v.cbegin() == v.cend());
        REQUIRE(mem.current_allocations < old_allocations);
    }
    REQUIRE(mem.current_allocations == 0);
}

TEST_CASE("PMR forwards to PMR enabled class")
{
    counting_memory_resource mem_nonfwd;
    counting_memory_resource mem_fwd;

    {
        pmr::stable_vector<std::string> vs(&mem_nonfwd);
        pmr::stable_vector<std::pmr::string> v(&mem_fwd);

        v.emplace_back("woohoo a longish string with much nonsense");
        vs.emplace_back("woohoo a longish string with much nonsense");

        REQUIRE(mem_fwd.current_allocations == mem_nonfwd.current_allocations + 1);
        REQUIRE(mem_nonfwd.deallocations == 0);
        REQUIRE(mem_fwd.deallocations == 0);
    }
    REQUIRE(mem_fwd.current_allocations == 0);
    REQUIRE(mem_nonfwd.current_allocations == 0);
}

TEST_CASE("move assign moves when using the same rmemory esource")
{
    counting_memory_resource mem;

    pmr::stable_vector<std::pmr::string> v1(&mem);
    pmr::stable_vector<std::pmr::string> v2(&mem);

    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    v2.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v2.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    auto addr0 = &v2[0];
    auto addr1 = &v2[1];

    v1 = std::move(v2);

    REQUIRE(&v1[0] == addr0);
    REQUIRE(&v1[1] == addr1);
}

TEST_CASE("move assign copies when using different memory resources")
{
    counting_memory_resource mem1;
    counting_memory_resource mem2;

    pmr::stable_vector<std::pmr::string> v1(&mem1);
    pmr::stable_vector<std::pmr::string> v2(&mem2);

    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    v2.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v2.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    auto addr0 = &v2[0];
    auto addr1 = &v2[1];

    v1 = std::move(v2);

    REQUIRE(&v1[0] != addr0);
    REQUIRE(&v1[1] != addr1);
}

TEST_CASE("move construct keeps the memory resource")
{
    counting_memory_resource mem;

    pmr::stable_vector<std::pmr::string> v1(&mem);

    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    auto v2 = std::move(v1);

    REQUIRE(v2.get_allocator().resource() == &mem);
}

TEST_CASE("copy construct with explicit allocator uses that allocator")
{
    counting_memory_resource mem1;

    pmr::stable_vector<std::pmr::string> v1(&mem1);

    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    counting_memory_resource mem2;
    auto v2 = stable_vector(v1, &mem2);

    REQUIRE(v2.get_allocator().resource() == &mem2);

    REQUIRE(mem1.current_allocations == mem2.current_allocations);
}

TEST_CASE("copy construct with explicit allocator returns mem if copy throws")
{
    counting_memory_resource mem1;

    pmr::stable_vector<throw_on_copy> v1(&mem1);

    v1.emplace_back(1);
    v1.emplace_back(2);
    v1.emplace_back(-1); // throws
    v1.emplace_back(4);

    counting_memory_resource mem2;

    REQUIRE_THROWS(stable_vector(v1, &mem2));
    REQUIRE(mem2.current_allocations == 0);
}

TEST_CASE("move construct with explicit allocator moves all when same allocator")
{
    counting_memory_resource mem;

    pmr::stable_vector<std::pmr::string> v1(&mem);

    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    auto* addr2 = &v1[2];
    auto v2 = stable_vector(std::move(v1), &mem);

    REQUIRE(v2.get_allocator().resource() == &mem);
    REQUIRE(&v2[2] == addr2);
}

TEST_CASE("move construct with explicit allocator does elemeentwise move if different allocators")
{
    counting_memory_resource mem1;

    pmr::stable_vector<std::string> v1(&mem1);

    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");
    v1.emplace_back("123456789abcdefghijklmnopqrstuvwxyz");

    auto* addr2 = &v1[2];
    auto* str2 = v1[2].data();

    counting_memory_resource mem2;
    auto v2 = stable_vector(std::move(v1), &mem2);
    REQUIRE(!v1.empty());
    REQUIRE(v2.get_allocator().resource() == &mem2);
    REQUIRE(&v2[2] != addr2);
    REQUIRE(v2[2].data() == str2);
}

TEST_CASE("construct from range and allocator uses said allocator")
{
    counting_memory_resource mem;
    pmr::stable_vector<std::pmr::string> v(
        std::initializer_list<const char*>
            {
            "foo",
            "bar",
            "1234567890abcdefghijklmnopqrstuvwxyz"
        },
        &mem);
    REQUIRE(v.size() == 3);
    REQUIRE(v[0].get_allocator().resource() == &mem);
    REQUIRE(mem.current_allocations > 3);
}

