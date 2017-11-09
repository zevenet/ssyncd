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

#include "sync_master.h"
#include <algorithm>
#include <fstream>
bool sync_master::handle_recent_sync(int fd) {
  xt_recent_action *xaction_tosend;
  int sec = 0;
  for (auto table : recent_data) {
    DEBUG_LOG << "sending table: " << table.first << std::endl;
    for (auto entry : table.second.entries) {
      sec++;
      xaction_tosend =
          xt_recent_sync::get_entry_as_action(table.first, &entry.second);
      xaction_tosend->fd = fd;
      if (!conn->send_packet(xaction_tosend)) {
        delete xaction_tosend;
        return false;
      }
      delete xaction_tosend;
    }
  }

  return true;
}

bool sync_master::handle_http_sync(int fd) {
  if (!http_listener.is_running)
    return false;
  http::http_action action_;
  action_.action = http::SESS_ADD;
  std::string data = "";
  for (auto &data : get_http_data()) {
    for (auto &listener : data.second) {
      action_.listener = listener.first;
      for (auto &service : listener.second) {
        action_.service = service.first;
        for (auto &session : service.second.sessions) {
          action_.session = session.second;
          action_.backend = session.second.backend_id;
          action_.fd = fd;
          action_.farm_name = data.first;
          DEBUG_LOG << " HTTP ADD " << action_.listener << " "
                    << action_.service << " " << action_.session.backend_id
                    << " src: " << action_.session.key << "["
                    << action_.session.last_acc << "] >> "
                    << action_.session.content << std::endl;
          conn->send_packet(&action_);
        }
      }
    }
  }
  return true;
}

sync_master::sync_master() {}

sync_master::~sync_master() {}

void sync_master::run() {
  if (!conn->is_running) {
    OUT_ERROR << "No connection error" << std::endl;
    return;
  }
  base_sync::run();
  http_listener.is_listener = true;
  bool wait = true;
  int max_items = 0;
  while (this->is_running) {
    // Check requests [new connections, sync_requests, desconections]

    std::unique_ptr<conn_packet> req(conn->get_packet(false));
    if (req != nullptr) {
      switch (req->type) {
        case CONNECT: {
          backup_servers_fd++;
          OUT_ERROR << "Backup node connected" << std::endl;
          to_syslog(LOG_ALERT, "Backup node connected");
          continue;
        }
        case SYNC_REQUEST: {
          if (recent_listener.is_running && !handle_recent_sync(req->fd)) {
            OUT_ERROR << "Error sending recent data " << std::endl;
          }
          if (http_listener.is_running && !handle_http_sync(req->fd)) {
            OUT_ERROR << "Error sending http data " << std::endl;
          }
          continue;
        }
        case DISCONNECT:backup_servers_fd--;
          OUT_ERROR << "Backup node disconnected" << std::endl;
          to_syslog(LOG_ALERT, "Backup node disconnected");
          continue;
        default:continue;
      }
    }

    if (wait || (!recent_listener.is_running && !http_listener.is_running)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(600));
      wait = false;
      continue;
    }
    if (!http_listener.is_running && get_http_data().size() > 0) {
      to_syslog(LOG_ALERT, "clearing http session data");
      for (auto &lstn : get_http_data()) {
        http::http_action clear_request;
        clear_request.fd = -1;
        clear_request.farm_name = lstn.first;
        clear_request.action = http::CLEAR_DATA;
        conn->send_packet(&clear_request);
      }
      clear_http_data(); // TODO:: POUND DISCONNECTED!!! CLEAR DATA
      wait = true;

    } else if (http_listener.is_running) {
      max_items = 0;
      while (true) {
        if (max_items == 10) {
          wait = false;
          break;
        }
        std::unique_ptr<http::http_action> pkt(static_cast<http::http_action *>(
                                                   http_listener.input_data.try_dequeue()));
        if (pkt == nullptr || max_items > 10) {
          // std::this_thread::sleep_for(std::chrono::milliseconds(200));
          wait = true;
          break;
        }
        if (pkt.get()->type == CONNECT || pkt.get()->type == DISCONNECT) {
          DEBUG_LOG << "[" << pkt->farm_name << "] "
                    << " HTTP CNT " << pkt->listener << " " << pkt->service
                    << " src: " << pkt->session.key << "["
                    << pkt->session.last_acc << "] >>  " << pkt->session.content
                    << std::endl;
          http::http_action clear_request;
          clear_request.fd = -1;
          clear_request.farm_name = pkt.get()->farm_name;
          clear_request.action = http::CLEAR_DATA;
          conn->send_packet(&clear_request);
          clear_http_farm_data(pkt.get()->farm_name);

          if (pkt.get()->type == CONNECT) {
            http::http_action sync_request;
            sync_request.fd = pkt.get()->fd;
            sync_request.action = http::SYNC_REQUEST;
            http_listener.send_pound_action(pkt.get()->farm_name,
                                            &sync_request);
          }
          continue;
        }
        max_items++;
        if (!apply_http_action(pkt.get())) {
          // pound_pending_actions.enqueue(*pkt);
          // pound_listener.input_data.enqueue(pkt);//TODO:: WHAT TO DO??
          OUT_ERROR << "Error apply http data" << std::endl;
        } else {
          pkt->fd = -1;
          if (backup_servers_fd > 0 && !conn->send_packet(pkt.get())) {
            OUT_ERROR << "Error sending http data to backup" << std::endl;
          } else {
#ifdef DEBUG_MASTER_CON
            if (pkt->listener > 10 || pkt->service > 5 ||
                pkt->listener < 0 || // TODO:DELETE
                pkt->service < 0 || pkt->session.last_acc < 0 ||
                pkt->session.last_acc < 1000000) {
              DEBUG_LOG << "[" << pkt->farm_name << "] "
                        << " HTTP ERROR " << pkt->listener << " "
                        << pkt->service << " src: " << pkt->session.key << "["
                        << pkt->session.last_acc << "] >>  "
                        << pkt->session.content << std::endl;
            } else {
              switch (pkt->action) {
              case http::SYNC_REQUEST:
                break;
              case http::SESS_UPDATE:
                DEBUG_LOG << "[" << pkt->farm_name << "] "
                          << " HTTP UPDATE " << pkt->listener << " "
                          << pkt->service << " src: " << pkt->session.key << "["
                          << pkt->session.last_acc << "] >>  "
                          << pkt->session.content << std::endl
                          << std::endl;
                break;
              case http::SESS_ADD:
                DEBUG_LOG << "[" << pkt->farm_name << "] "
                          << " HTTP ADD " << pkt->listener << " "
                          << pkt->service << " src: " << pkt->session.key << "["
                          << pkt->session.last_acc << "] >>  "
                          << pkt->session.content << std::endl;

                break;
              case http::SESS_DELETE:
                DEBUG_LOG << " HTTP DELETE" << pkt->listener << " "
                          << pkt->service << " src: " << pkt->session.key
                          << std::endl;
                break;
              case http::BCK_ADD:
                break;
              case http::BCK_DELETE:
                break;
              case http::BCK_UPDATE:
                break;
              }
#endif
          }
        }
        // delete pkt;
      }
    } else {
      wait = true;
    }
    if (recent_listener.is_running) {
      max_items = 0;
      while (true) {
        if (max_items == 10) {
          wait = false;
          break;
        }
        std::unique_ptr<xt_recent_action> pkt(static_cast<xt_recent_action *>(
                                                  recent_listener.input_data.try_dequeue()));
        if (pkt == nullptr) {
          wait = wait ? true : false;
          break;
        } else {
          max_items++;
          if (!apply_recent_action(pkt.get())) {
            // recent_pending_actions.enqueue(pkt.get());//TODO:: WHAT TO DO??
            OUT_ERROR << "Error apply recent data" << std::endl;
          }

          if (backup_servers_fd > 0 && !conn->send_packet(pkt.get())) {
            OUT_ERROR << "Error sending recent data to backup" << std::endl;
          }
#ifdef DEBUG_MASTER_CON
          else {
            switch (pkt->action) {
            case ADD_UPD_ENTRY:
              DEBUG_LOG << " UPDATE " << pkt->xt_table_name
                        << " src :" << pkt->target_entry.get_src_ip()
                        << " index :"
                        << pkt->target_entry.get_oldest_pkt_index()
                        << " lastseen :" << pkt->target_entry.get_last_seen()
                        << std::endl;
              break;
            case DELETE_ENTRY:
              DEBUG_LOG << " DELETE" << pkt->xt_table_name
                        << " src :" << pkt->target_entry.get_src_ip()
                        << std::endl;
              break;
            } // TODO::if not sent, return to queue and resend later??
            DEBUG_LOG << std::flush << std::endl;
          }
        }
#endif
        }
      }
    }
  }
}
