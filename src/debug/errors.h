/*
 *    Zevenet Software License
 *    This file is part of the Zevenet Load Balancer software package.
 *
 *    Copyright (C) 2017-today ZEVENET SL, Sevilla (Spain)
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

static std::mutex mtx_cout;

// Asynchronous output
struct stdcout {
  std::unique_lock<std::mutex> lk;
  stdcout() : lk(std::unique_lock<std::mutex>(mtx_cout)) {}

  template<typename T>
  stdcout &operator<<(const T &_t) {
    std::cout << _t;
    return *this;
  }

  stdcout &operator<<(std::ostream &(*fp)(std::ostream &)) {
    std::cout << fp;
    return *this;
  }
};
#ifndef OUT_ERROR
#define OUT_ERROR                                                              \
  stdcout() << "[" << __PRETTY_FUNCTION__ << ":" << __LINE__ << "] "
#endif

enum recent_sync_errors {
  ERROR_NOT_DEFINED = -2,
};

#ifndef DEBUG
#define DEBUG 0 // set debug mode
#endif

#if DEBUG
#define DEBUG_LOG                                                              \
  stdcout() << "[" << __FILENAME__ << "][Line " << __LINE__ << "]: "
#define DEBUG_INFO stdcout() << "[" << __FUNCTION__ << "]: "

#else
#define DEBUG_LOG 0 && std::clog
#define DEBUG_INFO std::cout
#endif

#ifndef SYSLOG_ON
#define SYSLOG_ON 0

#include <syslog.h>

static void to_syslog(int type, const char *data) {
#ifdef SYSLOG_ON
  syslog(type, "%s", data);
#endif
}
#endif

