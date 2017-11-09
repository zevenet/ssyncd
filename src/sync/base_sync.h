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
#include "../connection/connection.h"
#include "../debug/errors.h"
#include "../http/http.h"
#include "../http/http_sync.h"
#include "../xt_recent/xt_recent_action.h"
#include "../xt_recent/xt_recent_sync.h"
#include "../xt_recent/xt_recent_table.h"
#include "pkt_parser.h"
#include <atomic>
#include <map>
#include <memory>

using std::string;
using std::map;

class base_sync {
 protected:
  connection *conn;
  http_sync http_listener;
  xt_recent_sync recent_listener;

  std::map<string, xt_recent_table> recent_data;
  std::map<string,
           std::map<unsigned int, std::map<unsigned int, http::http_service>>>
      http_session_data; // <listener,<service_id, service_object>>

  pkt_parser *parser;
  bool apply_recent_action(xt_recent_action *xaction);
  bool apply_http_action(http::http_action *action);

  static void run_task(base_sync &);
  virtual void run();

 public:
  base_sync();
  virtual ~base_sync();
  virtual void init_master(int port);
  virtual void init_backup(std::string address, int port);
  virtual void start_recent_sync();
  virtual void stop_recent_sync();
  bool write_recent_all();
  int write_recent_data(std::string table_name);
  int clear_recent_data(std::string table_name);
  void clear_recent_data();
  bool start_http_sync(bool clear_data);
  bool is_http_sync_running();
  bool is_recent_sync_running();
  void stop_http_sync();
  bool write_http_data();
  void clear_http_data();
  void clear_http_farm_data(string farm_name);
  void start();
  void stop();
  bool add_http_listener_farm(std::string farm_name);
  bool remove_http_listener_farm(std::string farm_name);
  std::atomic<bool> is_running;
  std::map<string, xt_recent_table> get_recent_data() const;
  std::map<string,
           std::map<unsigned int, std::map<unsigned int, http::http_service>>>
  get_http_data() const;
};
