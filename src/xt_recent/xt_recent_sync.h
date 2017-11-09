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
#include "xt_recent_action.h"
#include "xt_recent_proc.h"
#include "xt_recent_table.h"
#include <map>
#include <string>
#include <thread>
#include <vector>

class xt_recent_sync {
  std::map<std::string, xt_recent_table> recent_data;
  void fake_update(int entries = 10);
  xt_recent_table get_table_updates(const std::string table_name);
  static void proc_task(xt_recent_sync &);
  int update_time_sec = 1000;
  std::thread update_thread;

public:
  void set_update_time(int milliseconds);
  safe_queue<xt_recent_action *> input_data;
  volatile bool is_running;
  void start();
  void stop();
  static xt_recent_action *get_entry_as_action(std::string target_table,
                                               xt_recent_entry *entry,
                                               bool delete_action = false);
};
