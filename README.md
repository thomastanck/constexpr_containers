# WIP: C++20 constexpr vector

Not meant to be a patch to any existing stdlibs
as this project liberally uses C++20 features
that will not compile on older compilers.

Mainly a project to show that it is indeed possible
to write a constexpr vector in GCC / Clang, it's just very tedious. :)

# Quick setup

1) Clone this repo somewhere
2) Add `-I/path/to/constexpr_containers/include` somewhere to your build flags
3) Include with `#include "constexpr_containers/vector.h"`
4) Instantiate with `constexpr_containers::vector`

## Example code

hello.cc:
```c++
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
```

# Roadmap

- (DONE) Proof of concept
- (DONE) Allocator aware
- Finish implementing the last few modifier functions
- Write a constexpr test suite as well as integrate with some runtime test runner
- Implement optional bounds checked iterators (like MSVC in debug mode)
- Write some simple benchmarks against std::vector
- (Maybe?) Write some compile-time benchmarks
- (Maybe?) Implement some other containers like list
