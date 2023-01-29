#pragma once
#include <utility>
// first==width, second==height
typedef std::pair<int, int> dims;       

template<typename T> using range = std::pair<T, T>;