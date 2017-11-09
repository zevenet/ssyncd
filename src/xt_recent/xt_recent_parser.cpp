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

#include "xt_recent_parser.h"
#include "../helpers/utils.h"
#include "xt_recent_action.h"
#include <cstring>
#include <memory>

conn_packet *xt_recent_table_parser::deserialize(unsigned char *buffer,
                                                 unsigned int buffer_size,
                                                 unsigned int *buffer_used) {
  return nullptr;
}

bool xt_recent_table_parser::serialize(conn_packet *pkt, char *out_buffer,
                                       unsigned int *out_size) {
  return false;
}

conn_packet *xt_recent_action_parser::deserialize(unsigned char *buffer,
                                                  unsigned int buffer_size,
                                                  unsigned int *buffer_used) {
  *buffer_used = 0;
  std::unique_ptr<conn_packet> ret(
      base_pkt_parser::deserialize(buffer, buffer_size, buffer_used));
  if (ret != nullptr && ret.get()->type == XT_RECENT_SYNC) {
    xt_recent_action *new_packet = new xt_recent_action(ret.get());

    new_packet->action = (action_type)buffer[(*buffer_used)++];
    if (new_packet->action == ADD_UPD_ENTRY ||
        new_packet->action == DELETE_ENTRY) {
      // new_packet->target_entry ;//= xt_recent_entry();
      new_packet->target_entry.ip_family_ = (ip_family)buffer[(*buffer_used)++];
      std::string src_ip = std::to_string(buffer[(*buffer_used)++]);
      for (int i = 0; i < 3; i++)
        src_ip += '.' + std::to_string(buffer[(*buffer_used)++]);
      new_packet->target_entry.set_src_ip(src_ip);
    }
    if (new_packet->action == ADD_UPD_ENTRY) {
      new_packet->target_entry.set_oldest_pkt_index(buffer[(*buffer_used)++]);
      new_packet->target_entry.set_ttl(buffer[(*buffer_used)++]);
      int tmstmp_count = buffer[(*buffer_used)++];
      for (int i = 0; i < tmstmp_count; i++) {
        unsigned long timestamp = 0;
        for (int i = 0; i < 8; i++) {
          timestamp |= (((unsigned long)buffer[(*buffer_used)++]) << (8 * i));
        }
        new_packet->target_entry.timestamps.push_back(timestamp);
      }
    }
    int len = buffer[*buffer_used];
    for (int i = 1; i < len; i++) {
      (*buffer_used)++;
      new_packet->xt_table_name += buffer[*buffer_used];
    }
    (*buffer_used)++;
    return new_packet;

  } else
    return nullptr;
}

bool xt_recent_action_parser::serialize(conn_packet *pkt, char *out_buffer,
                                        unsigned int *out_size) {
  if (base_pkt_parser::serialize(pkt, out_buffer, out_size)) {
    xt_recent_action *to_parser = (xt_recent_action *)pkt;
    if (*out_size == 5) {
      out_buffer[(*out_size)++] =
          dynamic_cast<xt_recent_action *>(pkt)->action & 0xff;
      if (to_parser->action == ADD_UPD_ENTRY ||
          to_parser->action == DELETE_ENTRY) {
        out_buffer[(*out_size)++] = to_parser->target_entry.ip_family_ & 0xff;
        for (auto &part :
             utils::get_ip_address_v4(to_parser->target_entry.get_src_ip()))
          out_buffer[(*out_size)++] = part;
      }
      if (to_parser->action == ADD_UPD_ENTRY) {
        out_buffer[(*out_size)++] =
            to_parser->target_entry.get_oldest_pkt_index() & 0xff;
        out_buffer[(*out_size)++] = to_parser->target_entry.get_ttl() & 0xff;
        out_buffer[(*out_size)++] =
            to_parser->target_entry.timestamps.size() & 0xff;
        for (auto &nstamp : to_parser->target_entry.timestamps) {
          unsigned long tmp = nstamp;
          for (int i = 0; i < 8; i++) {
            out_buffer[(*out_size)++] = static_cast<char>(tmp) & 0xff;
            tmp = (tmp >> 8);
          }
        }
      }
      int name_size = to_parser->xt_table_name.size() + 1;
      out_buffer[(*out_size)++] = static_cast<char>(name_size);

      auto tab2 = std::unique_ptr<char[]>(new char[name_size]);
      std::strcpy(tab2.get(), to_parser->xt_table_name.c_str());
      for (int i = 0; i < name_size; i++) {
        out_buffer[(*out_size)++] = tab2[i];
      }

      (*out_size) -= 1;
      out_buffer[3] = static_cast<char>(*out_size & 0xff);
      out_buffer[4] = static_cast<char>(((*out_size) >> 8) & 0xff);
      pkt->pkt_len = static_cast<char>((*out_size));
      return true;
    }
  }
  return false;
}
