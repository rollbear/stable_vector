#include <stable_vector.hpp>

#include <span>
#include <functional>
#include <optional>
#include <memory>
#include <cstring>

class fuzzer
{
public:
  fuzzer(std::span<const uint8_t> data) : input(data) {}
  std::function<void()> next_action()
  {
    if (input.empty())
    {
      return {};
    }
    auto byte = input.front();
    input = input.subspan(1);
    switch (byte % 13)
    {
      case 0:
        if (!v.empty()) {
          log.push_back("pop_back");
          return pop_back;
        }
        [[fallthrough]];
      case 1:
        if (!v.empty()) {
          log.push_back("front");
          return front;
        }
        [[fallthrough]];
      case 2:
        if (!v.empty()) {
          log.push_back("back");
          return back;
        }
      case 3:
        if (!v.empty()) {
          if (auto idx = get_index()) {
            log.push_back("v[]");
            return index_read(*idx);
          }
        }
        [[fallthrough]];
      case 4:
        if (!v.empty()) {
          if (auto idx = get_index()) {
            if (auto c = get_count()) {
              log.push_back("v[]=x");
              return index_write(*idx, *c);
            }
          }
        }
        [[fallthrough]];
      case 5:
        if (!v.empty())
        {
          if (auto idx = get_index())
          {
            log.push_back("erase i");
            return erase(*idx);
          }
        }
        [[fallthrough]];
      case 6:
        if (!v.empty())
        {
          auto id1 = get_index();
          auto id2 = get_index();
          if (id1 && id2)
          {
            log.push_back("erase b,e");
            return erase(*id1, *id2);
          }
        }
        [[fallthrough]];
      case 7:
        if (auto c = get_count()) {
          log.push_back("push_back");
          return push_back(*c);
        }
      case 8:
        log.push_back("move assign");
        return move_assign;
      case 9:
        log.push_back("copy assign");
        return copy_assign;
      case 10:
        log.push_back("iterate forward");
        return iterate_forward;
      case 11:
        log.push_back("iterate backward");
        return iterate_backward;
      case  12:
        log.push_back("swap");
        return swap;
    }
    return {};
  }
private:
  std::optional<size_t> get_index()
  {
    if (input.size() < sizeof(size_t))
    {
      return {};
    }
    auto idx = std::bit_cast<size_t>(input.data());
    input = input.subspan(sizeof(idx));
    return idx % v.size();
  }
  std::optional<int8_t> get_count()
  {
    if (input.empty())
    {
      return {};
    }
    auto v = input.front();
    input = input.subspan(1);
    return v;
  }
  struct throwing
  {
    throwing(uint8_t c) : count(c) { if (count == 0) { throw "construct"; } }
    throwing(const throwing& t) : count(t.count - 1), p(t.p) { if (t.count == 0) { throw "copy";} }
    throwing(throwing&&) = default;
    throwing& operator=(const throwing& t) { count = t.count - 1; if (t.count == 0) { throw "assign";} p = t.p; return *this; }
    throwing& operator=(throwing&&) = default;
    uint8_t count;
    std::shared_ptr<int> p = std::make_shared<int>(3);
  };
  std::vector<const char*> log;
  std::span<const uint8_t> input;
  stable_vector<throwing> v;
  stable_vector<throwing> other;
  std::function<void()> push_back(uint8_t count) { return [this,count](){ try { v.emplace_back(count);} catch (...) {}};}
  std::function<void()> pop_back = [this](){ v.pop_back();};
  std::function<void()> front = [this]() { [[maybe_unused]] auto& val = v.front();};
  std::function<void()> back = [this]() { [[maybe_unused]] auto& val = v.back();};
  std::function<void()> index_read(size_t idx) { return [this,idx] (){ [[maybe_unused]] auto& val = v[idx];};};
  std::function<void()> index_write(size_t idx, uint8_t c) { return [this,idx,c](){ try { v[idx] = throwing(c);} catch (...) {} }; }
  std::function<void()> move_assign = [this]() { other = std::move(v);};
  std::function<void()> copy_assign = [this]() { try { other = v;} catch (...) {} };
  std::function<void()> iterate_forward = [this]() { long long rv = 0; for (auto& p : v) { rv+= p.count;} return rv;};
  std::function<void()> iterate_backward = [this]() { long long rv = 0; for (auto i = v.rbegin(); i != v.rend(); ++i) { rv+= i->count;} return rv;};
  std::function<void()> swap = [this]() { std::swap(v, other);};
  std::function<void()> erase(size_t idx) { return [this,idx](){ auto i = std::next(v.begin(), idx); v.erase(i); }; }
  std::function<void()> erase(size_t id1, size_t id2) { return [this,id1,id2](){ auto b = std::next(v.begin(), std::min(id1,id2)); auto e = std::next(v.begin(), std::max(id1,id2)); v.erase(b,e);};}
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  fuzzer fuzz({data, size});
  while (auto action = fuzz.next_action())
  {
    action();
  }
  return 0;  // Values other than 0 and -1 are reserved for future use.
}
