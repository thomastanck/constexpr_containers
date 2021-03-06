#pragma once

#include <algorithm>
#include <compare>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "constexpr_containers/algorithm.h"

namespace constexpr_containers {

template<typename T, typename Allocator>
struct vector_base
{
  //////////////////
  // Member types //
  //////////////////

private:
  // Purely to make notation easier
  using AllocTraitsT = std::allocator_traits<Allocator>;

public:
  using value_type = T;
  using allocator_type = Allocator;
  using size_type = typename AllocTraitsT::size_type;
  using difference_type = typename AllocTraitsT::difference_type;
  using reference = T&;
  using const_reference = const T&;
  using pointer = typename AllocTraitsT::pointer;
  using const_pointer = typename AllocTraitsT::const_pointer;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using reverse_const_iterator = std::reverse_iterator<const_iterator>;
  using comparison_type = std::conditional_t<std::three_way_comparable<T>,
                                             decltype(std::declval<T>() <=> std::declval<T>()),
                                             std::weak_ordering>;

  /////////////////
  // Data layout //
  /////////////////
private:
  pointer m_begin;
  pointer m_end;
  pointer m_realend;
  [[no_unique_address]] Allocator m_alloc;

public:
  //////////////////
  // Constructors //
  //////////////////

  constexpr       //
    vector_base() //
    noexcept(noexcept(Allocator()))
    : m_begin(nullptr)
    , m_end(nullptr)
    , m_realend(nullptr)
    , m_alloc()
  {}

  constexpr explicit                    //
    vector_base(const Allocator& alloc) //
    noexcept
    : m_begin(nullptr)
    , m_end(nullptr)
    , m_realend(nullptr)
    , m_alloc(alloc)
  {}

  constexpr //
    vector_base(size_type count, const T& value, const Allocator& alloc = Allocator())
    : m_alloc(alloc)
  {
    allocate(count, m_alloc);
    for (auto& elem : make_range(m_begin, m_realend)) {
      AllocTraitsT::construct(m_alloc, std::launder(&elem), value);
    }
    m_end = m_realend;
  }

  constexpr explicit //
    vector_base(size_type count, const Allocator& alloc = Allocator())
    : m_alloc(alloc)
  {
    allocate(count, m_alloc);
    for (auto& elem : make_range(m_begin, m_realend)) {
      AllocTraitsT::construct(m_alloc, std::launder(&elem));
    }
    m_end = m_realend;
  }

  // Looser overload that allows any input iterator
  template<std::input_iterator InputIt>
  constexpr //
    vector_base(InputIt first, InputIt last, const Allocator& alloc = Allocator())
    : m_begin(nullptr)
    , m_end(nullptr)
    , m_realend(nullptr)
    , m_alloc(alloc)
  {
    for (const auto& elem : make_range(first, last)) {
      push_back(elem);
    }
  }

  // Tighter overload to reserve up front
  template<std::random_access_iterator RandomAccessIt>
  constexpr //
    vector_base(RandomAccessIt first, RandomAccessIt last, const Allocator& alloc = Allocator())
    : m_alloc(alloc)
  {
    if (last - first > 0) {
      allocate(last - first, m_alloc);
      m_end = uninitialized_copy(first, last, m_begin, m_alloc);
    } else {
      m_begin = m_end = m_realend = nullptr;
    }
  }

  /////////////////////////////////////////////////////////
  // Special member functions (and similar constructors) //
  /////////////////////////////////////////////////////////

  constexpr //
    vector_base(const vector_base& other)
    : vector_base(other.m_begin,
                  other.m_end,
                  AllocTraitsT::select_on_container_copy_construction(other.m_alloc))
  {}

  constexpr //
    vector_base(const vector_base& other, const Allocator& alloc)
    : vector_base(other.m_begin, other.m_end, alloc)
  {}

  constexpr vector_base(std::initializer_list<T> il, const Allocator& alloc = Allocator())
    : vector_base(il.begin(), il.end(), alloc)
  {}

  constexpr                          //
    vector_base(vector_base&& other) //
    noexcept
    : m_begin(other.m_begin)
    , m_end(other.m_end)
    , m_realend(other.m_realend)
    , m_alloc(std::move(other.m_alloc))
  {
    other.m_begin = other.m_end = other.m_realend = nullptr;
  }

  constexpr                                                  //
    vector_base(vector_base&& other, const Allocator& alloc) //
    : m_alloc(alloc)
  {
    if (m_alloc != other.m_alloc) {
      allocate(other.size());
      uninitialized_move(other.m_begin, other.m_end, m_begin, m_alloc);
      m_end = m_realend;
    } else {
      m_begin = other.m_begin;
      m_end = other.m_end;
      m_realend = other.m_realend;
      other.m_begin = other.m_end = other.m_realend = nullptr;
    }
  }

  constexpr //
    vector_base&
    operator=(const vector_base& other)
  {
    // don't self-assign
    if (this != &other) {
      if constexpr (AllocTraitsT::propagate_on_container_copy_assignment::value) {
        if (not AllocTraitsT::is_always_equal::value and m_alloc != other.m_alloc) {
          deallocate();
        }
        allocate_empty(other.size());
        m_alloc = other.m_alloc;
      }

      // we require realloc, so we construct into fresh array directly
      if (other.size() > capacity()) {
        auto tmp = allocate_tmp(other.size(), m_alloc);
        try {
          uninitialized_copy(other.m_begin, other.m_end, tmp, m_alloc);
        } catch (...) {
          AllocTraitsT::deallocate(m_alloc, tmp, other.size());
          throw;
        }
        AllocTraitsT::deallocate(m_alloc, m_begin, capacity());
        m_begin = tmp;
        m_end = m_realend = tmp + other.size();
        return *this;
      }

      // destroy excess
      while (other.size() < size()) {
        pop_back();
      }

      // copy-assign onto existing elements
      auto tmp = other.m_begin;
      for (auto& elem : *this) {
        elem = *std::launder(tmp);
        ++tmp;
      }

      // copy-construct new elements
      while (tmp != other.m_end) {
        push_back(*std::launder(tmp));
        ++tmp;
      }
    }
    return *this;
  }

  constexpr //
    vector_base&
    operator=(vector_base&& other) //
    noexcept(AllocTraitsT::propagate_on_container_move_assignment::value ||
             AllocTraitsT::is_always_equal::value)
  {
    if constexpr (AllocTraitsT::propagate_on_container_move_assignment::value) {
      if (not AllocTraitsT::is_always_equal::value and m_alloc != other.m_alloc) {
        m_alloc = other.m_alloc;
      }
      deallocate();
      m_begin = other.m_begin;
      m_end = other.m_end;
      m_realend = other.m_realend;
      other.m_begin = other.m_end = other.m_realend = nullptr;
    } else {
      if (not AllocTraitsT::is_always_equal::value and m_alloc != other.m_alloc) {
        // We must move-assign elements :(
        if (other.size() > capacity()) {
          // We must realloc, so directly move into new buffer
          auto tmp = allocate_tmp(other.size(), m_alloc);
          try {
            uninitialized_move(other.m_begin, other.m_end, tmp, m_alloc);
            deallocate();
            m_begin = tmp;
            m_realend = m_end = tmp + other.size();
          } catch (...) {
            AllocTraitsT::deallocate(m_alloc, tmp, other.size());
            throw;
          }
        } else {
          // destroy excess
          while (other.size() < size()) {
            pop_back();
          }

          // move-assign onto existing elements
          auto tmp = other.m_begin;
          for (auto& elem : *this) {
            elem = std::move(*std::launder(tmp));
            ++tmp;
          }

          // move-construct new elements
          while (tmp != other.m_end) {
            push_back(std::move(*std::launder(tmp)));
            ++tmp;
          }
        }
      } else {
        deallocate();
        m_begin = other.m_begin;
        m_end = other.m_end;
        m_realend = other.m_realend;
        other.m_begin = other.m_end = other.m_realend = nullptr;
      }
    }
    return *this;
  }

  constexpr //
    vector_base&
    operator=(std::initializer_list<T> il)
  {
    if (il.size() > capacity()) {
      // We must realloc, so directly move into new buffer
      auto tmp = allocate_tmp(il.size(), m_alloc);
      try {
        uninitialized_move(il.begin(), il.end(), tmp, m_alloc);
        deallocate();
        m_begin = tmp;
        m_realend = m_end = tmp + il.size();
      } catch (...) {
        AllocTraitsT::deallocate(m_alloc, tmp, il.size());
        throw;
      }
    } else {
      // destroy excess
      while (il.size() < size()) {
        pop_back();
      }

      // copy-assign onto existing elements
      auto tmp = il.begin();
      for (auto& elem : *this) {
        elem = *tmp;
        ++tmp;
      }

      // copy-construct new elements
      while (tmp != il.end()) {
        push_back(*tmp);
        ++tmp;
      }
    }
  }

  constexpr //
    void
    swap(vector_base& other) //
    noexcept(AllocTraitsT::propagate_on_container_swap::value ||
             AllocTraitsT::is_always_equal::value)
  {
    if constexpr (AllocTraitsT::propagate_on_container_swap::value) {
      using std::swap;
      swap(m_alloc, other.m_alloc);
    }
    // We're allowed to UB if m_alloc != other.m_alloc and propagate is false
    // This is cause swap must be constant time, if propagate is false and allocs are not equal
    // we would be forced to copy / move (and thus not be constant time anymore)
    std::swap(m_begin, other.m_begin);
    std::swap(m_end, other.m_end);
    std::swap(m_realend, other.m_realend);
  }

  friend //
    void
    swap(vector_base& a, vector_base& b) //
    noexcept(AllocTraitsT::propagate_on_container_swap::value ||
             AllocTraitsT::is_always_equal::value)
  {
    a.swap(b);
  }

  constexpr ~vector_base() { deallocate(); }

private:
  constexpr //
    void
    check_range(size_type n) //
    const
  {
    if (n >= size()) {
      // TODO: do fancier formatting when I implement constexpr string (?)
      throw std::out_of_range("Bounds check failed.");
    }
  }

public:
  [[nodiscard]] constexpr //
    reference
    at(size_type pos)
  {
    check_range(pos);
    return *this[pos];
  }
  [[nodiscard]] constexpr //
    const_reference
    at(size_type pos) //
    const
  {
    check_range(pos);
    return *this[pos];
  }

  [[nodiscard]] constexpr //
    reference
    operator[](size_type pos) //
    noexcept
  {
    return *std::launder(m_begin + pos);
  }
  [[nodiscard]] constexpr //
    const_reference
    operator[](size_type pos) //
    const noexcept
  {
    return *std::launder(m_begin + pos);
  }

  /////////////
  // Getters //
  /////////////

  [[nodiscard]] constexpr /********/ pointer data() /************/ noexcept { return m_begin; }
  [[nodiscard]] constexpr /**/ const_pointer data() /******/ const noexcept { return m_begin; }
  [[nodiscard]] constexpr /******/ Allocator get_allocator() const noexcept { return m_alloc; }

  [[nodiscard]] constexpr /***/ reference front() /********/ noexcept { return *m_begin; }
  [[nodiscard]] constexpr const_reference front() /**/ const noexcept { return *m_begin; }
  [[nodiscard]] constexpr /***/ reference back() /*********/ noexcept { return *(m_end - 1); }
  [[nodiscard]] constexpr const_reference back() /***/ const noexcept { return *(m_end - 1); }

  [[nodiscard]] constexpr /***/ iterator begin() /*********/ noexcept { return m_begin; }
  [[nodiscard]] constexpr const_iterator begin() /***/ const noexcept { return m_begin; }
  [[nodiscard]] constexpr /***/ iterator end() /***********/ noexcept { return m_end; }
  [[nodiscard]] constexpr const_iterator end() /*****/ const noexcept { return m_end; }
  [[nodiscard]] constexpr const_iterator cbegin() /**/ const noexcept { return m_begin; }
  [[nodiscard]] constexpr const_iterator cend() /****/ const noexcept { return m_end; }

  [[nodiscard]] constexpr /***/ reverse_iterator rbegin() /*********/ noexcept { return m_end; }
  [[nodiscard]] constexpr reverse_const_iterator rbegin() /***/ const noexcept { return m_end; }
  [[nodiscard]] constexpr /***/ reverse_iterator rend() /***********/ noexcept { return m_begin; }
  [[nodiscard]] constexpr reverse_const_iterator rend() /*****/ const noexcept { return m_begin; }
  [[nodiscard]] constexpr reverse_const_iterator crbegin() /**/ const noexcept { return m_end; }
  [[nodiscard]] constexpr reverse_const_iterator crend() /****/ const noexcept { return m_begin; }

  [[nodiscard]] constexpr size_type size() /******/ const noexcept { return m_end - m_begin; }
  [[nodiscard]] constexpr size_type capacity() /**/ const noexcept { return m_realend - m_begin; }
  [[nodiscard]] constexpr bool empty() /**********/ const noexcept { return size() == 0; }
  [[nodiscard]] constexpr //
    size_type
    max_size() //
    const
  {
    const size_type diffmax = std::numeric_limits<difference_type>::max() / sizeof(T);
    const size_type allocmax = AllocTraitsT::max_size(m_alloc);
    return std::min(diffmax, allocmax);
  }

  ////////////////////
  // Size modifiers //
  ////////////////////

  constexpr //
    void
    reserve(size_type new_cap)
  {
    if (new_cap > capacity()) {
      auto tmp = allocate_tmp(new_cap, m_alloc);
      try {
        auto end = uninitialized_move_if_noexcept_launder(m_begin, m_end, tmp, m_alloc);
        deallocate();
        m_begin = tmp;
        m_end = end;
        m_realend = tmp + new_cap;
      } catch (...) {
        AllocTraitsT::deallocate(tmp);
        throw;
      }
    }
  }

  constexpr //
    void
    shrink_to_fit()
  {
    auto oldsize = size();
    if (oldsize < capacity()) {
      auto tmp = allocate_tmp(oldsize, m_alloc);
      try {
        auto end = uninitialized_move_launder(m_begin, m_end, tmp, m_alloc);
        deallocate();
        m_begin = tmp;
        m_end = end;
        m_realend = tmp + oldsize;
      } catch (...) {
        AllocTraitsT::deallocate(tmp);
        throw;
      }
    }
  }

  constexpr //
    void
    resize(size_type count)
  {
    if (count > capacity()) {
      auto tmp = allocate_tmp(count, m_alloc);
      try {
        auto end = uninitialized_move_if_noexcept_launder(m_begin, m_end, tmp, m_alloc);
        for (; end < tmp + count; ++end) {
          AllocTraitsT::construct(m_alloc, end);
        }
        deallocate();
        m_begin = tmp;
        m_end = end;
        m_realend = tmp + count;
      } catch (...) {
        AllocTraitsT::deallocate(tmp);
        throw;
      }
    } else if (count > size()) {
      while (size() > count) {
        emplace_back();
      }
    } else {
      while (size() > count) {
        pop_back();
      }
    }
  }

  constexpr //
    void
    resize(size_type count, const value_type& value)
  {
    if (count > capacity()) {
      auto tmp = allocate_tmp(count, m_alloc);
      try {
        // We construct new elements first in case value is part of vector_base
        auto end = tmp + size();
        for (; end < tmp + count; ++end) {
          AllocTraitsT::construct(m_alloc, end, value);
        }
        end = uninitialized_move_if_noexcept_launder(m_begin, m_end, tmp, m_alloc);
        deallocate();
        m_begin = tmp;
        m_end = end;
        m_realend = tmp + count;
      } catch (...) {
        AllocTraitsT::deallocate(tmp);
        throw;
      }
    } else if (count > size()) {
      while (size() > count) {
        emplace_back(value);
      }
    } else {
      while (size() > count) {
        pop_back();
      }
    }
  }

  constexpr //
    void
    clear() //
    noexcept
  {
    while (not empty()) {
      pop_back();
    }
  }

  /////////////////////////
  // Insertion modifiers //
  /////////////////////////

  // Strong exception guarantee
  template<typename... Args>
  constexpr //
    void
    emplace_back(Args&&... args)
  {
    if (m_end < m_realend) {
      AllocTraitsT::construct(m_alloc, std::launder(m_end), std::forward<Args>(args)...);
      ++m_end;
    }

    // Ensure we've fully prepared a tmp buffer before deallocating m_begin
    auto oldsize = size();
    auto newcap = size() * 2 + 1;
    auto tmp = allocate_tmp(newcap, m_alloc);
    try {
      // construct new value into tmp, we should do this first in case input is part of the
      // vector_base
      AllocTraitsT::construct(m_alloc, tmp + oldsize, std::forward<Args>(args)...);
      // move existing values if noexcept, else copy
      uninitialized_move_if_noexcept_launder(m_begin, m_end, tmp, m_alloc);
    } catch (...) {
      AllocTraitsT::deallocate(m_alloc, tmp, newcap);
      throw;
    }
    // buffer is ready, do the swap
    AllocTraitsT::deallocate(m_alloc, m_begin, capacity());
    m_begin = tmp;
    m_end = tmp + oldsize + 1;
    m_realend = tmp + newcap;
  }

  // Strong exception guarantee
  constexpr void push_back(const T& v) { emplace_back(v); }
  // Strong exception guarantee
  constexpr void push_back(T&& v) { emplace_back(std::move(v)); }

  // Conditionally strong exception guarantee
  // as long as value_type is nothrow assignable and constructible either by move or copy.
  template<typename... Args>
  constexpr //
    pointer
    emplace(const_pointer pos, Args&&... args)
  {
    if (pos == m_end) {
      emplace_back(args...);
      return m_end - 1;
    }

    if (m_end == m_realend) {
      // We need to realloc
      auto index = pos - m_begin;
      auto oldsize = size();
      auto newcap = size() * 2 + 1;
      auto tmp = allocate_tmp(newcap, m_alloc);
      try {
        // construct new value into tmp, we should do this first in case input is part of the
        // vector_base
        AllocTraitsT::construct(m_alloc, tmp + index, std::forward<T>(args)...);
        // move existing values if noexcept, else copy
        uninitialized_move_if_noexcept_launder(m_begin, pos, tmp, m_alloc);
        uninitialized_move_if_noexcept_launder(pos, m_end, tmp + index + 1, m_alloc);
      } catch (...) {
        AllocTraitsT::deallocate(m_alloc, tmp, newcap);
        throw;
      }
      // buffer is ready, do the swap
      AllocTraitsT::deallocate(m_alloc, m_begin, capacity());
      m_begin = tmp;
      m_end = tmp + oldsize + 1;
      m_realend = tmp + newcap;
      return m_begin + index;
    }

    // No realloc needed
    // We're allowed to UB if the move / copy constructor / assignment throws
    // ... unfortunately we can't shift the elements first, THEN construct
    // because if the constructor throws we aren't supposed to UB
    // So we start by constructing the element into a temporary that we move into place later.
    auto tmp = T(std::forward<T>(args)...);
    // After this point, everything is either allowed to UB or is noexcept :)

    // Shift elements back
    uninitialized_move_if_noexcept_launder_backward(m_end - 1, m_end, m_end + 1, m_alloc);
    move_if_noexcept_launder_backward(pos, m_end - 1, m_end);
    // Now move the tmp var into place
    *pos = std::move_if_noexcept(tmp);
    return pos;
  }

  constexpr //
    iterator
    insert(const_iterator pos, const T& value)
  {
    insert(pos, 1, value);
  }

  constexpr //
    iterator
    insert(const_iterator pos, T&& value)
  {
    if (pos == m_end) {
      emplace_back(value);
      return m_end - 1;
    }

    if (m_end + 1 < m_realend) {
      // We need to realloc
      auto index = pos - m_begin;
      auto oldsize = size();
      auto newcap = size() * 2 + 1;
      auto tmp = allocate_tmp(newcap, m_alloc);
      try {
        // construct new values into tmp, we should do this first in case input is part of the
        // vector_base
        AllocTraitsT::construct(m_alloc, tmp + index, std::move(value));
        // move existing values if noexcept, else copy
        uninitialized_move_if_noexcept_launder(m_begin, pos, tmp, m_alloc);
        uninitialized_move_if_noexcept_launder(pos, m_end, tmp + index + 1, m_alloc);
      } catch (...) {
        AllocTraitsT::deallocate(m_alloc, tmp, newcap);
        throw;
      }
      // buffer is ready, do the swap
      AllocTraitsT::deallocate(m_alloc, m_begin, capacity());
      m_begin = tmp;
      m_end = tmp + oldsize + 1;
      m_realend = tmp + newcap;
      return m_begin + index;
    }

    // No realloc needed
    // Shift elements back
    uninitialized_move_if_noexcept_launder_backward(m_end - 1, m_end, m_end + 1, m_alloc);
    move_if_noexcept_launder_backward(pos, m_end - 1, m_end);
    // Now move value into place
    pos = std::move(value);
    return pos;
  }

  constexpr //
    iterator
    insert(const_iterator pos, size_type count, const T& value)
  {
    if (count != 0) {
      if (pos == m_end) {
        emplace_back(value);
        return m_end - 1;
      }

      if (m_end + count < m_realend) {
        // We need to realloc
        auto index = pos - m_begin;
        auto oldsize = size();
        auto newcap = size() * 2 + count;
        auto tmp = allocate_tmp(newcap, m_alloc);
        try {
          // construct new values into tmp, we should do this first in case input is part of the
          // vector_base
          for (auto it = tmp + index; it < tmp + index + count; ++it) {
            AllocTraitsT::construct(m_alloc, it, value);
          }
          // move existing values if noexcept, else copy
          uninitialized_move_if_noexcept_launder(m_begin, pos, tmp, m_alloc);
          uninitialized_move_if_noexcept_launder(pos, m_end, tmp + index + count, m_alloc);
        } catch (...) {
          AllocTraitsT::deallocate(m_alloc, tmp, newcap);
          throw;
        }
        // buffer is ready, do the swap
        AllocTraitsT::deallocate(m_alloc, m_begin, capacity());
        m_begin = tmp;
        m_end = tmp + oldsize + count;
        m_realend = tmp + newcap;
        return m_begin + index;
      }

      // No realloc needed
      // Shift elements back
      uninitialized_move_if_noexcept_launder_backward(m_end - count, m_end, m_end + count, m_alloc);
      move_if_noexcept_launder_backward(pos, m_end - count, m_end);
      // Now copy the value into place repeatedly
      std::fill(pos, pos + count, value);
      return pos;
    }
  }

  // Not quite the same as LegacyInputIterator,
  // but this way is easier and shouldn't break any existing code anyway.
  template<std::input_iterator InputIt>
  constexpr //
    iterator
    insert(const_iterator pos, InputIt first, InputIt last)
  {
    // Since input iterator is single pass, we will just insert one at a time the dumb way.
    // TODO: copy first to a vector then call the random_access_iterator overload.

    // Convert iterator to an index first to handle reallocation case
    auto index = std::distance(m_begin, pos);
    while (first != last) {
      insert(m_begin + index, *first);
      ++index;
      ++first;
    }
    return m_begin + index;
  }

  constexpr //
    iterator
    insert(const_iterator pos, std::initializer_list<T> ilist)
  {
    return insert(pos, ilist.begin(), ilist.end());
  }

  ///////////////////////
  // Removal modifiers //
  ///////////////////////

  constexpr //
    void
    pop_back() //
  {
    AllocTraitsT::destroy(m_alloc, std::launder(m_end - 1));
    --m_end;
  }

  constexpr //
    iterator
    erase(const_iterator pos)
  {
    return erase(pos, pos + 1);
  }

  constexpr //
    iterator
    erase(const_iterator first, const_iterator last)
  {
    move_if_noexcept_launder(last, m_end, first, m_alloc);
    for (size_t i = 0; i < last - first; ++i) {
      pop_back();
    }
    return first;
  }

  //////////////////////////
  // Comparison operators //
  //////////////////////////

  [[nodiscard]] constexpr //
    bool
    operator==(const vector_base& other)                 //
    const noexcept(noexcept(*begin() == *other.begin())) //
    requires std::equality_comparable<T>
  {
    return std::equal(begin(), end(), other.begin(), other.end());
  }

  [[nodiscard]] constexpr //
    comparison_type
    operator<=>(const vector_base& other)                //
    const noexcept(noexcept(*begin() == *other.begin())) //
    requires std::three_way_comparable<T> ||             //
    requires(const T& elem)
  {
    elem < elem;
  } //
  {
    if constexpr (std::three_way_comparable<T>) {
      return std::lexicographical_compare_three_way(m_begin, m_end, other.m_begin, other.m_end);
    } else {
      return std::lexicographical_compare_three_way(
        m_begin, m_end, other.m_begin, other.m_end, [](const auto& a, const auto& b) {
          return a < b ? std::weak_ordering::less :
                 b < a ? std::weak_ordering::greater :
                         std::weak_ordering::equivalent;
        });
    }
  }

  /////////////////////////////////////////
  // Allocation / deallocation utilities //
  /////////////////////////////////////////

private:
  constexpr //
    void
    allocate(size_type capacity, Allocator& alloc)
  {
    try {
      m_begin = AllocTraitsT::allocate(alloc, capacity);
      m_realend = m_begin + capacity;
    } catch (...) {
      if (capacity > max_size()) {
        throw std::length_error("Tried to allocate too many elements.");
      } else {
        throw;
      }
    }
  }

  constexpr //
    pointer
    allocate_tmp(size_type capacity, Allocator& alloc)
  {
    try {
      return AllocTraitsT::allocate(alloc, capacity, m_begin);
    } catch (...) {
      if (capacity > max_size()) {
        throw std::length_error("Tried to allocate too many elements.");
      } else {
        throw;
      }
    }
  }

  constexpr //
    void
    deallocate() //
    noexcept
  {
    clear();
    if (m_begin)
      AllocTraitsT::deallocate(m_alloc, m_begin, capacity());
  }
};

template<typename T, typename Alloc, typename U>
constexpr //
  typename vector_base<T, Alloc>::size_type
  erase(vector_base<T, Alloc>& c, const U& value)
{
  auto it = std::remove(c.begin(), c.end(), value);
  auto r = std::distance(it, c.end());
  c.erase(it, c.end());
  return r;
}

template<typename T, typename Alloc, typename Pred>
constexpr //
  typename vector_base<T, Alloc>::size_type
  erase_if(vector_base<T, Alloc>& c, Pred pred)
{
  auto it = std::remove_if(c.begin(), c.end(), pred);
  auto r = std::distance(it, c.end());
  c.erase(it, c.end());
  return r;
}

} // namespace constexpr_containers
