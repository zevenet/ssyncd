###############################################################################
#
#    Zevenet Software License
#    This file is part of the Zevenet Load Balancer software package.
#
#    Copyright (C) 2017-today ZEVENET SL, Sevilla (Spain)
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as
#    published by the Free Software Foundation, either version 3 of the
#    License, or any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

cmake_minimum_required(VERSION 2.8)

project(ssyncd)

SET(APPLICATION_DESC "Session syncronization for zevenet load balancer")
SET(APPLICATION_CODENAME "${PROJECT_NAME}")
SET(APPLICATION_COPYRIGHT_YEARS "2017")
SET(APPLICATION_VERSION_MAJOR 1)
SET(APPLICATION_VERSION_MINOR 0)
SET(APPLICATION_VERSION_PATCH 0)
SET(APPLICATION_VENDOR_NAME "zevenet")
SET(APPLICATION_VENDOR_URL "https://www.zevenet.com")
SET(BINARY_TAIL "${APPLICATION_VENDOR_NAME}_${APPLICATION_VERSION_MAJOR}_${APPLICATION_VERSION_MINOR}_${APPLICATION_VERSION_PATCH}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst  ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE  "-O3")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_BACKUP_CON")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_MASTER_CON")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_HTTP_CON")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_CONNECTION")

SET(MAINFOLDER ${PROJECT_SOURCE_DIR})
SET(EXECUTABLE_OUTPUT_PATH "${MAINFOLDER}/bin")

FIND_PATH(SYSTEM_RE2_INCLUDE_DIR re2/re2.h)
IF (SYSTEM_RE2_INCLUDE_DIR)
    MESSAGE(STATUS "Found RE2 include dir")
    FIND_LIBRARY(SYSTEM_RE2_LIBRARY re2)
    IF (SYSTEM_RE2_LIBRARY)
        MESSAGE(STATUS "Found RE2 library")
        SET(RE2_INCLUDE_DIR ${SYSTEM_RE2_INCLUDE_DIR})
        SET(RE2_LIBRARY ${SYSTEM_RE2_LIBRARY})
    ELSE ()
        MESSAGE(FATAL_ERROR "Re2 library not found")
    ENDIF ()
ELSE ()
    MESSAGE(STATUS "Did not find system RE2")
ENDIF ()

find_package(Threads)

set(sources_ctl
        src/ctl_interface/main.cpp
        src/connection/conn_packet.h
        src/connection/packet_parser.cpp
        src/connection/packet_parser.h
        src/ctl_interface/command_pkt.cpp
        src/ctl_interface/ctl_controller.h
        src/ctl_interface/command_pkt.h
        src/ctl_interface/ctl_controller.cpp
        )

set(sources
        src/main.cpp
        src/connection/connection.cpp
        src/connection/packet_parser.cpp
        src/xt_recent/xt_recent_action.cpp
        src/xt_recent/xt_recent_entry.cpp
        src/xt_recent/xt_recent_table.cpp
        src/xt_recent/xt_recent_proc.cpp
        src/sync/base_sync.cpp
        src/sync/pkt_parser.cpp
        src/sync/sync_master.cpp
        src/sync/sync_backup.cpp
        src/sync/sync_controller.cpp
        src/ctl_interface/command_pkt.cpp
        src/tests/basic_test.cpp
        src/http/http_parser.cpp
        src/http/http_sync.cpp
        src/xt_recent/xt_recent_sync.cpp
        src/xt_recent/xt_recent_sync.h)

set(headers
        src/connection/conn_packet.h
        src/connection/connection.h
        src/debug/errors.h
        src/helpers/safe_queue.h
        src/helpers/utils.h
        src/connection/packet_parser.h
        src/xt_recent/xt_recent_action.h
        src/xt_recent/xt_recent_entry.h
        src/xt_recent/xt_recent_table.h
        src/xt_recent/xt_recent_proc.h
        src/xt_recent/xt_recent_report.h
        src/xt_recent/xt_recent_parser.h
        src/xt_recent/xt_recent_parser.cpp
        src/sync/pkt_parser.h
        src/sync/base_sync.h
        src/sync/sync_master.h
        src/sync/sync_backup.h
        src/sync/sync_controller.h
        src/ctl_interface/command_pkt.h
        src/tests/basic_test.h
        src/http/http_parser.h
        src/http/http_sync.h
        src/http/http.h
        )

add_executable("${PROJECT_NAME}_${BINARY_TAIL}" ${sources} ${headers})
add_executable("${PROJECT_NAME}ctl_${BINARY_TAIL}" ${sources_ctl} ${ALL_SRC_HEADER_FILES})

target_link_libraries("${PROJECT_NAME}ctl_${BINARY_TAIL}" ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries("${PROJECT_NAME}_${BINARY_TAIL}" ${CMAKE_THREAD_LIBS_INIT} ${RE2_LIBRARY})

