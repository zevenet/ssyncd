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

#if __cplusplus <= 199711L
#error ssyncd needs at least a C++11 compliant compiler
#endif

#include "connection/connection.h"
#include "debug/errors.h"
#include "sync/pkt_parser.h"
#include "sync/sync_backup.h"
#include "sync/sync_controller.h"
#include "sync/sync_master.h"
#include "tests/basic_test.h"
#include "xt_recent/xt_recent_action.h"
#include "xt_recent/xt_recent_proc.h"
#include <chrono>
#include <cstring>
#include <execinfo.h>
#include <getopt.h>
#include <iostream>
#include <signal.h>   //signal(3)
#include <sys/stat.h> //umask(3)
#include <syslog.h>   //syslog(3), openlog(3), closelog(3)
#include <thread>
#include <unistd.h>
using namespace std;

void handler(int sig) {
  void *array[100];
  char **strings;
  int j, nptrs;

  nptrs = backtrace(array, 10);
  syslog(LOG_ERR, "Error: signal %d:\n", sig);
  strings = backtrace_symbols(array, nptrs);
  if (strings == NULL) {
    to_syslog(LOG_ERR, "backtrace_symbols");
    exit(EXIT_FAILURE);
  }

  for (j = 0; j < nptrs; j++)
    syslog(LOG_ERR, "backtrace_symbols: %s", strings[j]);
  free(strings);
  exit(EXIT_FAILURE);
}
int daemonize(string name, string path, string outfile, string errfile,
              string infile) {
  if (path.empty()) {
    path = "/";
  }
  if (name.empty()) {
    name = "ssyncd";
  }
  if (infile.empty()) {
    infile = "/dev/null";
  }
  if (outfile.empty()) {
    outfile = "/dev/null";
  }
  if (errfile.empty()) {
    errfile = "/dev/null";
  }

  pid_t child;

  if ((child = fork()) < 0) {
    fprintf(stderr, "error: failed fork\n");
    exit(EXIT_FAILURE);
  }
  if (child > 0) { // parent
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    exit(EXIT_SUCCESS);
  }
  if (setsid() < 0) { // failed to become session leader
    fprintf(stderr, "error: failed setsid\n");
    exit(EXIT_FAILURE);
  }

  // catch/ignore signals
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  // fork second time
  if ((child = fork()) < 0) { // failed fork
    fprintf(stderr, "error: failed fork\n");
    exit(EXIT_FAILURE);
  }
  if (child > 0) {
    exit(EXIT_SUCCESS);
  }

  // new file permissions
  umask(0);
  // change to path directory
  chdir(path.c_str());

  // Close all open file descriptors
  int fd;
  for (fd = sysconf(_SC_OPEN_MAX); fd > 0; --fd) {
    close(fd);
  }

  // reopen stdin, stdout, stderr
  stdin = fopen(infile.c_str(), "r");
  stdout = fopen(outfile.c_str(), "w+");
  stderr = fopen(errfile.c_str(), "w+");

  // open syslog
  // openlog(name.c_str(), LOG_PID, LOG_DAEMON);
  return (0);
}

void show_help() {

  string help = "Zevenet Software \n"
      "\n"
      "Usage: ssyncd -[MB] [adp]\n"
      "    \n"
      "Examples\n"
      "    ssyncd -M [-p 7777]\n"
      "    ssyncd -B [-a 127.0.0.1 -p 7777]\n"
      "\n"
      "Commands: Either long or short options are allowed.\n"
      "    -M  --master                        start master node\n"
      "    -B  --backup                        start backup node\n"
      "\n"
      "Options:\n"
      "    -d  --daemon                        run ssyncd as daemon\n"
      "    -a  --address   [master address]    master node address\n"
      "    -p  --port      [master port]       master listening port";

  cout << help << endl;
}

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"master", no_argument, 0, 'M'},
    {"backup", no_argument, 0, 'B'},
    {"address", required_argument, 0, 'a'},
    {"daemon", required_argument, 0, 'd'},
    {"port", required_argument, 0, 'p'},
    {"test", required_argument, 0, 't'},
    {"local-test", required_argument, 0, 'l'},
    {0, 0, 0, 0}};

enum run_command { NONE, MASTER, BACKUP, TEST };
std::string xt_recent_proc::xt_recent_dir = "/proc/net/xt_recent/";

int main(int argc, char *argv[]) {
  signal(SIGSEGV, handler);
  int c;
  int server_port = 7777;
  std::string server_address = "";
  run_command start_mode = NONE;
  int option_index = 0;
  int test_id = 0;
  bool daemon = false;
  while (true) {
    c = getopt_long(argc, argv, "MBdt:ha:p:", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
      case 'd':daemon = true;
        break;
      case 'M':start_mode = MASTER;
        break;
      case 'B':start_mode = BACKUP;
        break;
      case 'a':server_address = optarg;
        break;
      case 'p':server_port = atoi(optarg);
        break;
      case 't':start_mode = TEST;
        test_id = atoi(optarg);
        if (test_id > 2 || test_id < 1) {
          cout << "Test not found" << endl;
          show_help();
          return EXIT_FAILURE;
        }
        break;
      case 'h':show_help();
        return EXIT_SUCCESS;
      default:show_help();
        return EXIT_FAILURE;
    }
  }
  if (daemon) {
    if (daemonize("ssyncd", "/", "", "", "") != 0) {
      fprintf(stderr, "error: daemonize failed\n");
      exit(EXIT_FAILURE);
    }
  }
  if (start_mode == NONE) {
    show_help();
    exit(EXIT_FAILURE);
  }
  xt_recent_proc::xt_recent_dir = "/proc/net/xt_recent/";
  sync_controller ctl_control;
  ctl_control.server_port = server_port;
  ctl_control.server_address = server_address;
  ctl_control.init_ctl_interface();
  switch (start_mode) {
    case MASTER: {
      // setlogmask(LOG_UPTO(LOG_NOTICE));
      openlog("ssyncd::master", LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY,
              LOG_DAEMON);
      ctl_control.set_worker_mode(controller::MASTER);
      ctl_control.start();

      while (ctl_control.is_running)
        std::this_thread::sleep_for(std::chrono::seconds(5));
      break;
    }
    case BACKUP: {
      if (server_address == "") {
        OUT_ERROR << "Need to specify master ip with option -a" << endl;
        exit(EXIT_FAILURE);
      }
      // setlogmask(LOG_UPTO(LOG_NOTICE));
      openlog("ssyncd::backup", LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY,
              LOG_DAEMON);
      ctl_control.set_worker_mode(controller::BACKUP);
      ctl_control.start();

      while (ctl_control.is_running)
        std::this_thread::sleep_for(std::chrono::seconds(5));
      break;
    }
    case TEST: {
      xt_recent_proc::xt_recent_dir = "./backup_proc/";
      xt_recent_proc::xt_recent_dir = "./master_proc/";
      basic_test tests;
      tests.test_xt_action_serialization();
      switch (test_id) {
        case 1:tests.run_master(server_port);
          break;
        case 2:tests.run_backup(server_address, server_port);
          break;
      }
      break;
    }
  }
  return EXIT_SUCCESS;
}
