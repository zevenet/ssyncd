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

#include "packet_parser.h"

bool base_pkt_parser::serialize(conn_packet *pkt, char *out_buffer,
                                unsigned int *out_size) {
  // out_buffer = new char[3];
  (*out_size) = 5;
  out_buffer[0] = dynamic_cast<conn_packet *>(pkt)->header_1 & 0xff;
  out_buffer[1] = dynamic_cast<conn_packet *>(pkt)->header_2 & 0xff;
  out_buffer[2] = dynamic_cast<conn_packet *>(pkt)->type;
  out_buffer[3] = dynamic_cast<conn_packet *>(pkt)->pkt_len & 0xff;
  out_buffer[4] = (dynamic_cast<conn_packet *>(pkt)->pkt_len >> 8) & 0xff;
  return true;
}

base_pkt_parser::base_pkt_parser() {}

base_pkt_parser::~base_pkt_parser() {}

conn_packet *base_pkt_parser::deserialize(unsigned char *buffer,
                                          unsigned int buffer_size,
                                          unsigned int *buffer_used) {
  if (buffer_size >= 5) {
    conn_packet *packet = new conn_packet;
    packet->header_1 = buffer[0];
    packet->header_2 = buffer[1];
    packet->type = (packet_type) buffer[2];
    packet->pkt_len = buffer[3];
    packet->pkt_len |= (((int) buffer[4]) << 8);
    if (buffer_size < packet->pkt_len) {
      delete packet;
      return nullptr;
    }
    (*buffer_used) = 5;
    return packet;
  } else {
    return nullptr;
  }
}
