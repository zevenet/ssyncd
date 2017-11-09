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

#include "basic_test.h"
#include "../connection/connection.h"
#include "../debug/errors.h"

using namespace std;
#define server_address "127.0.0.1"
#define server_port 8888

basic_test::~basic_test() {}

basic_test::basic_test() {}

bool basic_test::test_xt_action_serialization() {
  try {
    pkt_parser *parser = new pkt_parser();
    xt_recent_entry entry;
    entry.set_src_ip("192.168.0.159");
    entry.set_ttl(66);
    entry.ip_family_ = IPV4;
    for (int i = 1; i <= 32; i++) {
      entry.timestamps.push_back(4700000000 + (i));
    }
    entry.timestamps[0] = 4700000000 + (33);
    entry.timestamps[1] = 4700000000 + (34);
    entry.set_oldest_pkt_index(2);
    entry.set_last_seen(entry.timestamps[entry.get_oldest_pkt_index() - 1]);
    xt_recent_action *xaction = new xt_recent_action(ADD_UPD_ENTRY);
    xaction->xt_table_name = "abdess";
    xaction->target_entry = entry;

    char *temp = new char[65553];
    unsigned int size;
    auto ret = parser->serialize(xaction, temp, &size);
    unsigned int buffer_used;
    auto received =
        parser->deserialize((unsigned char *) temp, size, &buffer_used);
    if (received && buffer_used == size)
      return true;
  } catch (std::exception ex) {
    return false;
  }
  return true;
}

bool basic_test::test_server_client() {
  std::thread master_thread = std::thread([]() {
    pkt_parser *parser = new pkt_parser();
    connection master(server_port);
    master.set_parser(parser);
    master.start();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    while (master.is_running) {
      DEBUG_INFO << std::this_thread::get_id() << " waiting ..... " << endl;
      auto pkt = master.get_packet();
      if (pkt->type == CONNECT)
        continue;
      DEBUG_INFO << std::this_thread::get_id() << " <<<<< IN " << pkt->pkt_len
                 << endl;
      pkt->pkt_len++;
      master.send_packet(pkt);
      DEBUG_INFO << std::this_thread::get_id()
                 << " >>>>>>  OUT :" << pkt->pkt_len << endl;
      DEBUG_INFO << std::this_thread::get_id() << " queue size: " << std::dec
                 << master.input_packet_queue.size() << endl;
      delete pkt;
    }

  });

  std::vector<std::thread> clients;
  for (int i = 0; i < 2; i++) {
    clients.push_back(std::thread([]() {
      pkt_parser *parser = new pkt_parser();

      int iteraciones = 0;

      xt_recent_entry entry;
      entry.set_src_ip("192.168.0.159");
      entry.set_ttl(66);
      entry.ip_family_ = IPV4;
      for (int i = 1; i <= 32; i++) {
        entry.timestamps.push_back(4700000000 + (i * 1000));
      }
      entry.timestamps[0] = 4700000000 + (33 * 1000);
      entry.timestamps[1] = 4700000000 + (34 * 1000);
      entry.set_oldest_pkt_index(2);

      xt_recent_action *xaction = new xt_recent_action(ADD_UPD_ENTRY);
      xaction->xt_table_name = "abdess";
      xaction->target_entry = entry;
      xaction->pkt_len = 1;

      connection backup(server_address, server_port);
      backup.set_parser(parser);
      backup.start();
      std::this_thread::sleep_for(std::chrono::seconds(5));
      if (!backup.send_packet(xaction))
        backup.is_running = false;
      DEBUG_INFO << std::this_thread::get_id()
                 << " >>>>>>  OUT :" << xaction->pkt_len << endl;
      iteraciones = 1;
      while (iteraciones < 1000 && backup.is_running) {
        DEBUG_INFO << std::this_thread::get_id() << " waiting ..... "
                   << iteraciones << endl;

        // auto pkt = backup.get_packet();
        // if (pkt->type == CONNECT)
        //   continue;
        // DEBUG_INFO << std::this_thread::get_id() << " <<<<< IN " <<
        // iteraciones
        //           << endl;

        DEBUG_INFO << std::this_thread::get_id()
                   << " >>>>>>  OUT :" << xaction->pkt_len
                   << "IT:" << iteraciones << endl;
        //        DEBUG_INFO << std::this_thread::get_id() << "queue size: " <<
        //        std::dec
        //                   << backup.input_packet_queue.size() << endl;
        xaction->pkt_len++;
        iteraciones++;
        backup.send_packet(xaction);

        // delete pkt;
      }
      DEBUG_INFO << std::this_thread::get_id() << " Closing backup" << endl;
      std::this_thread::sleep_for(std::chrono::seconds(2));

      backup.is_running = false;
      DEBUG_INFO << std::this_thread::get_id() << " > sent " << iteraciones
                 << endl;
      if (backup.input_packet_queue.size() > 0)
        DEBUG_INFO << std::this_thread::get_id() << " > received: " << std::dec
                   << backup.input_packet_queue.size() << endl;
    }));
  }
  std::cout << "main thread\n";

  for (auto &cl : clients) {
    cl.detach();
  }
  master_thread.join();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  return true;
}

bool basic_test::test_client_pool() { return false; }

int basic_test::run_master(int port) {
  pkt_parser *parser = new pkt_parser();
  connection master(port);
  master.set_parser(parser);
  master.start();
  while (master.is_running) {
    DEBUG_INFO << " MASTER waiting ..... " << endl;
    auto pkt = master.get_packet();
    if (pkt->type == CONNECT)
      continue;
    DEBUG_INFO << " MASTER_IN " << pkt->pkt_len << endl;
    pkt->pkt_len++;
    master.send_packet(pkt);
    DEBUG_INFO << " MASTER_OUT :" << pkt->pkt_len << endl;
    DEBUG_INFO << " MASTER_Q_SIZE: " << std::dec
               << master.input_packet_queue.size() << endl;
    delete pkt;
  }
  return true;
}

int basic_test::run_backup(std::string host, int port) {
  pkt_parser *parser = new pkt_parser();

  int iteraciones = 0;
  xt_recent_entry entry;
  entry.set_src_ip("192.168.0.159");
  entry.set_ttl(66);
  entry.ip_family_ = IPV4;
  for (int i = 1; i <= 32; i++) {
    entry.timestamps.push_back(4700000000 + (i * 1000));
  }
  entry.timestamps[0] = 4700000000 + (33 * 1000);
  entry.timestamps[1] = 4700000000 + (34 * 1000);
  entry.set_oldest_pkt_index(2);

  xt_recent_action *xaction = new xt_recent_action(ADD_UPD_ENTRY);
  xaction->xt_table_name = "abdess";
  xaction->target_entry = entry;
  xaction->pkt_len = 1;
  connection backup(host, port);
  backup.set_parser(parser);
  backup.start();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  if (!backup.send_packet(xaction))
    backup.is_running = false;
  DEBUG_INFO << std::this_thread::get_id()
             << " BACKUP_OUT :" << xaction->pkt_len << endl;
  iteraciones = 1;
  while (iteraciones < 1000 && backup.is_running) {
    DEBUG_INFO << " BACKUP waiting ..... " << iteraciones + 1 << endl;

    auto pkt = backup.get_packet();
    if (pkt->type == CONNECT)
      continue;
    DEBUG_INFO << " BACKUP_IN " << pkt->pkt_len << endl;

    backup.send_packet(pkt);
    DEBUG_INFO << " BACKUP_OUT : " << pkt->pkt_len << "\tIT:" << iteraciones
               << endl;
    iteraciones++;
    delete pkt;
  }
  DEBUG_INFO << " BACKUP_Closing " << endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));

  DEBUG_INFO << " > BACKUP_IT " << iteraciones << endl;
  if (backup.input_packet_queue.size() > 0)
    DEBUG_INFO << " > BACKUP_Q_SIZE: " << std::dec
               << backup.input_packet_queue.size() << endl;
  backup.is_running = false;
  return 0;
}

void basic_test::run_test() {
  DEBUG_INFO << "Serialization...";
  if (test_xt_action_serialization())
    DEBUG_INFO << " OK" << std::endl;
  else
    DEBUG_INFO << " FAILED" << std::endl;
  DEBUG_INFO << "Server multiclient test";
  if (test_server_client())
    DEBUG_INFO << " OK" << std::endl;
  else
    DEBUG_INFO << " FAILED" << std::endl;
}
