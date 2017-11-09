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

#include "../http/http_parser.h"
#include "../xt_recent/xt_recent_parser.h"

class pkt_parser : public base_pkt_parser {
  xt_recent_action_parser action_parser;
  xt_recent_table_parser table_parser;
  base_pkt_parser base_parser;
  http_parser pound_parser_;
  packet_parser &get_packet_parser(packet_type type);

 public:
  pkt_parser();
  virtual ~pkt_parser();

  conn_packet *deserialize(unsigned char *buffer, unsigned int buffer_size,
                           unsigned int *buffer_used) override;
  bool serialize(conn_packet *pkt, char *out_buffer,
                 unsigned int *out_size) override;
};
