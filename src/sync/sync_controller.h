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

#include "../ctl_interface/command_pkt.h"
#include "base_sync.h"

namespace controller {
enum worker_mode { NONE = 0, MASTER, BACKUP };
} // namespace controller

class sync_controller {
  int listen_fd;
  controller::worker_mode current_mode;
  base_sync *sync_worker;
  thread command_thread;

 public:
  int server_port = 9999;
  string server_address = "localhost";
  command_pkt_parser *command_parser;
  //  static int server_port;
  //  static std::string server_address;
  bool is_running;
  sync_controller();
  ~sync_controller();
  static void ctl_task(sync_controller &);
  void init_ctl_interface();
  void send_response(int fd, std::string str);
  void set_worker_mode(controller::worker_mode mode);
  void start();
  void stop();
};
