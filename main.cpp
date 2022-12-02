#include <stable_vector.hpp>

#include <catch2/catch_test_macros.hpp>


#include <memory>

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
        stable_vector<int> v;
        for (int i = 0; i < 32; ++i) {
            v.push_back(i);
        }
        WHEN("accessed as const instance")
        {
            const auto& vc = v;
            THEN("all elements are readable")
            {
                for (int i = 0; i < 32; ++i) {
                    REQUIRE(vc[i] == i);
                }
            }
        }
        AND_WHEN("accessed as non-const instance")
        {
            THEN("all elements are modifiable")
            {
                for (int i = 0; i < 32; ++i)
                {
                    v[i]++;
                    REQUIRE(v[i] == i+1);
                }
            }
        }
    }
}

TEST_CASE("pushed elements can be visited with forward iteration")
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
                for (int i = 0; auto &elem: vc)
                {
                    REQUIRE(&elem == &vc[i]);
                    ++i;
                }
            }
        }
        AND_WHEN("looped over a non-constant instance")
        {
            THEN("all elements are modifiable in the order pushed")
            {
                for (int i = 0; auto& elem : v)
                {
                    elem+= 1;
                    REQUIRE(v[i] == i+1);
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
                auto i = 31;
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
                auto i = 31;
                const auto e = v.rend();
                for (auto iter = v.rbegin(); iter != e; ++iter)
                {
                    auto& elem = *iter;
                    REQUIRE(&elem == &v[i]);
                    --i;
                }
            }
        }
    }
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
    throw_on_copy(int b) : throw_(b) {}
    throw_on_copy(const throw_on_copy& orig) : throw_(orig.throw_) { if (throw_ < 0) { throw "foo"; }}
    throw_on_copy& operator=(const throw_on_copy& orig) { if (orig.throw_ < 0) throw "foo"; return *this; }
};

TEST_CASE("element throwing during copy construction deallocates and throws from constructor")
{
    stable_vector<throw_on_copy> src;
    src.emplace_back(0);
    src.emplace_back(0);
    src.emplace_back(-1);
    src.emplace_back(0);

    REQUIRE_THROWS(stable_vector(src));
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
            REQUIRE(v[j].throw_ == j);
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
        stable_vector<std::unique_ptr<int>> v;
        for (int i = 0; i != 10; ++i) {
            v.push_back(std::make_unique<int>(i));
        }
        WHEN("erasing an element in the middle")
        {
            auto i = std::find_if(v.begin(), v.end(),
                                  [](const auto &i) { return *i == 3; });
            i = v.erase(i);
            THEN("the returned iterator refers to the element that was after the erased")
            {
                REQUIRE(**i == 4);
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
                for (int n = 0; n != 10; ++n)
                {
                    REQUIRE(*v[n] == n);
                }
            }
        }
        AND_WHEN("erasing one before the end iterator")
        {
            auto pos = std::prev(v.end());
            auto i = v.erase(pos);
            THEN("the size is shrunk by one")
            {
                REQUIRE(v.size() == 9);
            }
            AND_THEN("the returned iterator is the new end()")
            {
                REQUIRE(i == v.end());
            }
            AND_THEN("the remaining elements are intact as before")
            {
                for (int i = 0; i != 9; ++i)
                {
                    REQUIRE(*v[i] == i);
                }
            }
        }
    }
}