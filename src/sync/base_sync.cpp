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

#include "base_sync.h"
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

base_sync::base_sync() {
  is_running = false;
  parser = new pkt_parser();
}

base_sync::~base_sync() {
  conn->stop();
  if (http_listener.is_running)
    http_listener.stop();
  if (recent_listener.is_running)
    recent_listener.stop();
  delete parser;
}

void base_sync::start_recent_sync() {
  // load host tables
  recent_data.clear();
  //  for (auto table : xt_recent_proc::get_xt_recent_tables()) {
  //    recent_data[table.name] = table;
  //  }
  recent_listener.start();
}

void base_sync::init_master(int port) {
  conn = new connection(port);
  conn->set_parser(parser);
}

void base_sync::init_backup(std::string address, int port) {
  conn = new connection(address, port);
  conn->set_parser(parser);
}

void base_sync::stop_recent_sync() { recent_listener.stop(); }

bool base_sync::write_recent_all() {
  for (auto table : recent_data) {
    if (table.second.entries.size() < 1)
      continue;
    string tail = "/proc/net/xt_recent/" + table.first;
    DEBUG_INFO << "Dumping data to /proc/net/xt_recent/" + table.first
               << std::endl;
    int filedesc = open(tail.c_str(), O_WRONLY);
    if (filedesc < 0) {
      OUT_ERROR << "write_recent_all:open() failed on " << tail << " "
                << std::strerror(errno) << std::endl;
      return false;
    }
    if (write(filedesc, "/", 1) < 0) {
      OUT_ERROR << "There was an error flushing table " << tail << " "
                << std::strerror(errno) << std::endl;
      close(filedesc);
      return false;
    }
    close(filedesc);
    for (auto entry : table.second.entries) {
      filedesc = open(tail.c_str(), O_WRONLY);
      std::sort(entry.second.timestamps.begin(), entry.second.timestamps.end());
      string update = "*" + entry.first + ';' +
          std::to_string(entry.second.get_ttl()) + ';';
      for (auto nstamps : entry.second.timestamps)
        update += std::to_string(nstamps) + ';';
      // update += '\0';
      if (write(filedesc, update.c_str(), update.size()) < 0) {
        OUT_ERROR << "There was an error writing to " << tail << " "
                  << std::strerror(errno) << std::endl;
        close(filedesc);
        return false;
      }
      close(filedesc);
    }
  }
  return true;
}

int base_sync::write_recent_data(std::string table_name) {
  if (recent_data.find(table_name) == recent_data.end()) {
    return -2;
  }

  auto table = recent_data[table_name];
  string tail = "/proc/net/xt_recent/" + table.name;
  DEBUG_INFO << "Dumping data to /proc/net/xt_recent/" + table.name
             << std::endl;
  int filedesc = open(tail.c_str(), O_WRONLY);
  if (filedesc < 0) {
    OUT_ERROR << "There was an error opening for write " << tail << " "
              << std::strerror(errno) << std::endl;
    return -1;
  }
  if (write(filedesc, "/", 1) < 0) {
    OUT_ERROR << "There was an error flushing table " << tail << " "
              << std::strerror(errno) << std::endl;
    close(filedesc);
    return -1;
  }
  close(filedesc);
  for (auto entry : table.entries) {
    filedesc = open(tail.c_str(), O_WRONLY);
    std::sort(entry.second.timestamps.begin(), entry.second.timestamps.end());
    string update =
        "*" + entry.first + ';' + std::to_string(entry.second.get_ttl()) + ';';
    for (auto nstamps : entry.second.timestamps) // TODO:: SORT
      update += std::to_string(nstamps) + ';';
    // update += '\0';
    if (write(filedesc, update.c_str(), update.size()) < 0) {
      OUT_ERROR << "There was an error writing to " << tail << " "
                << std::strerror(errno) << std::endl;
      close(filedesc);
      continue;
    }
    close(filedesc);
  }
  return 0;
}

int base_sync::clear_recent_data(std::string table_name) {
  if (recent_data.find(table_name) == recent_data.end()) {
    return -2;
  }
  auto table = recent_data[table_name];
  table.entries.clear();
  string tail = "/proc/net/xt_recent/" + table.name;
  DEBUG_INFO << "clearing data in /proc/net/xt_recent/" + table.name
             << std::endl;
  int filedesc = open(tail.c_str(), O_WRONLY);
  if (filedesc < 0) {
    OUT_ERROR << "There was an error opening for write " << tail << " "
              << std::strerror(errno) << std::endl;
    return -1;
  }
  if (write(filedesc, "/", 1) < 0) {
    OUT_ERROR << "There was an error flushing table " << tail << " "
              << std::strerror(errno) << std::endl;
    close(filedesc);
    return -1;
  }
  close(filedesc);
  return 0;
}

void base_sync::clear_recent_data() {
  for (auto &table : recent_data) {
    clear_recent_data(table.first);
  }
}

bool base_sync::start_http_sync(bool clear_data) {
  if (clear_data)
    http_session_data.clear();
  if (!http_listener.init()) {
    OUT_ERROR << "http daemon not running " << std::endl;
    return false;
  }
  if (!http_listener.start()) {
    OUT_ERROR << "error starting http listener " << std::endl;
    return false;
  }
  return true;
}

bool base_sync::is_http_sync_running() { return http_listener.is_running; }
bool base_sync::is_recent_sync_running() { return recent_listener.is_running; }

void base_sync::stop_http_sync() { http_listener.stop(); }

bool base_sync::write_http_data() {
  if (!http_listener.is_running) {
    start_http_sync(false);
  }
  if (http_listener.is_running) {
    http::http_action action_;
    action_.action = http::SESS_WRITE;
    std::string data = "";
    for (auto &data : get_http_data()) {

      for (auto &listener : data.second) {
        action_.listener = listener.first;
        for (auto &service : listener.second) {
          action_.service = service.first;
          for (auto &session : service.second.sessions) {
            action_.session = session.second;
            action_.backend = session.second.backend_id;
            string farm_name = data.first;
#ifdef DEBUG_HTTP_CON
            DEBUG_LOG << " HTTP WRITE " << action_.listener << " "
                      << action_.service << " " << action_.session.backend_id
                      << " src: " << action_.session.key << "["
                      << action_.session.last_acc << "] >> "
                      << action_.session.content << std::endl;
#endif
            http_listener.send_pound_action(farm_name, &action_);
          }
        }
      }
    }

    return true;
  } else
    return false;
}

void base_sync::clear_http_data() { http_session_data.clear(); }
void base_sync::clear_http_farm_data(string farm_name) {
  if (http_session_data.count(farm_name) > 0)
    http_session_data[farm_name].clear();
}

void base_sync::start() {
  http_session_data.clear();
  recent_data.clear();
  this->is_running = true;
  //  while (this->is_running && !conn->is_running) {
  //    conn->start();
  //    std::this_thread::sleep_for(std::chrono::seconds(3));
  //  }
  std::thread task(run_task, std::ref(*this));
  task.detach();
}

void base_sync::stop() {
  is_running = false;
  if (http_listener.is_running)
    http_listener.stop();
  if (recent_listener.is_running)
    recent_listener.stop();
  conn->stop();
}

bool base_sync::add_http_listener_farm(std::string farm_name) {
  return http_listener.add_farm(farm_name);
}

bool base_sync::remove_http_listener_farm(std::string farm_name) {
  return http_listener.remove_farm(farm_name);
}

void base_sync::run_task(base_sync &worker) {
  while (worker.is_running && !worker.conn->is_running) {
    worker.conn->start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }
  worker.run();
}

void base_sync::run() {
#ifdef DEBUG
  if (conn->is_server) {
    DEBUG_LOG << "Master node started" << std::endl;
    to_syslog(LOG_NOTICE, "Master node started");
  } else {
    DEBUG_LOG << "Backup node started" << std::endl;
    to_syslog(LOG_NOTICE, "Backup node started");
  }
#endif
  this->is_running = true;
}
std::map<string, xt_recent_table> base_sync::get_recent_data() const {
  return recent_data;
}

std::map<string,
         std::map<unsigned int, std::map<unsigned int, http::http_service>>>
base_sync::get_http_data() const {
  return http_session_data;
}

bool base_sync::apply_recent_action(xt_recent_action *xaction) {
  try {
    switch (xaction->action) {
      case DELETE_TABLE:
        // master_tables.erase(xaction->xt_table_name);
        break;
      case DELETE_ENTRY:
        if (recent_data.count(xaction->xt_table_name) > 0) {
          recent_data[xaction->xt_table_name].entries.erase(
              xaction->target_entry.get_src_ip());
        }
        break;
      case FLUSH_TABLE:recent_data[xaction->xt_table_name].entries.clear();
        break;
      case ADD_UPD_ENTRY:
        if (recent_data.count(xaction->xt_table_name) == 0) {
          xt_recent_table table;
          table.name = xaction->xt_table_name;
          recent_data[xaction->xt_table_name] =
              std::move(table); // xt_recent_table();
        }
        recent_data[xaction->xt_table_name]
            .entries[xaction->target_entry.get_src_ip()] = xaction->target_entry;
        break;
    }
  } catch (std::exception ex) {
    OUT_ERROR << "Error updating xt recent table:" << ex.what() << std::endl;
    return false;
  }
  return true;
}

bool base_sync::apply_http_action(http::http_action *action) {
  if (action == nullptr)
    return false;
  //  if(http_session_data.count(action->farm_name) < 1)
  //    {

  //    }
  try {
    switch (action->action) {
      case http::CLEAR_DATA:http_session_data[action->farm_name].clear();
        break;
      case http::SESS_WRITE:
      case http::SYNC_REQUEST:break;
      case http::SESS_UPDATE:
      case http::SESS_ADD: {
        if (http_session_data[action->farm_name].count(action->listener) == 0 ||
            http_session_data[action->farm_name][action->listener].count(
                action->service) == 0) {
          http::http_service service;
          map<unsigned int, http::http_service> service_to_add;
          service_to_add[action->service] = service;
          http_session_data[action->farm_name][action->listener] = service_to_add;
        }
        http_session_data[action->farm_name][action->listener][action->service]
            .sessions[action->session.key] = action->session;
      }
        break;
      case http::SESS_DELETE:
        if (http_session_data[action->farm_name].count(action->listener) > 0 &&
            http_session_data[action->farm_name][action->listener].count(
                action->service) > 0 &&
            http_session_data[action->farm_name][action->listener]
            [action->service]
                .sessions.count(action->session.key) > 0) {
          http_session_data[action->farm_name][action->listener][action->service]
              .sessions.erase(action->session.key);
        }
        break;
      case http::BCK_ADD:break;
      case http::BCK_DELETE:break;
      case http::BCK_UPDATE:break;
    }
  } catch (std::exception ex) {
    OUT_ERROR << "Exception: " << ex.what() << std::endl;
  }
  return true;
}
