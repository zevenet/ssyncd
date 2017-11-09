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
#include "../helpers/safe_queue.h"
#include "http.h"
#include "http_parser.h"
#include <atomic>
#include <mutex>
#include <thread>
//#define SYNC_SOCKET_PATH "/tmp/pound_sync_path"

#define HTTP_EPOLL_TIMEOUT_MS 1000
#define HTTP_MAX_PACKET_SIZE (65350 * 100)
#define HTTP_MAX_CLIENTS 1000

class http_sync {
  struct connection_event_data {
    int fd;
    unsigned char buffer[HTTP_MAX_PACKET_SIZE];
    unsigned int buffer_size;
  };
  // int listen_fd;
  std::map<int, string> farms_fd;
  std::atomic<int> epoll_fd;
  std::mutex send_mutex;
  std::thread listen_thread;
  static void listen_task(http_sync &);
  http_parser parser;
  void peer_disconnect_handler(int fd);
  int get_farm_fd(string farm_name);

 public:
  std::atomic<bool> is_listener;
  http_sync(bool enable_listen_mode = false);
  ~http_sync();
  bool send_pound_action(std::string farm_name, http::http_action *action);
  bool init();
  bool start();
  void stop();
  // bool is_connected;

 public:
  safe_queue<http::http_action *> input_data;
  std::atomic<bool> is_running;
  bool add_farm(string farm_name);
  bool remove_farm(string farm_name);
};
