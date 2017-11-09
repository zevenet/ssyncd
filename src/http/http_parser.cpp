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

#include "http_parser.h"
#include "../debug/errors.h"
#include "http.h"

using namespace http;

http_parser::http_parser() {}

http_parser::~http_parser() {}

conn_packet *http_parser::deserialize(unsigned char *buffer,
                                      unsigned int buffer_size,
                                      unsigned int *buffer_used) {
  *buffer_used = 0;
  std::unique_ptr<conn_packet> ret(
      base_pkt_parser::deserialize(buffer, buffer_size, buffer_used));
  if (ret != nullptr && ret->type == HTTP_SYNC) {
    http_action *new_packet = new http_action();
    new_packet->listener = 0;
    new_packet->service = 0;
    new_packet->backend = 0;
    new_packet->session.last_acc = 0;

    new_packet->action = (action_type) buffer[(*buffer_used)++];
    int farm_size = buffer[(*buffer_used)++];
    for (int i = 0; i < farm_size; i++)
      new_packet->farm_name += buffer[(*buffer_used)++];

    if (new_packet->action != http::SYNC_REQUEST) {
      for (int i = 0; i < 4; i++)
        new_packet->listener |= (buffer[(*buffer_used)++] << (8 * i));
      for (int i = 0; i < 4; i++)
        new_packet->service |= (buffer[(*buffer_used)++] << (8 * i));
      for (int i = 0; i < 4; i++)
        new_packet->backend |= (buffer[(*buffer_used)++] << (8 * i));
      new_packet->session.backend_id = new_packet->backend;
      int size = buffer[(*buffer_used)++];
      for (int i = 0; i < size; i++)
        new_packet->session.key += buffer[(*buffer_used)++];

      if (new_packet->action == SESS_ADD || new_packet->action == SESS_UPDATE ||
          new_packet->action == SESS_WRITE) {
        size = buffer[(*buffer_used)++];
        for (int i = 0; i < size; i++)
          new_packet->session.content += buffer[(*buffer_used)++];
        unsigned long timestamp = 0;
        for (int i = 0; i < 8; i++) {
          timestamp |= (((unsigned long) buffer[(*buffer_used)++]) << (8 * i));
        }
        new_packet->session.last_acc = timestamp;
        new_packet->pkt_len = (*buffer_used);
      }
    }
    return new_packet;
  } else
    return nullptr;
}

bool http_parser::serialize(conn_packet *pkt, char *out_buffer,
                            unsigned int *out_size) {
  if (pkt->type != HTTP_SYNC)
    return false;
  if (base_pkt_parser::serialize(pkt, out_buffer, out_size)) {
    http_action *to_serialize = (http_action *) pkt;
    if (*out_size == 5) {
      out_buffer[(*out_size)++] = static_cast<char>(to_serialize->action);
      out_buffer[(*out_size)++] =
          static_cast<char>(to_serialize->farm_name.size());
      for (char d : to_serialize->farm_name)
        out_buffer[(*out_size)++] = d;

      if (to_serialize->action != http::SYNC_REQUEST) {
        for (int i = 0; i < 4; i++)
          out_buffer[(*out_size)++] = (to_serialize->listener << (8 * i));
        for (int i = 0; i < 4; i++)
          out_buffer[(*out_size)++] = (to_serialize->service << (8 * i));
        for (int i = 0; i < 4; i++)
          out_buffer[(*out_size)++] = (to_serialize->backend << (8 * i));

        out_buffer[(*out_size)++] =
            static_cast<char>(to_serialize->session.key.size());

        for (char d : to_serialize->session.key)
          out_buffer[(*out_size)++] = d;

        if (to_serialize->action == SESS_ADD ||
            to_serialize->action == SESS_UPDATE ||
            to_serialize->action == SESS_WRITE) {
          out_buffer[(*out_size)++] =
              static_cast<char>(to_serialize->session.content.size());
          for (char d : to_serialize->session.content)
            out_buffer[(*out_size)++] = d;
          unsigned long tmp = to_serialize->session.last_acc;
          for (int i = 0; i < 8; i++) {
            out_buffer[(*out_size)++] = static_cast<char>(tmp);
            tmp = (tmp >> 8);
          }
        }
        // (*out_size) -= 1;
      }
      out_buffer[3] = static_cast<char>(*out_size & 0xff);
      out_buffer[4] = static_cast<char>(((*out_size) >> 8) & 0xff);
      pkt->pkt_len = static_cast<char>((*out_size));
      return true;
    }
  }
  return false;
}
