#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

#include "simple/container_algorithm.h"

namespace simple {

template<typename ValueT, typename AllocT = std::allocator<ValueT>>
struct vector
{

  using AllocTraitsT = std::allocator_traits<AllocT>;
  using InitListT = std::initializer_list<ValueT>;
  using PtrT = ValueT*;
  using ConstPtrT = const ValueT*;
  using RevPtrT = std::reverse_iterator<ValueT*>;
  using RevConstPtrT = std::reverse_iterator<const ValueT*>;
  using RefT = ValueT&;
  using ConstRefT = const ValueT&;

  using value_type = ValueT;
  using allocator_type = AllocT;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = RefT;
  using const_reference = ConstRefT;
  using pointer = PtrT;
  using const_pointer = ConstPtrT;
  using iterator = PtrT;
  using const_iterator = ConstPtrT;
  using reverse_iterator = RevPtrT;
  using reverse_const_iterator = RevConstPtrT;

  PtrT m_begin;
  PtrT m_end;
  PtrT m_realend;
  [[no_unique_address]] AllocT m_alloc;

  constexpr vector() noexcept(noexcept(AllocT()))
    : m_begin(nullptr)
    , m_end(nullptr)
    , m_realend(nullptr)
  {}

  constexpr vector(const AllocT alloc) noexcept
    : m_begin(nullptr)
    , m_end(nullptr)
    , m_realend(nullptr)
    , m_alloc(alloc)
  {}

  template<typename T>
  constexpr explicit //
    vector(size_t count, const T& value, const AllocT& alloc = AllocT())
    : m_alloc(alloc)
  {
    allocate(count);
    for (auto& elem : make_range(m_begin, m_realend)) {
      AllocTraitsT::construct(m_alloc, std::launder(&elem), value);
    }
    m_end = m_realend;
  }

  constexpr explicit //
    vector(size_t count, const AllocT& alloc = AllocT())
    : m_alloc(alloc)
  {
    allocate(count, m_alloc);
    for (auto& elem : make_range(m_begin, m_realend)) {
      AllocTraitsT::construct(m_alloc, std::launder(&elem));
    }
    m_end = m_realend;
  }

  constexpr //
    vector(const vector& other)
    : m_alloc(AllocTraitsT::select_on_container_copy_construction(other.m_alloc))
  {
    allocate(other.size(), m_alloc);
    uninitialized_copy(other.m_begin, other.m_end, m_begin, m_alloc);
    m_end = m_realend;
  }

  constexpr //
    vector(const vector& other, const AllocT& alloc)
    : m_alloc(alloc)
  {
    allocate(other.size(), m_alloc);
    uninitialized_copy(other.m_begin, other.m_end, m_begin, m_alloc);
    m_end = m_realend;
  }

  constexpr                //
    vector(vector&& other) //
    noexcept
    : m_begin(other.m_begin)
    , m_end(other.m_end)
    , m_realend(other.m_realend)
    , m_alloc(std::move(other.m_alloc))
  {
    other.m_begin = other.m_end = other.m_realend = nullptr;
  }

  constexpr                                     //
    vector(vector&& other, const AllocT& alloc) //
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

  constexpr vector(InitListT il, const AllocT& alloc = AllocT())
    : m_alloc(alloc)
  {
    allocate(il.size());
    m_end = m_begin;
    for (const auto& elem : il) {
      push_back(elem);
    }
  }

  // Basic exception guarantee
  constexpr //
    vector&
    operator=(const vector& other)
  {
    // don't self-assign
    if (this != &other) {
      if constexpr (AllocTraitsT::propagate_on_container_copy_assignment::value) {
        if (!AllocTraitsT::is_always_equal::value and m_alloc != other.m_alloc) {
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
    vector&
    operator=(vector&& other) //
    noexcept(AllocTraitsT::propagate_on_container_move_assignment::value ||
             AllocTraitsT::is_always_equal::value)
  {
    if constexpr (AllocTraitsT::propagate_on_container_move_assignment::value) {
      if (!AllocTraitsT::is_always_equal::value and m_alloc != other.m_alloc) {
        m_alloc = other.m_alloc;
      }
      deallocate();
      m_begin = other.m_begin;
      m_end = other.m_end;
      m_realend = other.m_realend;
      other.m_begin = other.m_end = other.m_realend = nullptr;
    } else {
      if (!AllocTraitsT::is_always_equal::value and m_alloc != other.m_alloc) {
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

  constexpr vector& operator=(InitListT il)
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
    swap(vector& other) //
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

  constexpr ~vector() { deallocate(); }

  [[nodiscard]] constexpr /*****/ AllocT get_allocator() const noexcept { return m_alloc; }

  [[nodiscard]] constexpr /**/ RefT at(size_t i)
  {
    check_range(i);
    return *this[i];
  }
  [[nodiscard]] constexpr ConstRefT at(size_t i) const
  {
    check_range(i);
    return *this[i];
  }

  [[nodiscard]] constexpr //
    RefT
    operator[](size_t i) //
    noexcept
  {
    return *std::launder(m_begin + i);
  }
  [[nodiscard]] constexpr //
    ConstRefT
    operator[](size_t i) //
    const noexcept
  {
    return *std::launder(m_begin + i);
  }

private:
  constexpr void check_range(size_t n) const
  {
    if (n >= size()) {
      // TODO: do fancier formatting when I implement constexpr string (?)
      throw std::out_of_range("Bounds check failed.");
    }
  }

public:
  [[nodiscard]] constexpr /**/ RefT front() /*********/ noexcept { return *m_begin; }
  [[nodiscard]] constexpr ConstRefT front() /***/ const noexcept { return *m_begin; }
  [[nodiscard]] constexpr /**/ RefT back() /**********/ noexcept { return *(m_end - 1); }
  [[nodiscard]] constexpr ConstRefT back() /****/ const noexcept { return *(m_end - 1); }
  [[nodiscard]] constexpr /*******/ PtrT data() /************/ noexcept { return m_begin; }
  [[nodiscard]] constexpr /**/ ConstPtrT data() /******/ const noexcept { return m_begin; }

  [[nodiscard]] constexpr /**/ PtrT begin() /*********/ noexcept { return m_begin; }
  [[nodiscard]] constexpr ConstPtrT begin() /***/ const noexcept { return m_begin; }
  [[nodiscard]] constexpr /**/ PtrT end() /***********/ noexcept { return m_end; }
  [[nodiscard]] constexpr ConstPtrT end() /*****/ const noexcept { return m_end; }
  [[nodiscard]] constexpr ConstPtrT cbegin() /**/ const noexcept { return m_begin; }
  [[nodiscard]] constexpr ConstPtrT cend() /****/ const noexcept { return m_end; }

private:
  template<typename T>
  [[nodiscard]] constexpr static decltype(auto) mk_rev(T x) noexcept
  {
    return std::make_reverse_iterator<T>(x);
  }

public:
  [[nodiscard]] constexpr /**/ RevPtrT rbegin() /*********/ noexcept { return mk_rev(m_begin); }
  [[nodiscard]] constexpr RevConstPtrT rbegin() /***/ const noexcept { return mk_rev(m_begin); }
  [[nodiscard]] constexpr /**/ RevPtrT rend() /***********/ noexcept { return mk_rev(m_end); }
  [[nodiscard]] constexpr RevConstPtrT rend() /*****/ const noexcept { return mk_rev(m_end); }
  [[nodiscard]] constexpr RevConstPtrT crbegin() /**/ const noexcept { return mk_rev(m_begin); }
  [[nodiscard]] constexpr RevConstPtrT crend() /****/ const noexcept { return mk_rev(m_end); }

  [[nodiscard]] constexpr size_t size() /*******/ const noexcept { return m_end - m_begin; }
  [[nodiscard]] constexpr size_t capacity() /***/ const noexcept { return m_realend - m_begin; }
  [[nodiscard]] constexpr bool empty() /********/ const noexcept { return size() == 0; }

  [[nodiscard]] constexpr //
    size_t
    max_size() //
    const
  {
    const size_t diffmax = std::numeric_limits<ptrdiff_t>::max() / sizeof(ValueT);
    const size_t allocmax = AllocTraitsT::max_size(m_alloc);
    return std::min(diffmax, allocmax);
  }

  [[nodiscard]] constexpr //
    bool
    operator==(const vector& other) //
    const noexcept(noexcept(*begin() == *other.begin()))
  {
    return std::equal(begin(), end(), other.begin(), other.end());
  }
  [[nodiscard]] constexpr //
    bool
    operator!=(const vector& other) //
    const noexcept(noexcept(*this == other))
  {
    return !(*this == other);
  }

  constexpr //
    void
    reserve(size_t new_cap)
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
    resize(size_t count)
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
    resize(size_t count, const value_type& value)
  {
    if (count > capacity()) {
      auto tmp = allocate_tmp(count, m_alloc);
      try {
        auto end = uninitialized_move_if_noexcept_launder(m_begin, m_end, tmp, m_alloc);
        for (; end < tmp + count; ++end) {
          AllocTraitsT::construct(m_alloc, end, value);
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
    clear()
  {
    while (!empty()) {
      pop_back();
    }
  }

  // Strong exception guarantee
  template<typename... T>
  constexpr //
    void
    emplace_back(T&&... args)
  {
    if (m_end < m_realend) {
      AllocTraitsT::construct(m_alloc, std::launder(m_end), std::forward<T>(args)...);
      ++m_end;
    }

    // Ensure we've fully prepared a tmp buffer before deallocating m_begin
    auto oldsize = size();
    auto newcap = size() * 2;
    auto tmp = allocate_tmp(newcap, m_alloc);
    try {
      // construct new value into tmp
      AllocTraitsT::construct(m_alloc, tmp + oldsize, std::forward<T>(args)...);
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
  template<typename T>
  constexpr //
    void
    push_back(T&& v)
  {
    emplace_back(std::forward<T>(v));
  }

  constexpr //
    void
    pop_back() //
  {
    AllocTraitsT::destroy(m_alloc, std::launder(m_end - 1));
    --m_end;
  }

  template<typename T>
  friend //
    void
    swap(vector<T>& a, vector<T>& b) //
    noexcept
  {
    a.swap(b);
  }

private:
  constexpr //
    void
    allocate(size_t capacity, AllocT& alloc)
  {
    m_begin = AllocTraitsT::allocate(alloc, capacity);
    m_realend = m_begin + capacity;
  }

  constexpr //
    PtrT
    allocate_tmp(size_t capacity, AllocT& alloc)
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
    deallocate()
  {
    clear();
    if (m_begin)
      AllocTraitsT::deallocate(m_alloc, m_begin, capacity());
  }
};

namespace pmr {

template<typename T>
using vector = ::simple::vector<T, std::pmr::polymorphic_allocator<T>>;

} // namespace pmr

} // namespace simple
