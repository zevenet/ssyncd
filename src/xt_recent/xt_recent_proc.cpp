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

#include "xt_recent_proc.h"
#include "../debug/errors.h"
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <re2/re2.h>

using std::string;

std::string xt_recent_proc::parse_data(const std::string str,
                                       const std::string &reg_pattern) {
  std::string res;
  if (RE2::PartialMatch(str, reg_pattern, &res))
    return res;
  else
    return std::string();
}

xt_recent_entry xt_recent_proc::parse_file_row(const std::string str_row) {
  string s_src = parse_data(str_row, regs_patterns[0]);
  string s_ttl = parse_data(str_row, regs_patterns[1]);
  string s_last_seen = parse_data(str_row, regs_patterns[2]);
  string s_oldest_packet_index = parse_data(str_row, regs_patterns[3]);
  string s_timestamps = parse_data(str_row, regs_patterns[4]);
  xt_recent_entry entry;
  entry.set_src_ip(s_src);
  entry.set_oldest_pkt_index(std::stoi(s_oldest_packet_index));
  entry.set_last_seen(std::stoul(s_last_seen));
  entry.set_ttl(std::stoi(s_ttl));
  entry.ip_family_ = ip_family::IPV4;
  char *str = (char *)s_timestamps.c_str();
  char *token, *strpos = str;
  while ((token = strsep(&strpos, ",")) != NULL) {
    unsigned long tmp = std::stoul(token);
    if (tmp != 0)
      entry.timestamps.push_back(tmp);
  }
  // std::sort(entry.timestamps.begin(), entry.timestamps.end());
  return entry;
}

std::vector<xt_recent_table> xt_recent_proc::get_xt_recent_tables() {
  std::vector<xt_recent_table> tables;
  std::shared_ptr<DIR> directory_ptr(
      opendir(xt_recent_proc::xt_recent_dir.c_str()),
      [](DIR *dir) { dir &&closedir(dir); });
  struct dirent *dirent_ptr;
  if (!directory_ptr) {
    OUT_ERROR << "Error opening : " << std::strerror(errno)
              << xt_recent_proc::xt_recent_dir << std::endl;
    to_syslog(LOG_EMERG, "Error opening /proc/net/xt_recent/ directory:");
    exit(EXIT_FAILURE);
    // return tables;
  }

  while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr) {
    if (dirent_ptr->d_type == DT_REG) {
      tables.push_back(get_table(std::string(
          dirent_ptr->d_name))); // TODO: use relative table name path
      //      DEBUG_INFO << "Found xt recent table: " << dirent_ptr->d_name
      //                 << std::endl;
    }
  }
  return tables;
}

xt_recent_table xt_recent_proc::get_table(const std::string table_name) {
  xt_recent_table table;
  table.name = std::string(table_name);
  std::ifstream infile(xt_recent_dir + table_name);
  std::string line;
  while (std::getline(infile, line)) {
    xt_recent_entry new_entry = parse_file_row(line);
    if (!new_entry.get_src_ip().empty()) {
      table.entries[new_entry.get_src_ip()] = new_entry;
    }
  }
  return table;
}

std::vector<xt_recent_entry>
xt_recent_proc::get_table_entries(const std::string table_name) {
  std::vector<xt_recent_entry> entries;
  std::ifstream infile(table_name);
  std::string line;
  while (std::getline(infile, line)) {
    xt_recent_entry new_entry = parse_file_row(line);
    if (!new_entry.get_src_ip().empty()) {
      entries.push_back(new_entry);
    }
  }
  return entries;
}
