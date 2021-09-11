#include "simple/container_algorithm.h"
#include "simple/vector.h"
#include <array>
#include <iostream>
#include <iterator>

constexpr auto f()
{
  simple::vector<int> v(10);
  simple::vector<int> v2(v);
  v.push_back(1);
  v.emplace_back(2);
  v.pop_back();
  v2 = v;
  return 1;
}

constexpr auto h()
{
  simple::vector<int> v1(10);
  simple::vector<int> v2(10);
  simple::vector<int> v3(10);
  v1.clear();
  simple::zip_transform( //
    v2.begin(),
    v2.end(),
    std::back_inserter(v1),
    [](auto a, auto b) { return a + b; },
    v3.begin());
  return 1;
}

int main()
{
  [[maybe_unused]] std::array<int, f()> a;
  [[maybe_unused]] std::array<int, h()> c;
  std::cout << sizeof(simple::vector<int>) << '\n';
  simple::vector<int> v;
  for (auto&& elem : simple::make_range(v.begin(), v.end())) {
    elem = 1;
  }
}
