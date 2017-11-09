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

#include "sync_controller.h"
#include "../debug/errors.h"
#include "sync_backup.h"
#include "sync_master.h"
#include <cstring>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/un.h>
#include <zconf.h>

#define XT_RECENT_APP_PATH "/tmp/ssyncd_path"
#define MAX_COMMAND_SIZE 1024

sync_controller::sync_controller() {
  sync_worker = nullptr;
  command_parser = new command_pkt_parser();
}

sync_controller::~sync_controller() {
  unlink(XT_RECENT_APP_PATH);
  delete command_parser;
}

void sync_controller::ctl_task(sync_controller &obj) {
  int count, new_conn_fd = -1;
  unsigned char recv_buffer[MAX_COMMAND_SIZE];
  command_pkt_parser *parser = obj.command_parser;
  while (obj.is_running) {
    new_conn_fd = accept(obj.listen_fd, NULL, NULL);
    if (new_conn_fd < 0) {
      std::string err_ = "accept() failed:  ";
      err_ += std::strerror(errno);
      DEBUG_LOG << err_ << std::endl;
      continue;
    }
    count = recv(new_conn_fd, recv_buffer, sizeof(recv_buffer), 0);
    if (count < 0) {
      std::string err_ = "recv() failed:  ";
      err_ += std::strerror(errno);
      DEBUG_LOG << err_ << std::endl;
      continue;
    }

    if (count == 0) {
      std::string err_ = "recv, connection closed: ";
      DEBUG_LOG << err_ << std::endl;
      close(new_conn_fd);
      continue;
    }

    unsigned int buffer_used;
    command_pkt *pkt = static_cast<command_pkt *>(
        parser->deserialize(recv_buffer, count, &buffer_used));
    if (pkt != nullptr) {
      string response = "";
      switch (pkt->command) {
        case QUIT:obj.stop();
          exit(EXIT_SUCCESS);
        case START_RECENT_SYNC:
          if (!obj.sync_worker->is_recent_sync_running())
            obj.sync_worker->start_recent_sync();
          else
            obj.send_response(new_conn_fd,
                              "recent sync service is already running");
          break;
        case STOP_RECENT_SYNC:
          if (obj.sync_worker->is_recent_sync_running()) {
            obj.send_response(new_conn_fd, "stopping recent sync service\n");
            obj.sync_worker->stop_recent_sync();
          } else
            obj.send_response(new_conn_fd, "recent sync service not running\n");
          break;
        case START_HTTP_SYNC:
          if (!obj.sync_worker->is_http_sync_running()) {
            obj.send_response(new_conn_fd, "starting http sync service\n");
            if (!obj.sync_worker->start_http_sync(
                obj.current_mode == controller::MASTER ? true : false)) {
              obj.send_response(new_conn_fd, "error starting http sync service");
              to_syslog(LOG_ERR, "error starting http sync service");
              break;
            }
          }
          if (obj.sync_worker->add_http_listener_farm(pkt->command_data))
            obj.send_response(new_conn_fd, "http farm added");
          else
            obj.send_response(new_conn_fd, "error adding http farm listener");

          break;
        case CLEAR_HTTP:obj.sync_worker->clear_http_data();
          break;
        case STOP_HTTP_SYNC:
          if (obj.sync_worker->is_http_sync_running()) {
            //          obj.send_response(new_conn_fd, "stopping http sync
            //          service\n"); obj.sync_worker->stop_http_sync();

            obj.sync_worker->remove_http_listener_farm(pkt->command_data);
            obj.send_response(new_conn_fd, "http farm removed");
          } else
            obj.send_response(new_conn_fd, "http sync service not running\n");
          break;
        case SW_TO_CLIENT:obj.server_port = pkt->server_port;
          obj.server_address = pkt->server_address;
          obj.set_worker_mode(controller::BACKUP);
          obj.start();
          break;
        case SW_TO_MASTER:obj.server_port = pkt->server_port;
          obj.set_worker_mode(controller::MASTER);
          obj.start();
          break;
        case WRITE_RECENT:obj.send_response(new_conn_fd, "writing data");
          if (obj.sync_worker->write_recent_all()) {
            obj.send_response(new_conn_fd, "recent data written correclty");
            to_syslog(LOG_ALERT, "recent data written correclty");
          } else {
            obj.send_response(new_conn_fd, "error writing recent data");
            to_syslog(LOG_ERR, "error writing recent data");
          }
          break;
        case WRITE_HTTP: {
          if (obj.sync_worker->write_http_data()) {
            obj.send_response(new_conn_fd, "http session data written correclty");
            to_syslog(LOG_ALERT, "http session data written correclty");
          } else {
            obj.send_response(new_conn_fd, "error writing http session data");
            to_syslog(LOG_ERR, "error writing http session data");
          }
          break;
        }
        case COUNT_RECENT: {
          unsigned long number = 0;
          for (auto &table : obj.sync_worker->get_recent_data()) {
            obj.send_response(new_conn_fd, "# " + table.first + ": ");
            for (auto entry : table.second.entries) {
              number++;
            }
            obj.send_response(new_conn_fd, std::to_string(number) + "\n");
            number = 0;
          }
          break;
        }
        case CLEAR_RECENT:obj.sync_worker->clear_recent_data();
          break;
        case SHOW_RECENT:
          for (auto &table : obj.sync_worker->get_recent_data()) {
            obj.send_response(new_conn_fd, "# " + table.first + "\n");
            for (auto entry : table.second.entries) {
              obj.send_response(new_conn_fd, entry.second.to_string() + "\n");
            }
          }
          break;
        case COUNT_HTTP: {
          unsigned int count = 0;
          for (auto &farm : obj.sync_worker->get_http_data()) {
            string data = "farm:  " + farm.first + "\n";
            obj.send_response(new_conn_fd, data);
            for (auto &listener : farm.second) {
              for (auto &service : listener.second) {
                for (auto &session : service.second.sessions) {
                  count++;
                }
                obj.send_response(new_conn_fd, std::to_string(count) + "\n");
                count = 0;
              }
            }
          }
        }
          break;
        case SHOW_HTTP: {
          std::string data = "";
          for (auto &farm : obj.sync_worker->get_http_data()) {
            data = "farm:  " + farm.first + "\n";
            obj.send_response(new_conn_fd, data);
            for (auto &listener : farm.second) {
              data = "\thttp Listener: " + std::to_string(listener.first) + "\n";
              obj.send_response(new_conn_fd, data);
              for (auto &service : listener.second) {
                data = " \t\tService: " + std::to_string(service.first) + "\n";
                obj.send_response(new_conn_fd, data);
                for (auto &session : service.second.sessions) {
                  data = "\t\t\tSession: " + session.first + " [ " +
                      std::to_string(session.second.last_acc) + " ] >> " +
                      session.second.content + "\n";
                  obj.send_response(new_conn_fd, data);
                }
              }
            }
          }
          break;
        }
        case CLEAR_RECENT_TABLE: {
          int res = obj.sync_worker->clear_recent_data(pkt->command_data);
          switch (res) {
            case 0:obj.send_response(new_conn_fd, "table cleared correclty");
              break;
            case -2:obj.send_response(new_conn_fd, "table doesn't exist");
              break;
            default:obj.send_response(new_conn_fd, "error clearing table");
              break;
          }
          break;
        }
        case WRITE_RECENT_TABLE: {
          int res = obj.sync_worker->write_recent_data(pkt->command_data);
          switch (res) {
            case 0:obj.send_response(new_conn_fd, "table written correclty");
              break;
            case -2:obj.send_response(new_conn_fd, "table doesn't exist");
              break;
            default:obj.send_response(new_conn_fd, "error writing table");
              break;
          }
          break;
        }
        case SHOW_MODE:
          if (obj.current_mode == controller::BACKUP) {
            obj.send_response(new_conn_fd, "backup");
          } else if (obj.current_mode == controller::MASTER) {
            obj.send_response(new_conn_fd, "master");
          } else {
            obj.send_response(new_conn_fd, "error");
          }
          break;
        default: {
          obj.send_response(new_conn_fd, "Command no found\n");
          break;
        }
      }
      close(new_conn_fd);
      delete pkt;
    } else {
      obj.send_response(new_conn_fd, "Command error");
    }
  }
}

void sync_controller::init_ctl_interface() {
  unlink(XT_RECENT_APP_PATH);
  listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::string err_ = "socket() failed:  ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
  }
  struct sockaddr_un serveraddr;
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sun_family = AF_UNIX;
  strcpy(serveraddr.sun_path, XT_RECENT_APP_PATH);
  int res =
      bind(listen_fd, (struct sockaddr *) &serveraddr, SUN_LEN(&serveraddr));
  if (res < 0) {
    std::string err_ = "bind() failed:  ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
    exit(ERROR_NOT_DEFINED);
  }
  res = listen(listen_fd, 10);
  if (res < 0) {
    std::string err_ = "listen() failed:  ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
  }
}

void sync_controller::send_response(int fd, std::string str) {
  int sent = 0;
  int count = 0;
  while (sent < str.size()) {
    count = send(fd, str.c_str() + sent, str.size() - sent, 0);
    if (count < 0) {
      std::string err_ = "send() failed:  ";
      err_ += std::strerror(errno);
      OUT_ERROR << err_ << std::endl;
      return;
    }
    sent += count;
  }
}

void sync_controller::set_worker_mode(controller::worker_mode mode) {
  if (sync_worker != nullptr) {
    sync_worker->stop();
    delete sync_worker;
  }
  switch (mode) {
    case controller::MASTER:sync_worker = new sync_master();
      sync_worker->init_master(this->server_port);
      break;
    case controller::BACKUP:sync_worker = new sync_backup();
      sync_worker->init_backup(this->server_address, this->server_port);
      break;
    default:OUT_ERROR << "no working mode defined" << std::endl;
  }
  current_mode = mode;
}

void sync_controller::start() {
  if (sync_worker != nullptr) {
    sync_worker->start();
  } else {
    OUT_ERROR << "no working mode has been defined" << std::endl;
    return;
  }
  is_running = true;
  command_thread = thread(&ctl_task, std::ref(*this));
}

void sync_controller::stop() {
  if (sync_worker != nullptr) {
    sync_worker->stop();
    delete sync_worker;
  }
  is_running = false;
  close(listen_fd);
}
