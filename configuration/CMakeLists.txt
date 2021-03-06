
include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_DEFINITIONS "${FRUIT_COMPILE_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

CHECK_CXX_SOURCE_COMPILES("
template <typename T, typename U>
struct Pair {};

struct Map : public Pair<int, float>, Pair<int, char> {};

template <typename Value>
Value f(Pair<int, Value>*) { return Value(); }

int main() {
  f((Map*)0);
}
"
FRUIT_HAS_CLANG_ARBITRARY_OVERLOAD_RESOLUTION_BUG)

CHECK_CXX_SOURCE_COMPILES("
int main() {
  bool b = __has_trivial_copy(int);
  (void) b;
  return 0;
}
"
FRUIT_HAS_HAS_TRIVIAL_COPY)

CHECK_CXX_SOURCE_COMPILES("
int main() {
  bool b = __is_trivially_copyable(int);
  (void) b;
  return 0;
}
"
FRUIT_HAS_IS_TRIVIALLY_COPYABLE)

CHECK_CXX_SOURCE_COMPILES("
#include <cstddef>
using X = max_align_t;
int main() {
  return 0;
}
"
FRUIT_HAS_MAX_ALIGN_T)

CHECK_CXX_SOURCE_COMPILES("
#include <type_traits>
int main() {
  bool b = std::is_trivially_copyable<int>::value;
  (void) b;
  return 0;
}
"
FRUIT_HAS_STD_IS_TRIVIALLY_COPYABLE)

CHECK_CXX_SOURCE_COMPILES("
#include <cstddef>
using X = std::max_align_t;
int main() {
  return 0;
}
"
FRUIT_HAS_STD_MAX_ALIGN_T)

CHECK_CXX_SOURCE_COMPILES("
#include <typeinfo>
int main() {
  (void) typeid(int);
  return 0;
}
"
FRUIT_HAS_TYPEID)

CHECK_CXX_SOURCE_COMPILES("
#include <cxxabi.h>
int main() {
  auto* p = abi::__cxa_demangle;
  (void) p;
  return 0;
}
"
FRUIT_HAS_CXA_DEMANGLE)

if (NOT "${FRUIT_HAS_STD_MAX_ALIGN_T}" AND NOT "${FRUIT_HAS_MAX_ALIGN_T}")
  message(WARNING "The current C++ standard library doesn't support std::max_align_t nor ::max_align_t. Attempting to use std::max_align_t anyway, but it most likely won't work.")
endif()

if(NOT "${FRUIT_HAS_STD_IS_TRIVIALLY_COPYABLE}" AND NOT "${FRUIT_HAS_IS_TRIVIALLY_COPYABLE}"
   AND NOT "${FRUIT_HAS_HAS_TRIVIAL_COPY}")
  message(WARNING "The current standard library doesn't support std::is_trivially_copyable<T>, and the current compiler doesn't support __is_trivially_copyable(T) nor __has_trivial_copy(T). Attemping to use std::is_trivially_copyable<T> anyway, but it most likely won't work.")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fruit-config-base.h.in ${CMAKE_CURRENT_BINARY_DIR}/../include/fruit/impl/fruit-config-base.h)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/fruit/impl/fruit-config-base.h
  DESTINATION ${INSTALL_INCLUDE_DIR}/impl)
