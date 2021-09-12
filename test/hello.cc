#include "constexpr_containers/vector.h"
#include <array>
#include <iostream>

namespace cec = constexpr_containers;

constexpr int f()
{
  cec::vector<int> v{ 1, 2 };
  v.push_back(3);
  return v[0] + v[1] + v[2];
}

int main()
{
  // constexpr containers can be used in both runtime and compile time contexts

  // runtime context:
  cec::vector<int> v{ 1, 2 };

  std::cout << v.size() << std::endl;
  std::cout << v[0] << std::endl;
  std::cout << v[1] << std::endl;
  std::cout << v[2] << std::endl;
  std::cout << std::endl;

  v.push_back(3);

  std::cout << v.size() << std::endl;
  std::cout << v[0] << std::endl;
  std::cout << v[1] << std::endl;
  std::cout << v[2] << std::endl;

  // compile time context:
  std::array<char, f()> arr;
  return sizeof(arr);
}
