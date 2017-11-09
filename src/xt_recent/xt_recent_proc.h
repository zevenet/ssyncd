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
#include <thread>
#include "../helpers/safe_queue.h"
#include "xt_recent_action.h"
#include "xt_recent_table.h"

using std::thread;

static const std::string regs_patterns[6] = {
    /*src_pattern =*/"src=(\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3})",
    /*ttl_pattern =*/".*ttl: (\\d+)",
    /*last_seen_pattern=*/".*last_seen: (\\d+)",
    /*oldest_pkt_pattern =*/".*oldest_pkt: (\\d+)",
    /*timestamps_pattern =*/".*oldest_pkt: \\d+ (.*)"
    ///*hitcount_pattern =*/ "(?:hitcount\:\ )(?P<hitcount>[0-9]+)",
};
class xt_recent_proc {
  static std::string parse_data(const std::string str,
                                const std::string &reg_pattern);
  static xt_recent_entry parse_file_row(const std::string str_row);

 public:
  static std::vector<xt_recent_table> get_xt_recent_tables();
  static std::vector<xt_recent_entry> get_table_entries(
      const std::string table_name);
  static xt_recent_table get_table(const std::string table_name);

 public:
  static std::string xt_recent_dir;
};
