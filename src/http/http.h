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

#include "../connection/conn_packet.h"
#include <map>
#include <string>

using std::string;
using std::map;

namespace http {

enum action_type {
  CLEAR_DATA = 0,
  SYNC_REQUEST = 1, // Reserved
  SESS_ADD = 2,
  SESS_DELETE = 3,
  SESS_UPDATE = 4,
  SESS_WRITE = 5,
  BCK_ADD = 20,
  BCK_DELETE = 21,
  BCK_UPDATE = 22,
};

class http_session {
 public:
  std::string key;
  unsigned int backend_id;
  std::string content;
  unsigned long last_acc;
};

class http_service {
 public:
  int id;
  string name;
  map<string, http_session> sessions;
  bool disabled;
  int session_ttl = 0;
};

class http_action : public conn_packet {
 public:
  http_action() { type = HTTP_SYNC; }
  ~http_action() {}
  action_type action;
  unsigned int service;
  unsigned int listener;
  unsigned int backend;
  http_session session;
  std::string farm_name;
};
} // namespace http
