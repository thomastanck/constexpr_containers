#pragma once

#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace constexpr_containers {

template<typename Iterator>
using iterator_value_t = typename std::iterator_traits<Iterator>::value_type;

// Contains algorithms useful for container classes.
//
// Synopsis:
//
// make_range( begin, end )
//   Returns a range that you can use with range for loop or other std::range
//   stuff
// zip_transform(dst, fst, fst_end, [snd, third, rest...], n-ary op)
//   Applies op on each element in the specified ranges, if snd, third, etc are
//   at least as long as fst..fst_end, inserting results into dst
// zip_foreach(fst, fst_end, [snd, third, rest...], n-ary op)
//   Applies op on each element in the specified ranges, if snd, third, etc are
//   at least as long as fst..fst_end
// uninitialized_copy(src, src_end, dst)
//   Like std::uninitialized_copy, but supports a custom allocator
// uninitialized_move(src, src_end, dst)
//   Like std::uninitialized_move, but supports a custom allocator
// uninitialized_move_if_noexcept(src, src_end, dst)
//   Like the above but with move_if_noexcept
//
// *_launder
//   Like the above, but where the pointers in src..src_end are laundered

template<std::input_or_output_iterator It, std::input_or_output_iterator It2>
[[nodiscard]] constexpr //
  auto
  make_range(It begin, It2 end) //
  noexcept(std::is_nothrow_constructible_v<std::decay_t<It>, It&&>and
             std::is_nothrow_constructible_v<std::decay_t<It2>, It2&&>)
{
  using InnerIt = std::decay_t<It>;
  using InnerIt2 = std::decay_t<It2>;
  struct Range
  {
    InnerIt m_begin;
    InnerIt2 m_end;

    [[nodiscard]] constexpr InnerIt begin() const noexcept { return m_begin; }
    [[nodiscard]] constexpr InnerIt2 end() const noexcept { return m_end; }
  };
  return Range{ std::forward<It>(begin), std::forward<It2>(end) };
}

template<std::input_or_output_iterator OutputIt,
         std::input_iterator FstIt,
         typename Op,
         std::input_iterator... RestIt>
constexpr //
  OutputIt
  zip_transform(FstIt fst, FstIt fst_end, OutputIt dst, Op op, RestIt... rest)
{
  for (; fst != fst_end; ++dst, ++fst, (++rest, ...)) {
    *dst = op(*fst, *rest...);
  }
  return dst;
}

template<std::input_iterator FstIt, typename Op, std::input_iterator... RestIt>
constexpr //
  void
  zip_foreach(FstIt fst, FstIt fst_end, Op op, RestIt... rest)
{
  for (; fst != fst_end; ++fst, (++rest, ...)) {
    op(*fst, *rest...);
  }
}

template<std::input_iterator InputIt,
         std::input_or_output_iterator OutputIt,
         typename Allocator = std::allocator<iterator_value_t<OutputIt>>>
constexpr //
  OutputIt
  uninitialized_copy(InputIt src, InputIt src_end, OutputIt dst, Allocator alloc)
{
  for (; src != src_end; ++src, ++dst) {
    std::allocator_traits<Allocator>::construct(alloc, dst, *src);
  }
  return dst;
}

template<std::input_iterator InputIt,
         std::input_or_output_iterator OutputIt,
         typename Allocator = std::allocator<iterator_value_t<OutputIt>>>
constexpr //
  OutputIt
  uninitialized_move(InputIt src, InputIt src_end, OutputIt dst, Allocator alloc)
{
  for (; src != src_end; ++src, ++dst) {
    std::allocator_traits<Allocator>::construct(alloc, dst, std::move(*src));
  }
  return dst;
}

template<std::input_iterator InputIt,
         std::input_or_output_iterator OutputIt,
         typename Allocator = std::allocator<iterator_value_t<OutputIt>>>
constexpr //
  OutputIt
  uninitialized_move_if_noexcept(InputIt src, InputIt src_end, OutputIt dst, Allocator alloc)
{
  for (; src != src_end; ++src, ++dst) {
    std::allocator_traits<Allocator>::construct(alloc, dst, std::move_if_noexcept(*src));
  }
  return dst;
}

template<std::input_iterator InputIt,
         std::input_or_output_iterator OutputIt,
         typename Allocator = std::allocator<iterator_value_t<OutputIt>>>
constexpr //
  OutputIt
  uninitialized_copy_launder(InputIt src, InputIt src_end, OutputIt dst, Allocator alloc)
{
  for (; src != src_end; ++src, ++dst) {
    std::allocator_traits<Allocator>::construct(alloc, dst, *std::launder(src));
  }
  return dst;
}

template<std::input_iterator InputIt,
         std::input_or_output_iterator OutputIt,
         typename Allocator = std::allocator<iterator_value_t<OutputIt>>>
constexpr //
  OutputIt
  uninitialized_move_launder(InputIt src, InputIt src_end, OutputIt dst, Allocator alloc)
{
  for (; src != src_end; ++src, ++dst) {
    std::allocator_traits<Allocator>::construct(alloc, dst, std::move(*std::launder(src)));
  }
  return dst;
}

template<std::input_iterator InputIt,
         std::input_or_output_iterator OutputIt,
         typename Allocator = std::allocator<iterator_value_t<OutputIt>>>
constexpr //
  OutputIt
  uninitialized_move_if_noexcept_launder(InputIt src,
                                         InputIt src_end,
                                         OutputIt dst,
                                         Allocator alloc)
{
  for (; src != src_end; ++src, ++dst) {
    std::allocator_traits<Allocator>::construct(
      alloc, dst, std::move_if_noexcept(*std::launder(src)));
  }
  return dst;
}

} // namespace constexpr_containers
