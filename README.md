# WIP: C++20 constexpr vector

Not meant to be a patch to any existing stdlibs
as this project liberally uses C++20 features
that will not compile on older compilers.

Mainly a project to show that it is indeed possible
to write a constexpr vector in GCC / Clang, it's just very tedious. :)

# Roadmap

- (DONE) Proof of concept
- (DONE) Allocator aware
- Finish implementing the last few modifier functions
- Write a constexpr test suite as well as integrate with some runtime test runner
- Write some simple benchmarks against std::vector
- (Maybe?) Write some compile-time benchmarks
