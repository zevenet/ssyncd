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

#include "sync_backup.h"
#include <algorithm>
#include <cstring>
#include <fstream>

sync_backup::sync_backup() {}

sync_backup::~sync_backup() {}

void sync_backup::run() {
  if (!conn->is_running) {
    OUT_ERROR << "No connection to master node, reconnecting..." << std::endl;
    exit(EXIT_FAILURE);
  }
  base_sync::run();
  conn_packet *packet = new conn_packet();
  packet->type = packet_type::SYNC_REQUEST;
  if (!conn->send_packet(packet)) {
    return;
  }

  http_listener.is_listener = false;
  while (this->is_running) {
    conn_packet *pkt = conn->get_packet();

    if (conn->is_running && pkt == nullptr) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }
    if (pkt->type == DISCONNECT) {
      OUT_ERROR << "Master node disconected, reconecting ..." << std::endl;
      while (this->is_running && !conn->is_running) {
        conn->stop();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        conn->start();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if (conn->is_running) {
          http_session_data.clear();
          conn_packet *packet = new conn_packet();
          packet->type = packet_type::SYNC_REQUEST;
          conn->send_packet(packet);
          // TODO: Restart servicess, maybe send last timestamps received????
        }
      }
    }
    switch (pkt->type) {
      case XT_RECENT_SYNC: {
        std::unique_ptr<xt_recent_action> xaction(
            static_cast<xt_recent_action *>(pkt));
#ifdef DEBUG_BACKUP_CON
        switch (xaction->action) {
        case ADD_UPD_ENTRY: {
          DEBUG_LOG << " UPDATE"
                    << " table " << xaction->xt_table_name
                    << " src :" << xaction->target_entry.get_src_ip()
                    << std::endl;
          break;
        }
        case DELETE_ENTRY: {
          DEBUG_LOG << " DELETE"
                    << " table " << xaction->xt_table_name
                    << " src :" << xaction->target_entry.get_src_ip() << " len "
                    << xaction->pkt_len << std::endl;
          break;
        }
        }
#endif
        if (!apply_recent_action(xaction.get())) {
          DEBUG_LOG << "Error applying recent action " << std::flush;
          to_syslog(LOG_NOTICE, "Error applying recent action");
        }
        break;
      }
      case HTTP_SYNC: {
        std::unique_ptr<http::http_action> paction(
            static_cast<http::http_action *>(pkt));

#ifdef DEBUG_BACKUP_CON

        {
          switch (paction->action) {
          case http::CLEAR_DATA: {
            DEBUG_LOG << " [" << paction->farm_name << "]  "
                      << " HTTP ADD " << paction->listener << " "
                      << paction->service << " " << paction->backend
                      << " src: " << paction->session.key << "["
                      << paction->session.last_acc << "] >> "
                      << paction->session.content << std::endl;
            break;
          }
          case http::SESS_ADD: {
            DEBUG_LOG << " [" << paction->farm_name << "]  "
                      << " HTTP ADD " << paction->listener << " "
                      << paction->service << " " << paction->backend
                      << " src: " << paction->session.key << "["
                      << paction->session.last_acc << "] >> "
                      << paction->session.content << std::endl;
            break;
          }
          case http::SESS_UPDATE: {
            DEBUG_LOG << " [" << paction->farm_name << "]  "
                      << " HTTP UPDATE " << paction->listener << " "
                      << paction->service << " " << paction->backend
                      << " src: " << paction->session.key << "["
                      << paction->session.last_acc << "] >> "
                      << paction->session.content << std::endl;
            break;
          }
          case http::SESS_DELETE: {
            DEBUG_LOG << " [" << paction->farm_name << "]  "
                      << " HTTP DELETE " << paction->listener << " "
                      << paction->service << " src: " << std::endl;
            break;
          }
          }
        }
#endif
        if (!apply_http_action(paction.get())) {
          DEBUG_LOG << "Error applying http action " << std::flush;
          to_syslog(LOG_NOTICE, "Error applying http action");
        }
      }
        break;
      default:break;
    }
  }
  DEBUG_INFO << "Master disconected" << std::endl;
#if __cplusplus >= 201703L
  on_master_close();
}

void sync_backup::handle_on_master_disconect(std::function<void()> callback) {
  on_master_close = callback;
#endif
}
