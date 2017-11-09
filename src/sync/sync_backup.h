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
#include "../xt_recent/xt_recent_proc.h"
#include "base_sync.h"

class sync_backup : public base_sync {
#if __cplusplus >= 201703L
  std::function<void()> on_master_close = []() {
    OUT_ERROR << "on_master_close not defined" << std::endl;
  };

public:
  void handle_on_master_disconect(std::function<void()> callback);
#endif
 public:
  sync_backup();
  ~sync_backup();
  void run() override;
};
