# WIP: C++20 constexpr vector

Not meant to be a patch to any existing stdlibs
as this project liberally uses C++20 features
that will not compile on older compilers.

Mainly a project to show that it is indeed possible
to write a `constexpr` vector in GCC / Clang, it's just very tedious :)

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

# Notes

Here I dump my notes and thoughts about writing this library.
Think of this as a blog.
Might move this elsewhere if it gets too long.

## New compilers only

Making libraries compatible across multiple C++ standards revisions
is very tough and usually involves macro spam.
This project is primarily a learning one, and so my priorities are
(from most important to least important):

- Compliance to the C++20 standard
- Readable code
- No undefined behaviour
  - except `std::launder` related issues, see below
- Not slow
- Works properly on recent major compilers (i.e. avoids any bugs if possible)

Hitting just the first goal is surprisingly tricky already,
having to keep my language lawyer hat firmly on my head :)

I imagine hitting the last goal will just happen without any action on my part.
We can trust the compiler wizards, right...?

## `std::launder`

I've been liberally using this function everywhere.
If my understanding is correct,
this is required whenever storage persists past the end of lifetime of an object,
and a new object is constructed in the same place.
As pointers to `const` objects (among other things)
do not automatically point to the new object when such an operation occurs,
they need to be "refreshed" by either taking the result of the placement new,
or `std::launder` must be used.

Unfortunately, it seems that this 2017 paper[1] titled "On launder()"
thinks that this is insufficient for containers like `std::vector`.

I had thought it would be possible to solve this issue
with a smart iterator that launders on dereference,
but I think I would rather trust what this paper says.
A smart iterator implementation that does this is planned,
so you can see what I'm talking about after I implement it.

[1] N. Josuttis: P0532R0: On launder(): https://wg21.link/p0532r0

## `"constexpr_containers/algorithm.h"`

There's quite a few variants of `<algorithm>`-like functions missing from the standard library,
perhaps because these aren't really useful for normal programming
despite being pretty useful for container library authors.
Keep this in mind while reading the source
if you get confused by some unusual function signatures,
as they don't necessarily exist in the standard library!

## clang-format

This project uses clang-format to ensure formatting is fast and easy,
but clang-format really doesn't play nicely with:

- extremely long attribute lists,
- `const`/`noexcept` specifiers,
- types,
- long names,
- and concepts...

It leads to the following look when left unchecked:

```c++
  [[nodiscard]] constexpr bool operator==(const vector_base& other) const
    noexcept(noexcept(*begin() == *other.begin())) requires std::equality_comparable<T>
  {
    return std::equal(begin(), end(), other.begin(), other.end());
  }
```

I find this pretty hard to read,
especially with the `const` and `noexcept` specifiers
having been split arbitrarily across multiple lines,
and the requires clause stuck awkwardly to the end.
So I've opted to follow a style
where the different components of a declaration
are manually split across multiple lines according to their "kind":

```c++
  [[nodiscard]] constexpr                                // attributes
    bool                                                 // return type
    operator==(const vector_base& other)                 // function signature
    const noexcept(noexcept(*begin() == *other.begin())) // const / noexcept specifiers (if any)
    requires std::equality_comparable<T>                 // concepts (if any)
  {
    return std::equal(begin(), end(), other.begin(), other.end());
  }
```

To ensure that clang-format doesn't re-wrap the declaration to the original form,
I use line comments to forcibly break up lines:

```c++
  [[nodiscard]] constexpr //
    bool
    operator==(const vector_base& other)                 //
    const noexcept(noexcept(*begin() == *other.begin())) //
    requires std::equality_comparable<T>
  {
    return std::equal(begin(), end(), other.begin(), other.end());
  }
```

Occasionally, there are declarations that are so short they fit on a single line.
In these cases, I won't introduce any line breaks,
but I'm now able to use inline comments to create a "tabular" look.

```c++
  [[nodiscard]] constexpr /***/ reference front() /********/ noexcept { return *m_begin; }
  [[nodiscard]] constexpr const_reference front() /**/ const noexcept { return *m_begin; }
  [[nodiscard]] constexpr /***/ reference back() /*********/ noexcept { return *(m_end - 1); }
  [[nodiscard]] constexpr const_reference back() /***/ const noexcept { return *(m_end - 1); }
```

This does mean that function names are now in the middle of the line
as opposed to at the start,
but I find that having this "tabular" look aids readability
more than breaking each short declaration into 8 lines each.

Using this system, I've been able to achieve a much better source look
with only a very small amount of additional effort.
I do wish clang-format would be able to do these kinds of things out of the box, though.

## Roadmap

- (DONE) Proof of concept
- (DONE) Allocator aware
- Finish implementing the last few modifier functions
- Write a constexpr test suite as well as integrate with some runtime test runner
- Implement deferred launder smart iterator
- Implement optional bounds checked iterators (like MSVC in debug mode)
- Write some simple benchmarks against std::vector
- (Maybe?) Write some compile-time benchmarks
- (Maybe?) Implement some other containers like list
