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

#include "conn_packet.h"
class packet_parser {
 public:
  virtual conn_packet *deserialize(unsigned char *buffer,
                                   unsigned int buffer_size,
                                   unsigned int *buffer_used) = 0;
  virtual bool serialize(conn_packet *pkt, char *out_buffer,
                         unsigned int *out_size) = 0;
};

class base_pkt_parser : public packet_parser {
 public:
  virtual conn_packet *deserialize(unsigned char *buffer,
                                   unsigned int buffer_size,
                                   unsigned int *buffer_used);

  virtual bool serialize(conn_packet *pkt, char *out_buffer,
                         unsigned int *out_size);
  base_pkt_parser();
  ~base_pkt_parser();
};
