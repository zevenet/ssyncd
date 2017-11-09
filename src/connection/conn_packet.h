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
enum packet_type {
  CONNECT = 1,
  DISCONNECT = 2,
  GENERIC = 3, // header + size(2 bytes) + array
  XT_RECENT_SYNC = 4,
  XT_RECENT_TABLE = 5,
  HTTP_SYNC = 6,
  CTL_COMMAND = 7,
  SYNC_REQUEST = 8
};

class conn_packet {
 public:
  conn_packet() {}
  conn_packet(packet_type type) { this->type = type; }
  virtual ~conn_packet() {}
  int fd = -1; // default value -1, while sending means broadcast to all
  packet_type type = CONNECT;
  int header_1 = 0xef;
  int header_2 = 0xab;
  unsigned int pkt_len = 0;
};
