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

#include <string>
#include "../connection/conn_packet.h"
#include "xt_recent_entry.h"
enum action_type {
  ADD_UPD_ENTRY = 1,
  DELETE_ENTRY = 2,
  FLUSH_TABLE = 3,
  DELETE_TABLE = 4,
};

class xt_recent_action : public conn_packet {
 public:
  xt_recent_action();
  xt_recent_action(action_type type);
  xt_recent_action(conn_packet *pkt);
  ~xt_recent_action() {}
  unsigned long action_timestamp;
  xt_recent_entry target_entry;
  action_type action;
  std::string xt_table_name;
};
