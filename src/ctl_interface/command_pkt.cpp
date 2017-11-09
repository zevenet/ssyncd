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

#include "command_pkt.h"
#include "../helpers/utils.h"

command_pkt_parser::command_pkt_parser() {}

conn_packet *command_pkt_parser::deserialize(unsigned char *buffer,
                                             unsigned int buffer_size,
                                             unsigned int *buffer_used) {
  *buffer_used = 0;
  std::unique_ptr<conn_packet> ret(
      base_pkt_parser::deserialize(buffer, buffer_size, buffer_used));
  if (ret != nullptr) {
    command_pkt *pkt = new command_pkt();
    std::string res = "";
    pkt->command = static_cast<command_id>(buffer[(*buffer_used)++]);
    if (pkt->command == WRITE_RECENT_TABLE || // pkt->command == SHOW ||
        pkt->command == CLEAR_RECENT_TABLE || pkt->command == START_HTTP_SYNC ||
        pkt->command == STOP_HTTP_SYNC) { // get command data
      int name_size = buffer[(*buffer_used)++];
      for (int i = 0; i < name_size; i++)
        res += buffer[(*buffer_used)++];
    }
    pkt->command_data = res;
    if (pkt->command == SW_TO_CLIENT ||
        pkt->command == SW_TO_MASTER) { // get server port
      pkt->server_port = buffer[(*buffer_used)++];
      pkt->server_port |= buffer[(*buffer_used)++] << 8;
      pkt->server_port |= buffer[(*buffer_used)++] << 16;
    }
    if (pkt->command == SW_TO_CLIENT) { // get server ip address
      ip_family fam = static_cast<ip_family>(buffer[(*buffer_used)++]);
      switch (fam) {
        case IPV4:pkt->server_address = std::to_string(buffer[(*buffer_used)++]);
          for (int i = 0; i < 3; i++)
            pkt->server_address += '.' + std::to_string(buffer[(*buffer_used)++]);
          break;
        case IPV6:break;
      }
    }
    return pkt;
  }
  return nullptr;
}

bool command_pkt_parser::serialize(conn_packet *pkt, char *out_buffer,
                                   unsigned int *out_size) {
  *out_size = 0;
  if (base_pkt_parser::serialize(pkt, out_buffer, out_size)) {
    command_pkt *to_serialize = (command_pkt *) (pkt);
    out_buffer[(*out_size)++] = static_cast<int>(to_serialize->command) & 0xff;

    if (to_serialize->command ==
        WRITE_RECENT_TABLE || // to_serialize->command == SHOW ||
        to_serialize->command == CLEAR_RECENT_TABLE ||
        to_serialize->command == START_HTTP_SYNC ||
        to_serialize->command == STOP_HTTP_SYNC) {
      out_buffer[(*out_size)++] = to_serialize->command_data.size();
      for (char c : to_serialize->command_data) {
        out_buffer[(*out_size)++] = c;
      }
    }

    if (to_serialize->command == SW_TO_MASTER ||
        to_serialize->command == SW_TO_CLIENT) {
      out_buffer[(*out_size)++] = (to_serialize->server_port) & 0xff;
      out_buffer[(*out_size)++] = (to_serialize->server_port >> 8) & 0xff;
      out_buffer[(*out_size)++] = (to_serialize->server_port >> 16) & 0xff;
    }

    if (to_serialize->command == SW_TO_CLIENT) {
      out_buffer[(*out_size)++] = static_cast<char>(
          utils::get_ip_address_type(to_serialize->server_address));
      for (auto &part :
          utils::get_ip_address_array(to_serialize->server_address))
        out_buffer[(*out_size)++] = part;
    }
    // update packet size info
    out_buffer[3] = static_cast<char>(*out_size & 0xff);
    out_buffer[4] = static_cast<char>(((*out_size) >> 8) & 0xff);
    pkt->pkt_len = static_cast<char>((*out_size));
  }
  return true;
}
