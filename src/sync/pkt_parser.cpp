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

#include "pkt_parser.h"
#include <memory>
packet_parser &pkt_parser::get_packet_parser(packet_type type) {
  switch (type) {
    case XT_RECENT_SYNC:return action_parser;
    case XT_RECENT_TABLE:return table_parser;
    case HTTP_SYNC:return pound_parser_;
    case GENERIC:
    case CONNECT:
    case DISCONNECT:
    case SYNC_REQUEST:return base_parser;
    default:return base_parser;
  }
}

pkt_parser::pkt_parser() {}

pkt_parser::~pkt_parser() {}

conn_packet *pkt_parser::deserialize(unsigned char *buffer,
                                     unsigned int buffer_size,
                                     unsigned int *buffer_used) {
  std::unique_ptr<conn_packet> ret(
      base_pkt_parser::deserialize(buffer, buffer_size, buffer_used));
  if (ret != nullptr) {
    return get_packet_parser(ret.get()->type)
        .deserialize(buffer, buffer_size, buffer_used);
  } else
    return nullptr;
}

bool pkt_parser::serialize(conn_packet *pkt, char *out_buffer,
                           unsigned int *out_size) {
  if (base_pkt_parser::serialize(pkt, out_buffer, out_size)) {
    return get_packet_parser(pkt->type).serialize(pkt, out_buffer, out_size);
  }
  return false;
}
