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

#include "connection.h"
#include "../debug/errors.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <zconf.h>
using namespace std;

#define EPOLL_TIMEOUT_MS 3000

connection::connection(int port) {
  is_server = true;
  is_running = false;
  // Create socket
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    exit(ERROR_NOT_DEFINED);
  }
  if (set_socket_options(listen_fd) < 0)
    exit(ERROR_NOT_DEFINED);
  // Fill sockaddr_in structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY; // TODO:: filter ip to listen on
  server_addr.sin_port = htons(port);
  // Bind
  if (bind(listen_fd, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    exit(ERROR_NOT_DEFINED);
  }

  // Listen
  listen(listen_fd, MAX_CLIENTS);
}

connection::connection(string s_host, int port) {
  is_server = false;
  is_running = false;
  server_addr.sin_family = AF_INET;
  struct hostent *hp;
  hp = gethostbyname(s_host.c_str());
  bcopy(hp->h_addr, &(server_addr.sin_addr.s_addr), hp->h_length);
  server_addr.sin_port = htons(port);

  //  int ret = connect(listen_fd, (const sockaddr *)&server, sizeof(server));
  //  if (ret < 0 && errno != EINPROGRESS) {
  //    std::string error = "Error connecting to server ";
  //    error += s_host;
  //    error += " ";
  //    error += std::strerror(errno);
  //    OUT_ERROR << error << endl;
  //    to_syslog(LOG_ERR, error.c_str());
  //    exit(ERROR_NOT_DEFINED);
  //  }
}
void connection::start() {
  if (!is_server) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(listen_fd, (const sockaddr *) &server_addr,
                sizeof(server_addr)) == -1) {
      if (errno != EINPROGRESS) {
        std::string error = "Error connecting to master node ";
        error += inet_ntoa(server_addr.sin_addr);
        error += ":";
        error += std::to_string(ntohs(server_addr.sin_port));
        error += " ";
        error += std::strerror(errno);
        OUT_ERROR << error << endl;
        to_syslog(LOG_ERR, error.c_str());
        is_running = false;
        return;
      }
    } else {
      if (set_socket_options(listen_fd) < 0)
        exit(ERROR_NOT_DEFINED);
      std::string error = "Connected to master node ";
      error += inet_ntoa(server_addr.sin_addr);
      error += ":";
      error += std::to_string(ntohs(server_addr.sin_port));
      OUT_ERROR << error << endl;
      to_syslog(LOG_ERR, error.c_str());
    }
  }
  is_running = true;
  receive_thread = thread(receive_task, std::ref(*this));
  // send_thread = thread(send_task, std::ref(*this));
  receive_thread.detach();
  // send_thread.detach();
}
void connection::stop() {
  is_running = false;
  close(listen_fd);
}

void connection::receive_task(connection &cnt) {
  int ev_count, fd, i;
  int listen_fd = cnt.listen_fd;
  int receive_epfd = epoll_create(MAX_CLIENTS);
  struct epoll_event event_mask;
  struct epoll_event events[MAX_CLIENTS];
  event_mask.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET;
  // event_mask.events = EPOLLIN | EPOLLET;
  event_mask.data.fd = cnt.listen_fd;

  if (!cnt.is_server) {
    connection_event_data *cd = new connection_event_data;
    cd->fd = listen_fd;
    cd->buffer_size = 0;
    event_mask.data.ptr = (void *) cd;
  }

  if (epoll_ctl(receive_epfd, EPOLL_CTL_ADD, cnt.listen_fd, &event_mask) < 0) {
    OUT_ERROR << "epoll_ctl(2) failed on main server listen socket" << endl;
    to_syslog(LOG_EMERG, "epoll_ctl(2) failed on main server listen socket");
    exit(ERROR_NOT_DEFINED);
  }

  while (cnt.is_running) {
    ev_count = epoll_wait(receive_epfd, events, MAX_CLIENTS, EPOLL_TIMEOUT_MS);
    if (ev_count == -1) {
      OUT_ERROR << "epoll_pwait" << endl;
      to_syslog(LOG_DEBUG, "epoll wait");
      continue;
    }
    for (i = 0; i < ev_count; ++i) {
      fd = events[i].data.fd;
      if (cnt.is_server && fd == listen_fd) {
        struct sockaddr_in address;
        int address_len = sizeof(address);
        int connFD = accept(listen_fd, (struct sockaddr *) &address,
                            (socklen_t *) &address_len);

        if (connFD == -1) {
          DEBUG_LOG << "Error accepting socket: " << std::strerror(errno)
                    << std::endl;
          to_syslog(LOG_DEBUG, "Error accepting client socket");
          break;
        }
        string client_ip = inet_ntoa(address.sin_addr);
        DEBUG_LOG << "New node connected, fd: " << connFD
                  << ", IP: " << client_ip
                  << ", PORT :" << ntohs(address.sin_port) << std::endl;
        set_socket_options(connFD);
        event_mask.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET;
        connection_event_data *cd = new connection_event_data;
        cd->fd = connFD;
        cd->buffer_size = 0;
        event_mask.data.ptr = (void *) cd;
        if (epoll_ctl(receive_epfd, EPOLL_CTL_ADD, connFD, &event_mask) == -1) {
          exit(ERROR_NOT_DEFINED);
        }

        cnt.connected_nodes[connFD] = cd;
        conn_packet *packet = new conn_packet();
        packet->type = packet_type::CONNECT;
        packet->fd = connFD;
        cnt.input_packet_queue.enqueue(packet);
      } else if (events[i].events & EPOLLIN) {
        int nread = 0;
        connection_event_data *c_data =
            (connection_event_data *) events[i].data.ptr;
        if ((MAX_PACKET_SIZE - c_data->buffer_size) > 4) {
          std::memset(c_data->buffer + c_data->buffer_size, 0,
                      5); // clear header
        }
        while ((nread = recv(c_data->fd, c_data->buffer + c_data->buffer_size,
                             MAX_PACKET_SIZE - c_data->buffer_size,
                             MSG_NOSIGNAL)) > 0) {
          c_data->buffer_size += nread;
          if (nread >= MAX_PACKET_SIZE)
            break;
        }
        if (nread == -1 && errno != EAGAIN) {
          DEBUG_LOG << "read error" << endl;
          continue;
        } else if (nread == 0) {
          DEBUG_LOG << "peer disconected, fd " << c_data->fd << endl;
          conn_packet *packet = new conn_packet();
          packet->type = packet_type::DISCONNECT;
          packet->fd = c_data->fd;
          if (!cnt.is_server) {
            close(listen_fd);
            cnt.is_running = false;
            cnt.input_packet_queue.enqueue(packet);
            break;
          } else {
            cnt.input_packet_queue.enqueue(packet);
            cnt.connected_nodes.erase(c_data->fd);
            epoll_ctl(receive_epfd, EPOLL_CTL_DEL, c_data->fd, NULL);
            if (close(c_data->fd) != 0) {
              OUT_ERROR << "Error closing backup node socket" << endl;
              continue;
            }
            delete c_data;
          }
        } else {
          unsigned int buffer_used;
          while (true) {
            buffer_used = 0;
            conn_packet *new_packet = cnt.data_parser->deserialize(
                c_data->buffer, c_data->buffer_size, &buffer_used);

            if (new_packet == nullptr) {
              DEBUG_LOG << "Package incomplete" << endl;
              break;
            } else {
              new_packet->fd = c_data->fd;
#ifdef DEBUG_CONNECTION
              DEBUG_LOG << "\n\theader_1: 0x" << std::hex
                        << new_packet->header_1 << endl
                        << "\theader_2: 0x" << std::hex << new_packet->header_2
                        << endl
                        << "\ttype: 0x" << std::hex << new_packet->type << endl
                        << "\tlen: " << std::dec << new_packet->pkt_len << endl;
#endif
              cnt.input_packet_queue.enqueue(new_packet);
            }
#ifdef DEBUG_CONNECTION
            DEBUG_LOG << ">>Buffer size " << c_data->buffer_size << endl;
            DEBUG_LOG << ">>Buffer used " << buffer_used << endl;
#endif
            if ((MAX_PACKET_SIZE - c_data->buffer_size) >= buffer_used) {
              c_data->buffer_size -= buffer_used;
              memmove(c_data->buffer, c_data->buffer + buffer_used,
                      c_data->buffer_size);
            }
            if (c_data->buffer_size < 5)
              break;
          }
        }
      } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
        connection_event_data *c_data =
            (connection_event_data *) events[i].data.ptr;
        DEBUG_LOG << "peer disconected, fd " << c_data->fd << endl;
        conn_packet *packet = new conn_packet();
        packet->type = packet_type::DISCONNECT;
        packet->fd = c_data->fd;
        if (!cnt.is_server) {
          close(listen_fd);
          cnt.is_running = false;
          cnt.input_packet_queue.enqueue(packet);
          break;
        } else {
          cnt.input_packet_queue.enqueue(packet);
          cnt.connected_nodes.erase(c_data->fd);
          epoll_ctl(receive_epfd, EPOLL_CTL_DEL, c_data->fd, NULL);
          if (close(c_data->fd) != 0) {
            OUT_ERROR << "Error closing backup node socket" << endl;
            continue;
          }
        }

      } else if (events[i].events & EPOLLERR) {
        DEBUG_LOG << "Epoll error" << endl;
      }
    }
  }
  DEBUG_LOG << "Closing listen task" << endl;
  close(receive_epfd);
  close(listen_fd);
  cnt.is_running = false;
}

void connection::send_task(connection &cnt) {}
int connection::set_socket_options(int fd) {
  int one = 1;

  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int));
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(int));
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

  int flags, s;
  flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    std::string error = "Error setting socket non-blocking: fcntl:";
    error += std::strerror(errno);
    DEBUG_LOG << error << std::endl;
    to_syslog(LOG_ERR, error.c_str());
    return -1;
  }

  flags |= O_NONBLOCK;
  s = fcntl(fd, F_SETFL, flags);
  if (s == -1) {
    std::string error = "Error setting socket non-blocking: fcntl:";
    error += std::strerror(errno);
    DEBUG_LOG << error << std::endl;
    to_syslog(LOG_ERR, error.c_str());
    return -1;
  }

  return 0;
}
bool connection::check_socket(int fd) {
  int ret;
  int code;
  socklen_t len = sizeof(int);

  ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &code, &len);

  if ((ret || code) != 0)
    return true;

  return false;
}

bool connection::send_packet(conn_packet *pkt) {
  if (is_server && connected_nodes.size() == 0)
    return false;
  if (!is_running)
    return false;
  if (!is_server)
    pkt->fd = listen_fd;
  char *to_send = new char[MAX_PACKET_SIZE];
  unsigned int to_send_size_;
  if (this->data_parser->serialize(pkt, to_send, &to_send_size_)) {
#ifdef DEBUG_CONNECTION
    DEBUG_LOG << "\theader_1: 0x" << std::hex << pkt->header_1 << endl
              << "\theader_2: 0x" << std::hex << pkt->header_2 << endl
              << "\ttype: 0x" << std::hex << pkt->type << endl
              << "\tlen: " << std::dec << pkt->pkt_len << endl;
#endif
    std::lock_guard<std::mutex> scope_lock(send_mutex);
    // Send to all client ?
    if (is_server && pkt->fd == -1) {
      for (auto &client_fd : this->connected_nodes) {
        if (!handle_send(client_fd.first, to_send, to_send_size_)) {
          std::string err_ = "handle send error to fd: ";
          err_ += std::to_string(client_fd.first);
          OUT_ERROR << err_ << endl;
          delete[] to_send;
          return false;
        }
      }
    } else {
      int fd = pkt->fd;
      if (!handle_send(fd, to_send, to_send_size_)) {
        std::string err_ = "Send packet error to fd: ";
        err_ += is_server ? pkt->fd : listen_fd;
        OUT_ERROR << err_ << endl;
        delete[] to_send;
        return false;
      }
    }
  } else {
    OUT_ERROR << "Error serializing packet to " << pkt->fd
              #ifdef DEBUG_CONNECTION
              << endl
              << "\theader_1: 0x" << std::hex << pkt->header_1 << endl
              << "\theader_2: 0x" << std::hex << pkt->header_2 << endl
              << "\ttype: 0x" << std::hex << pkt->type << endl
              << "\tlen: " << std::dec << pkt->pkt_len
              #endif
              << endl;
  }
  delete[] to_send;
  return true;
}

// Send now, if not enqueue buffer
bool connection::handle_send(int fd, char *data, int data_size) {
  int ret = 0;
  int sent = 0;
  while (true) {
    ret = send(fd, data + sent, data_size - sent, MSG_NOSIGNAL);
    if ((ret == -1) && (errno == EWOULDBLOCK || errno == EAGAIN)) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(10)); // TODO::Enqueue to send stream buffer
      continue;
    }
    if (ret == (data_size - sent)) {
      break;
    } else if (ret == -1) {
      std::string err_ = "send() failed: ";
      err_ += fd;
      err_ += std::strerror(errno);
      DEBUG_LOG << err_ << endl;
      return false;
    }

    sent += ret;
  }
  return true;
}
connection::~connection() { stop(); }
conn_packet *connection::get_packet(bool wait) {
  if (wait)
    return input_packet_queue.dequeue();
  else
    return input_packet_queue.try_dequeue();
}

