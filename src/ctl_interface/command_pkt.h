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
#include "../connection/packet_parser.h"
#include <string>

enum command_id {
  VOID = 1,

  START_HTTP_SYNC,
  STOP_HTTP_SYNC,
  SHOW_HTTP,
  CLEAR_HTTP,
  WRITE_HTTP,

  STOP_RECENT_SYNC,
  START_RECENT_SYNC,

  SHOW_RECENT,
  CLEAR_RECENT,
  CLEAR_RECENT_TABLE,
  WRITE_RECENT_TABLE, // require argument  table_name
  WRITE_RECENT,

  SW_TO_MASTER, // require argument  port
  SW_TO_CLIENT, // require arguments server ip and port

  COUNT_RECENT,
  SHOW_MODE,
  QUIT,

  COUNT_HTTP,

  CTL_ERROR,
  STR_MESSAGE,
  RSYNC,
};
class command_pkt : public conn_packet {
 public:
  command_id command;
  std::string command_data = "";
  int server_port;
  std::string server_address;
  command_pkt() { type = CTL_COMMAND; }
  ~command_pkt() {}
};

class command_pkt_parser : public base_pkt_parser {
 public:
  conn_packet *deserialize(unsigned char *buffer, unsigned int buffer_size,
                           unsigned int *buffer_used) override;
  bool serialize(conn_packet *pkt, char *out_buffer,
                 unsigned int *out_size) override;
  command_pkt_parser();
  virtual ~command_pkt_parser() {};
};
