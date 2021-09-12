#pragma once

#include <memory>          // for allocator
#include <memory_resource> // for polymorphic_allocator

#include "simple/vector_base.h"

namespace simple {

template<typename T, typename Allocator = std::allocator<T>>
using vector = vector_base<T, Allocator>;

namespace pmr {

template<typename T>
using vector = ::simple::vector_base<T, std::pmr::polymorphic_allocator<T>>;

} // namespace pmr

} // namespace simple
