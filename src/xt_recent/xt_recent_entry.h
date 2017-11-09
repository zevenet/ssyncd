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

#include "../helpers/utils.h"
#include <string>
#include <vector>

class xt_recent_entry {
  std::string src_ip;
  int ttl;
  unsigned long int last_seen;
  int last_seen_index;

public:
  ip_family ip_family_;
  std::vector<unsigned long int> timestamps;
  const std::string &get_src_ip() const;
  void set_src_ip(const std::string &src_ip);
  int get_ttl() const;
  void set_ttl(int ttl);
  unsigned long get_last_seen() const;
  void set_last_seen(unsigned long last_seen);
  int get_oldest_pkt_index() const;
  void set_oldest_pkt_index(int last_seen_index);
  std::string to_string();
};
