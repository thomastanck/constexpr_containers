#pragma once

#include <memory>          // for allocator
#include <memory_resource> // for polymorphic_allocator

#include "constexpr_containers/vector_base.h"

namespace constexpr_containers {

template<typename T, typename Allocator = std::allocator<T>>
using vector = vector_base<T, Allocator>;

namespace pmr {

template<typename T>
using vector = ::constexpr_containers::vector_base<T, std::pmr::polymorphic_allocator<T>>;

} // namespace pmr

} // namespace constexpr_containers
