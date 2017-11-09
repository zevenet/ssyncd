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

#include <condition_variable>

#include <mutex>
#include <queue>

template<class T>
class safe_queue {
 public:
  safe_queue(void) : queue_(), mut_(), cond_() {}
  ~safe_queue(void) {}

  void enqueue(T t) {
    std::unique_lock<std::mutex> lock(mut_);
    queue_.push(t);
    lock.unlock();
    cond_.notify_one();
  }

  T dequeue(void) {
    std::unique_lock<std::mutex> lock(mut_);
    while (queue_.empty()) {
      cond_.wait(lock);
    }
    T val = queue_.front();
    queue_.pop();
    return val;
  }
  void pop(T &item) {
    std::unique_lock<std::mutex> lock(mut_);
    while (queue_.empty()) {
      cond_.wait(lock);
    }
    item = queue_.front();
    queue_.pop();
  }
  //only works on pointers
  T try_dequeue(void) {
    std::unique_lock<std::mutex> lock(mut_);
    if (!queue_.empty()) {
      T val = queue_.front();
      queue_.pop();
      return val;
    } else {
      return nullptr;
    }
  }

  int size() { return queue_.size(); }

 private:
  std::queue<T> queue_;
  mutable std::mutex mut_;
  std::condition_variable cond_;
};
