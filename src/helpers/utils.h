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
#include <memory.h>
#include <string>
#include <thread>
#include <vector>

enum ip_family {
  IPV4 = 1,
  IPV6 = 2,
};

namespace utils {
static std::vector<char> get_ip_address_v6(std::string ip_str) {
  return std::vector<char>();
}

static std::vector<char> get_ip_address_v4(std::string ip_str) {
  char str[ip_str.size() + 1]; // = (char *)ip_str.c_str();
  strcpy(str, ip_str.c_str());
  char *token, *strpos = str;
  std::vector<char> res;
  while ((token = strsep(&strpos, ".")) != NULL) {
    int tmp = std::stoi(token);
    char to_add = static_cast<char>(tmp);
    res.push_back(to_add);
  }
  return res;
}
static ip_family get_ip_address_type(std::string ip_str) {
  if (ip_str.find(':') == std::string::npos) {
    return IPV4;
  } else {
    return IPV6;
  }
}

static std::vector<char> get_ip_address_array(std::string ip_str) {
  std::vector<char> ret;
  if (ip_str.find(':') != std::string::npos) {
    return get_ip_address_v6(ip_str);

  } else if (ip_str.find('.') != std::string::npos) {
    return get_ip_address_v4(ip_str);
  }
  return ret;
}

} // namespace utils
