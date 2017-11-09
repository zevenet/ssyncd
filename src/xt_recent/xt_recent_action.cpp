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

#include "xt_recent_action.h"
// save current timestamp
xt_recent_action::xt_recent_action() { this->pkt_len = 0; }

xt_recent_action::xt_recent_action(action_type type) {
  this->type = XT_RECENT_SYNC;
  this->action = type;
  this->pkt_len = 0;
}

xt_recent_action::xt_recent_action(conn_packet *pkt) {
  this->header_1 = pkt->header_1;
  this->header_2 = pkt->header_2;
  this->type = pkt->type;
  this->pkt_len = pkt->pkt_len;
}
