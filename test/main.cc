#include <array>
#include <iostream>
#include <iterator>

#include "constexpr_containers/algorithm.h"
#include "constexpr_containers/vector.h"

constexpr auto f()
{
  constexpr_containers::vector<int> v(10);
  constexpr_containers::vector<int> v2(v);
  v.push_back(1);
  v.emplace_back(2);
  v.pop_back();
  v2 = v;
  return 1;
}

constexpr auto h()
{
  constexpr_containers::vector<int> v1(10);
  constexpr_containers::vector<int> v2(10);
  constexpr_containers::vector<int> v3(10);
  v1.clear();
  constexpr_containers::zip_transform( //
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
  std::cout << sizeof(constexpr_containers::vector<int>) << '\n';
  constexpr_containers::vector<int> v;
  for (auto&& elem : constexpr_containers::make_range(v.begin(), v.end())) {
    elem = 1;
  }
}
