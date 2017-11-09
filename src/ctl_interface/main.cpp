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

#include "../debug/errors.h"
#include "command_pkt.h"
#include "ctl_controller.h"
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int main(int argc, char *argv[]) {
  string option;
  command_pkt pkt;
  pkt.command = VOID;
  for (int i = 1; i < argc; i++) {
    option = argv[i];
    if (option.compare("help") == 0) {
      // show help
      exit(EXIT_SUCCESS);
    }
    if (option.compare("write") == 0) {
      if (argv[++i]) {
        string value = argv[i];
        if (value.compare("http") == 0) {
          pkt.command = WRITE_HTTP;
        } else if (value.compare("recent") == 0) {
          pkt.command = WRITE_RECENT;
        } else {
          OUT_ERROR << "unknown service: " << value << std::endl;
        }
      } else {
        OUT_ERROR << "target to start not valid" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    if (option.compare("clear") == 0) {
      if (argv[++i]) {
        string value = argv[i];
        if (value.compare("http") == 0) {
          pkt.command = CLEAR_HTTP;
        } else if (value.compare("recent") == 0) {
          pkt.command = CLEAR_RECENT;
        } else {
          OUT_ERROR << "unknown service: " << value << std::endl;
        }
      } else {
        OUT_ERROR << "service not valid" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    if (option.compare("quit") == 0) {
      pkt.command = QUIT;
    }
    if (option.compare("start") == 0) {
      if (argv[++i]) {
        string value = argv[i];
        if (value.compare("http") == 0) {
          pkt.command = START_HTTP_SYNC;
          if (argv[++i]) {
            string value = argv[i];
            pkt.command_data = value;
            // TODO::check if farms exists!!
            // if( "tmp/ssync_" +value ".socket" )
          } else {
            OUT_ERROR << "no farm specified" << std::endl;
            exit(EXIT_FAILURE);
          }
        } else if (value.compare("recent") == 0) {
          pkt.command = START_RECENT_SYNC;
        }
      } else {
        OUT_ERROR << "target to start not valid" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    if (option.compare("stop") == 0) {
      if (argv[++i]) {
        string value = argv[i];
        if (value.compare("http") == 0) {
          pkt.command = STOP_HTTP_SYNC;
          if (argv[++i]) {
            string value = argv[i];
            pkt.command_data = value;
            // TODO::check if farms exists!!
            // if( "tmp/ssync_" +value ".socket" )
          } else {
            OUT_ERROR << "no farm specified" << std::endl;
            exit(EXIT_FAILURE);
          }
        } else if (value.compare("recent") == 0) {
          pkt.command = STOP_RECENT_SYNC;
        } else {
          OUT_ERROR << "unknown service: " << value << std::endl;
        }
      } else {
        OUT_ERROR << "service to stop not valid" << std::endl;
        exit(EXIT_FAILURE);
      }
    }

    if (option.compare("table") == 0) {
      if (argv[++i]) {
        pkt.command_data = argv[i];
        if (pkt.command_data.compare("*") == 0) {
          switch (pkt.command) {
            case WRITE_RECENT:pkt.command = WRITE_RECENT_TABLE;
              break;
            default:OUT_ERROR << "command error" << std::endl;
              exit(EXIT_FAILURE);
          }
        }
      } else {
        OUT_ERROR << "table name not specified" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    if (option.compare("count") == 0) {
      if (argv[++i]) {
        string value = argv[i];
        if (value.compare("http") == 0) {
          pkt.command = COUNT_HTTP;
        } else if (value.compare("recent") == 0) {
          pkt.command = COUNT_RECENT;
        } else {
          OUT_ERROR << "unknown service: " << value << std::endl;
        }
      } else {
        OUT_ERROR << "service not valid" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    if (option.compare("show") == 0) {
      if (argv[++i]) {
        string value = argv[i];
        if (value.compare("http") == 0) {
          pkt.command = SHOW_HTTP;
        } else if (value.compare("recent") == 0) {
          pkt.command = SHOW_RECENT;
        } else if (value.compare("mode") == 0) {
          pkt.command = SHOW_MODE;
        } else {
          OUT_ERROR << "unknown service: " << value << std::endl;
        }
      } else {
        OUT_ERROR << "service not valid" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    if (option.compare("port") == 0) {
      if (argv[++i]) {
        try {
          pkt.server_port = std::stoi(argv[i]);

        } catch (std::exception ex) {
          OUT_ERROR << " port number not valid" << std::endl;
          exit(EXIT_FAILURE);
        }
      }
    }
    if (option.compare("address") == 0) {
      if (argv[++i]) {
        try {
          pkt.server_address = argv[i];

        } catch (std::exception ex) {
          OUT_ERROR << " address not valid" << std::endl;
          exit(EXIT_FAILURE);
        }
      }
    }
    if (option.compare("switch") == 0) {
      if (argv[++i]) {
        string value = argv[i];
        if (value.compare("master") == 0) {
          pkt.command = SW_TO_MASTER;
        } else if (value.compare("backup") == 0) {
          pkt.command = SW_TO_CLIENT;
        }
      } else {
        OUT_ERROR << "mode not valid" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }
  if (pkt.command == VOID) {
    cout << "Command not valid:" << endl;
    return EXIT_FAILURE;
  }
  ctl_controller controller;
  controller.init();

  controller.send_command(&pkt);
  std::string result = "";
  while (controller.is_running)
    result += controller.get_response();
  if (!result.empty())
    cout << result << endl;
  exit(EXIT_SUCCESS);
}
