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

#include "base_sync.h"

class sync_master : public base_sync {
  safe_queue<xt_recent_action> recent_pending_actions;
  safe_queue<http::http_action> pound_pending_actions;
  int backup_servers_fd = 0;
  bool handle_recent_sync(int fd);
  bool handle_http_sync(int fd);
  void run() override;

 public:
  sync_master();
  ~sync_master();
};
