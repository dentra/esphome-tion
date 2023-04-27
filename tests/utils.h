#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <stdio.h>
#include <stdarg.h>  // va_list

#include "cloak.h"


uint8_t fast_random_8();

// minimal C++11 allocator with debug output
template<class Tp> struct NAlloc {
  typedef Tp value_type;
  NAlloc() = default;
  template<class T> NAlloc(const NAlloc<T> &) {}

  Tp *allocate(std::size_t n) {
    n *= sizeof(Tp);
    Tp *p = static_cast<Tp *>(::operator new(n));
    std::cout << "allocating " << n << " bytes @ " << p << std::endl;
    return p;
  }

  void deallocate(Tp *p, std::size_t n) {
    std::cout << "deallocating " << n * sizeof *p << " bytes @ " << p << std::endl;
    ::operator delete(p);
  }
};
template<class T, class U> bool operator==(const NAlloc<T> &, const NAlloc<U> &) { return true; }
template<class T, class U> bool operator!=(const NAlloc<T> &, const NAlloc<U> &) { return false; }
