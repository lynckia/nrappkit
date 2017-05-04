cmake_minimum_required(VERSION 2.6)

project(nrappkit)

set(NRAPPKIT_BUILD "${CMAKE_CURRENT_BINARY_DIR}")

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

set(NRAPPKIT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND nrappkit__c_srcs
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_assoc.c"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_crc32.c"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_data.c"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_errors.c"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_list.c"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_memory.c"
  #"${NRAPPKIT_SOURCE}/src/util/libekr/r_replace.c"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_time.c"
  "${NRAPPKIT_SOURCE}/src/util/libekr/debug.c"
  "${NRAPPKIT_SOURCE}/src/util/byteorder.c"
  "${NRAPPKIT_SOURCE}/src/util/hex.c"
  "${NRAPPKIT_SOURCE}/src/util/p_buf.c"
  "${NRAPPKIT_SOURCE}/src/util/util.c"
  "${NRAPPKIT_SOURCE}/src/log/r_log.c"
  "${NRAPPKIT_SOURCE}/src/event/async_timer.c"
  "${NRAPPKIT_SOURCE}/src/event/async_wait.c"
  "${NRAPPKIT_SOURCE}/src/registry/c2ru.c"
  "${NRAPPKIT_SOURCE}/src/registry/registry.c"
  "${NRAPPKIT_SOURCE}/src/registry/registry_local.c"
  "${NRAPPKIT_SOURCE}/src/registry/registrycb.c")

list(APPEND nrappkit__other_srcs
  "${NRAPPKIT_SOURCE}/src/share/nr_common.h"
  "${NRAPPKIT_SOURCE}/src/share/nr_reg_keys.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/assoc.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/debug.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_assoc.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_common.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_crc32.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_data.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_defaults.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_errors.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_includes.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_list.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_macros.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_memory.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_thread.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_time.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/r_types.h"
  "${NRAPPKIT_SOURCE}/src/util/libekr/debug.h"
  "${NRAPPKIT_SOURCE}/src/util/byteorder.h"
  "${NRAPPKIT_SOURCE}/src/util/hex.h"
  "${NRAPPKIT_SOURCE}/src/util/p_buf.h"
  "${NRAPPKIT_SOURCE}/src/util/util.h"
  "${NRAPPKIT_SOURCE}/src/event/async_timer.h"
  "${NRAPPKIT_SOURCE}/src/event/async_wait.h"
  "${NRAPPKIT_SOURCE}/src/event/async_wait_int.h"
  "${NRAPPKIT_SOURCE}/src/log/r_log.h"
  "${NRAPPKIT_SOURCE}/src/registry/c2ru.h"
  "${NRAPPKIT_SOURCE}/src/registry/registry.h"
  "${NRAPPKIT_SOURCE}/src/registry/registry_int.h"
  "${NRAPPKIT_SOURCE}/src/registry/registry_vtbl.h")

file(GLOB_RECURSE PORT_GENERIC_INCLUDE "${NRAPPKIT_SOURCE}/src/port/generic/include")

list(APPEND nrappkit__other_srcs ${PORT_GENERIC_INCLUDE})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND nrappkit__other_srcs "${NRAPPKIT_SOURCE}/src/port/darwin/include/csi_platform.h")
ELSE()
  list(APPEND nrappkit__other_srcs "${NRAPPKIT_SOURCE}/src/port/linux/include/csi_platform.h")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

add_library(nrappkit STATIC ${nrappkit__c_srcs} ${nrappkit__other_srcs})

set_target_properties(nrappkit PROPERTIES EXCLUDE_FROM_ALL "FALSE")

set_source_files_properties(${nrappkit__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")

set_target_properties(nrappkit PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "nrappkit")
set_target_properties(nrappkit PROPERTIES PREFIX "lib")
set_target_properties(nrappkit PROPERTIES ARCHIVE_OUTPUT_NAME "nrappkit")
set_target_properties(nrappkit PROPERTIES SUFFIX ".a")

set_source_files_properties(nrappkit/libnrappkit.a PROPERTIES GENERATED "TRUE")

list(APPEND nrappkit__include_dirs
  "${NRAPPKIT_SOURCE}/src/event"
  "${NRAPPKIT_SOURCE}/src/log"
  "${NRAPPKIT_SOURCE}/src/port/generic/include"
  "${NRAPPKIT_SOURCE}/src/registry"
  "${NRAPPKIT_SOURCE}/src/share"
  "${NRAPPKIT_SOURCE}/src/plugin"
  "${NRAPPKIT_SOURCE}/src/stats"
  "${NRAPPKIT_SOURCE}/src/util"
  "${NRAPPKIT_SOURCE}/src/util/libekr")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set (NRAPPKIT_OS DARWIN)
  list(APPEND nrappkit__include_dirs "${NRAPPKIT_SOURCE}/src/port/darwin/include")
ELSE()
  set (NRAPPKIT_OS LINUX)
  list(APPEND nrappkit__include_dirs "${NRAPPKIT_SOURCE}/src/port/linux/include")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set_target_properties(nrappkit PROPERTIES COMPILE_FLAGS "-g -fPIC -Wall -Wno-parentheses -Wno-strict-prototypes -Wmissing-prototypes -DNO_REG_RPC=1 -DHAVE_VFPRINTF=1 -DRETSIGTYPE=void -DNEW_STDIO -DHAVE_STRDUP=1 -DHAVE_STRLCPY=1 -DHAVE_LIBM=1 -DHAVE_SYS_TIME_H=1 -DTIME_WITH_SYS_TIME_H=1 -D${NRAPPKIT_OS} -D__UNUSED__=\"__attribute__((unused))\"")

set_property(TARGET nrappkit APPEND PROPERTY INCLUDE_DIRECTORIES ${nrappkit__include_dirs})


install(TARGETS nrappkit
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

list(APPEND nrappkit__include_public_dirs
  "${NRAPPKIT_SOURCE}/src/event"
  "${NRAPPKIT_SOURCE}/src/log"
  "${NRAPPKIT_SOURCE}/src/plugin"
  "${NRAPPKIT_SOURCE}/src/port"
  "${NRAPPKIT_SOURCE}/src/registry"
  "${NRAPPKIT_SOURCE}/src/share"
  "${NRAPPKIT_SOURCE}/src/stats"
  "${NRAPPKIT_SOURCE}/src/util")

target_include_directories(nrappkit PUBLIC ${nrappkit__include_public_dirs})

install(DIRECTORY ${nrappkit__include_public_dirs} DESTINATION include FILES_MATCHING PATTERN "**.h")
