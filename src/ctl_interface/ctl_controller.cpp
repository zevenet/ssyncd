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

#include "ctl_controller.h"
#include "../connection/connection.h"
#include "../debug/errors.h"
#include <cstring>
#include <sys/un.h>
#define XT_RECENT_APP_PATH "/tmp/ssyncd_path"

ctl_controller::ctl_controller() {}

void ctl_controller::init() {
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    std::string err_ = "socket() failed:  ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
  }
  struct sockaddr_un serveraddr;
  strcpy(serveraddr.sun_path, XT_RECENT_APP_PATH);
  serveraddr.sun_family = AF_UNIX;
  int rc = connect(fd, (struct sockaddr *) &serveraddr, SUN_LEN(&serveraddr));
  if (rc < 0) {
    OUT_ERROR << "ssyncd daemon not running\n";
    std::string err_ = "connect() failed: ";
    err_ += std::strerror(errno);
    DEBUG_LOG << err_ << std::endl;
    exit(ERROR_NOT_DEFINED);
  }
  is_running = true;
}

bool ctl_controller::send_command(command_pkt *pkt) {
  int sent = 0;
  int count;
  char out_buffer[1024];
  unsigned int buffer_size = 0;
  command_pkt_parser parser;
  if (parser.serialize(pkt, out_buffer, &buffer_size)) {
    while (sent < buffer_size) {
      count = send(fd, out_buffer + sent, buffer_size - sent, 0);
      if (count < 0) {
        std::string err_ = "send() failed:  ";
        err_ += std::strerror(errno);
        OUT_ERROR << err_ << std::endl;
        return false;
      }
      sent += count;
    }
  }
  return false;
}

std::string ctl_controller::get_response() {
  int rc = 0;
  char buffer[MAX_PACKET_SIZE];
  rc = recv(fd, &buffer, MAX_PACKET_SIZE, 0);
  if (rc < 0) {
    OUT_ERROR << std::endl
              << "Error receiving response from daemon :"
              << std::strerror(errno) << std::endl;
  } else if (rc == 0) {
    is_running = false;
    return "";
  }

  string res = buffer;
  return res;
}
