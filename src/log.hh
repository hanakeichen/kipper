#pragma once

#ifndef NDEBUG

#include <iostream>
#include "message.hh"

#define LOG_DEBUG(format, ...) \
  std::cout << Message::Format(format, ##__VA_ARGS__) << "\n"

#else  // NDEBUG
#define LOG_DEBUG(format, ...) ((void)0)
#endif  // !NDEBUG
