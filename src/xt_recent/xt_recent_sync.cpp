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

#include "xt_recent_sync.h"
#include "../debug/errors.h"
#include <cstring>
#include <dirent.h>

xt_recent_table
xt_recent_sync::get_table_updates(const std::string table_name) {
  xt_recent_table xtable = xt_recent_proc::get_table(table_name);
  for (auto entry : xtable.entries) {
    if (recent_data[xtable.name].entries.count(entry.first)) {
      auto old_entry = recent_data[xtable.name].entries[entry.first];
      // check for entry update
      if (old_entry.get_oldest_pkt_index() !=
          entry.second.get_oldest_pkt_index() /*||
      old_entry.timestamps[old_entry.get_oldest_pkt_index()] !=
          entry.second.timestamps[old_entry.get_oldest_pkt_index()]*/
      ) {
        auto xaction = get_entry_as_action(table_name, &(entry.second));
        input_data.enqueue(xaction);
      }
    } else {
      auto xaction = get_entry_as_action(table_name, &(entry.second));
      input_data.enqueue(xaction);
    }
  }
  // search for removed entries
  for (auto entry : recent_data[xtable.name].entries) {
    if (xtable.entries.count(entry.first) == 0) {
      auto xaction = get_entry_as_action(table_name, &(entry.second), true);
      input_data.enqueue(xaction);
    }
  }
  return xtable;
}

void xt_recent_sync::proc_task(xt_recent_sync &obj) {
  while (obj.is_running) {
    std::shared_ptr<DIR> directory_ptr(
        opendir(xt_recent_proc::xt_recent_dir.c_str()),
        [](DIR *dir) { dir &&closedir(dir); });
    struct dirent *dirent_ptr;
    if (!directory_ptr) {
      OUT_ERROR << "Error opening : " << std::strerror(errno)
                << xt_recent_proc::xt_recent_dir << std::endl;
      to_syslog(LOG_EMERG, "Error opening /proc/net/xt_recent/ directory:");
      exit(EXIT_FAILURE);
      // return tables;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(obj.update_time_sec));
    while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr) {
      if (dirent_ptr->d_type == DT_REG) {
        obj.recent_data[std::string(dirent_ptr->d_name)] =
            obj.get_table_updates(std::string(dirent_ptr->d_name));
      }
    }
  }
}

void xt_recent_sync::set_update_time(int milliseconds) {
  this->update_time_sec = milliseconds;
}

void xt_recent_sync::start() {
  this->is_running = true;
  this->update_thread = thread(proc_task, std::ref(*this));
  this->update_thread.detach();
}

void xt_recent_sync::stop() { is_running = false; }

xt_recent_action *xt_recent_sync::get_entry_as_action(std::string target_table,
                                                      xt_recent_entry *entry,
                                                      bool delete_action) {
  xt_recent_action *xaction =
      new xt_recent_action(delete_action ? DELETE_ENTRY : ADD_UPD_ENTRY);
  xaction->xt_table_name = target_table;
  xaction->target_entry = *entry;
  xaction->pkt_len = 0;
  return xaction;
}

#if TESTS
void xt_recent_sync::fake_update(int entries) {
  for (auto table : master_tables) {
    std::ofstream new_data(xt_recent_proc::xt_recent_dir + table.first,
                           std::ios_base::trunc);
    std::string data =
        "src=192.168.0.194 ttl: 63 last_seen: 4705397993 oldest_pkt: 1 "
        "4705397993";
    int ip_sufix = 80;
    unsigned long ini_timestamp = 4705300000;

    std::string oldest_pkt;
    unsigned long lastseen;
    srand(time(NULL));
    // int entries_max = 4 + rand() % (entries - 4);
    for (int i = 0; i < entries; i++) {
      std::string entry = "src=192.168.0.";
      entry += std::to_string(ip_sufix + i) + " ttl: 63 last_seen: ";
      lastseen = 0;
      int index = 4 + rand() % (32 - 4);
      oldest_pkt = " oldest_pkt: " + std::to_string(index);
      for (int j = 1; j <= index; j++) {
        lastseen = ini_timestamp + j * index;
        oldest_pkt += ' ' + std::to_string(lastseen);
        if (j != index)
          oldest_pkt += ',';
      }
      entry += std::to_string(lastseen) + oldest_pkt;
      new_data << entry << std::endl;
      // std::cout << entry << std::endl;
    }
    new_data.close();
  }
}
#endif
