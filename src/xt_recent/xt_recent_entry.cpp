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

#include "xt_recent_entry.h"
#include <algorithm>
const std::string &xt_recent_entry::get_src_ip() const { return src_ip; }
void xt_recent_entry::set_src_ip(const std::string &src_ip) {
  xt_recent_entry::src_ip = src_ip;
}
int xt_recent_entry::get_ttl() const { return ttl; }
void xt_recent_entry::set_ttl(int ttl) { xt_recent_entry::ttl = ttl; }
unsigned long xt_recent_entry::get_last_seen() const {
  return timestamps[get_oldest_pkt_index() - 1];
}
void xt_recent_entry::set_last_seen(unsigned long last_seen) {
  xt_recent_entry::last_seen = last_seen;
}
int xt_recent_entry::get_oldest_pkt_index() const { return last_seen_index; }
void xt_recent_entry::set_oldest_pkt_index(int oldest_pkt_index) {
  xt_recent_entry::last_seen_index = oldest_pkt_index;
}

std::string xt_recent_entry::to_string() {
  std::sort(timestamps.begin(), timestamps.end());
  last_seen_index = timestamps.size();
  std::string update = " src: " + src_ip + "ttl: " + std::to_string(ttl) +
                       " last seen: " + std::to_string(get_last_seen()) +
                       " old packets: " + std::to_string(last_seen_index);
  for (auto nstamps : timestamps)
    update += ' ' + std::to_string(nstamps);

  return update;
}
