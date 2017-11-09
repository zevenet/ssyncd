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
#include "../helpers/safe_queue.h"
#include "conn_packet.h"
#include "packet_parser.h"
#include <atomic>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <vector>

#define MAX_CLIENTS 10
#define MAX_PACKET_SIZE (65555 * 100)
#define MIN_PACKET_SIZE 5

using std::string;
using std::thread;
using std::vector;
using std::map;
using std::mutex;

struct connection_event_data {
  int fd;
  unsigned char buffer[MAX_PACKET_SIZE];
  unsigned int buffer_size;
};

class connection {
  mutex send_mutex;
  int listen_fd;
  static int set_socket_options(int fd);
  static void receive_task(connection &);
  static void send_task(connection &);
  static bool check_socket(int fd);
  packet_parser *data_parser;
  struct sockaddr_in server_addr;

 public:
  void set_parser(packet_parser *parser) { data_parser = parser; }
  conn_packet *get_packet(bool wait = true);
  // void close_connection();
 public:
  bool is_server;
  map<int, connection_event_data *> connected_nodes;
  virtual bool handle_send(int fd, char *data, int data_size);
  bool send_packet(conn_packet *pkt);
  std::atomic<bool> is_running;
  safe_queue<conn_packet *> input_packet_queue;
  connection(int port);
  connection(string s_host, int port);
  virtual ~connection();
  void start();
  void stop();

 private:
  thread receive_thread;
  thread send_thread;
};
