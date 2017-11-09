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

#ifndef BASIC_TEST_H
#define BASIC_TEST_H

#include "../connection/conn_packet.h"
#include "../xt_recent/xt_recent_action.h"
#include "../sync/pkt_parser.h"
#include <memory>
#include <string>
class basic_test {
  std::string server_address = "127.0.0.1";
  int server_port = 8888;

 public:
  ~basic_test();
  basic_test();
  bool test_xt_action_serialization();
  bool test_server_client();
  bool test_client_pool();
  int run_master(int port);
  int run_backup(std::string host, int port);
  void run_test();
};

#endif // BASIC_TEST_H
