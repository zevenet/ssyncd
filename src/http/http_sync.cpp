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

#include "http_sync.h"
#include "../debug/errors.h"
#include "http_parser.h"
#include "sys/socket.h"
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <zconf.h>

void http_sync::listen_task(http_sync &obj) {

  int count = 0;
  int received = 0;
  
  struct epoll_event events[HTTP_MAX_CLIENTS];
  int i;

  while (obj.is_running) {
    count = epoll_wait(obj.epoll_fd, events, HTTP_MAX_CLIENTS,
                       HTTP_EPOLL_TIMEOUT_MS);
    if (-1 == count) {
      continue;
    }
    for (i = 0; i < count; i++) {
      if (events[i].events & EPOLLIN) // socket is ready for writing
      {
        connection_event_data *c_data =
            (connection_event_data *) events[i].data.ptr;
        if ((HTTP_MAX_PACKET_SIZE - c_data->buffer_size) > 4) {
          std::memset(c_data->buffer + c_data->buffer_size, 0,
                      5); // clear header
        }

        received = 0;
        while (
            (received = recv(c_data->fd, c_data->buffer + c_data->buffer_size,
                             HTTP_MAX_PACKET_SIZE - c_data->buffer_size,
                             MSG_NOSIGNAL)) > 0) {
          c_data->buffer_size += received;
          if (received >= HTTP_MAX_PACKET_SIZE)
            break;
        }
        if (received == -1 && errno != EAGAIN) {
          OUT_ERROR << "pound_thread; recv() failed: " << strerror(errno)
                    << std::endl;
          if (errno == ECONNRESET) {
            obj.peer_disconnect_handler(c_data->fd);
          }
          continue;
        } else if (received == 0) {
          std::string res = "pound_thread; recv() connection closed: ";
          res += strerror(errno);
          res += "\n";
          OUT_ERROR << res << std::endl;
          to_syslog(LOG_ERR, res.c_str());
          obj.peer_disconnect_handler(c_data->fd); // TODO:: RECONECT
          continue;
        } else if (c_data->buffer_size > 0) {
          while (true) {
            unsigned int buffer_used = 0;
            http::http_action *action =
                static_cast<http::http_action *>(obj.parser.deserialize(
                    c_data->buffer, c_data->buffer_size, &buffer_used));
#ifdef DEBUG_HTTP_CON
            DEBUG_LOG << ">> farm name " << obj.farms_fd[c_data->fd]
                      << std::endl;
            DEBUG_LOG << ">> Buffer size " << c_data->buffer_size << std::endl;
            DEBUG_LOG << ">> Buffer used " << buffer_used << std::endl;
#endif
            if (action != nullptr) {
#ifdef DEBUG_HTTP_CON
              if (action->session.last_acc < 1000000) {
                string error = " HTTP ERROR " +
                               std::to_string(action->listener) + " " +
                               std::to_string(action->service) +
                               " src: " + action->session.key + "[" +
                               std::to_string(action->session.last_acc) +
                               "] >>  " + action->session.content;
                OUT_ERROR << error << std::endl;
                to_syslog(LOG_DEBUG, error.c_str());
              } else {

                switch (action->action) {
                case http::SESS_ADD: {
                  DEBUG_LOG << " HTTP ADD " << action->listener << " "
                            << action->service
                            << " src: " << action->session.key << "["
                            << action->session.last_acc << "] >>  "
                            << action->session.content << std::endl;
                  break;
                }
                case http::SESS_UPDATE: {
                  DEBUG_LOG << " HTTP UPDATE " << action->listener << " "
                            << action->service
                            << " src: " << action->session.key << "["
                            << action->session.last_acc << "] >>  "
                            << action->session.content << std::endl;
                  break;
                }
                case http::SESS_DELETE: {
                  DEBUG_LOG << " HTTP DELETE " << action->listener << " "
                            << action->service << " src: " << std::endl;
                  break;
                }
                }
              }
#endif
              action->farm_name = obj.farms_fd[c_data->fd];
              action->fd = c_data->fd;
              obj.input_data.enqueue(action);
              c_data->buffer_size -= buffer_used;
              if ((HTTP_MAX_PACKET_SIZE - c_data->buffer_size) >= buffer_used)
                memmove(c_data->buffer, c_data->buffer + buffer_used,
                        c_data->buffer_size);
            } else {
              delete action;
              break;
            }
          }
        }
      }
      if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
        // obj.peer_disconnect_handler(c_data->fd);
        OUT_ERROR << "epoll failed, EPOLLRDHUP | EPOLLHUP" << std::endl;
        continue;
      }
      if (events[i].events & EPOLLERR) {
        perror("epoll");
        continue;
      }
    }
  }
  OUT_ERROR << "Closing http service listen task" << std::endl;
  obj.is_running = false;
  close(obj.epoll_fd);
}

void http_sync::peer_disconnect_handler(int fd) {
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
  close(fd);
  http::http_action *packet = new http::http_action();
  packet->type = packet_type::DISCONNECT;
  packet->farm_name = farms_fd[fd];
  packet->fd = fd;
  input_data.enqueue(packet);
}

int http_sync::get_farm_fd(std::string farm_name) {

  int fd = 0;
  for (auto &farm_fd : farms_fd) {
    if (farm_fd.second == farm_name)
      fd = farm_fd.first;
  }
  return fd;
}

http_sync::http_sync(bool enable_listen_mode) {
  is_listener = enable_listen_mode;
}

http_sync::~http_sync() {}

bool http_sync::send_pound_action(string farm_name, http::http_action *action) {
  if (get_farm_fd(farm_name) == 0) {
    if (!add_farm(farm_name))
      return false;
  }

  char *to_send = new char[HTTP_MAX_PACKET_SIZE];
  std::memset(to_send, 0, HTTP_MAX_PACKET_SIZE);
  unsigned int to_send_size_;
  action->fd = get_farm_fd(farm_name);
  action->farm_name = farm_name;
  if (parser.serialize(action, to_send, &to_send_size_)) {
    std::lock_guard<std::mutex> scope_lock(send_mutex);
#ifdef DEBUG_HTTP_CON
    DEBUG_LOG << "\n\theader_1: 0x" << std::hex << action->header_1 << std::endl
              << "\theader_2: 0x" << std::hex << action->header_2 << std::endl
              << "\ttype: 0x" << std::hex << action->type << std::endl
              << "\tlen: " << std::dec << action->pkt_len << std::endl
              << "\taction: " << std::dec << action->action << std::endl;
#endif
    int ret = 0;
    int sent = 0;
    while (true) {
      ret =
          send(action->fd, to_send + sent, to_send_size_ - sent, MSG_NOSIGNAL);
      if ((ret == -1) && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue;
      }
      if (ret == (to_send_size_ - sent)) {
        break;
      } else if (ret == -1) {
        std::string err_ = "http_sync: send packet error to fd: ";
        err_ += std::strerror(errno);
        OUT_ERROR << err_ << std::endl;
        delete[] to_send;
        return false;
      }
      sent += ret;
    }
  } else {
    DEBUG_LOG << "Error serializing packet to " << action->fd << std::endl;
  }
  delete[] to_send;
  return true;
}

bool http_sync::init() {
  epoll_fd = epoll_create(HTTP_MAX_CLIENTS);

  return true;
}

bool http_sync::start() {
  //  if (!is_connected)
  //    return false;
  is_running = true;
  try {
    listen_thread = std::thread(listen_task, std::ref(*this));
    listen_thread.detach();
    return true;
  } catch (std::exception ex) {
    OUT_ERROR << "Error starting http listener thread: ";
    OUT_ERROR << ex.what();
    OUT_ERROR << std::endl;
    return false;
  }
}

void http_sync::stop() {
  if (is_running) {
    is_running = false;
  }
  //  is_connected = false;
}

bool http_sync::add_farm(std::string farm_name) {
  string service_socket = "/tmp/ssync_" + farm_name + ".socket";
  int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::string err_ = "socket() failed:  ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
    return false;
  }
  struct sockaddr_un serveraddr;
  strcpy(serveraddr.sun_path, service_socket.c_str());
  serveraddr.sun_family = AF_UNIX;
  int rc =
      connect(listen_fd, (struct sockaddr *) &serveraddr, SUN_LEN(&serveraddr));
  if (rc < 0) {
    std::string err_ = "connect() failed: ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
    to_syslog(LOG_NOTICE, (err_ = "add http farm: " + err_).c_str());
    return false;
  }
  // is_connected = true;
  int one = 1;
  int flags, s;
  flags = fcntl(listen_fd, F_GETFL, 0);
  if (flags == -1) {
    std::string err_ = "fcntl() F_GETFL failed:  ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
    return false;
  }

  flags |= O_NONBLOCK;
  s = fcntl(listen_fd, F_SETFL, flags);
  if (s == -1) {
    std::string err_ = "socket(F_SETFL) failed:  ";
    err_ += std::strerror(errno);
    OUT_ERROR << err_ << std::endl;
    return false;
  }

  // add to epoll listener
  connection_event_data *cd = new connection_event_data;
  cd->fd = listen_fd;
  cd->buffer_size = 0;
  epoll_event Edgvent;
  Edgvent.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET; // EPOLLOUT |
  Edgvent.data.ptr = (void *) cd;
  if (epoll_ctl((int) epoll_fd, EPOLL_CTL_ADD, listen_fd, &Edgvent) != 0) {
    OUT_ERROR << "epoll_ctl, adding socket " << strerror(errno) << std::endl;
    return false;
  }
  farms_fd[listen_fd] = farm_name;
  http::http_action *packet = new http::http_action();
  packet->type = packet_type::CONNECT;
  packet->farm_name = farm_name;
  packet->fd = listen_fd;
  input_data.enqueue(packet);
  return true;
}

bool http_sync::remove_farm(std::string farm_name) {
  int fd = get_farm_fd(farm_name);

  if (fd != 0) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);

    http::http_action *packet = new http::http_action();
    packet->type = packet_type::DISCONNECT;
    packet->farm_name = farm_name;
    packet->fd = fd;
    input_data.enqueue(packet);
    return true;
  } else
    return false;
}
