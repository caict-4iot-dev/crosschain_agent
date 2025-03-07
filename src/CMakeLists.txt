cmake_minimum_required(VERSION 3.22)

project(layer1-agent)
add_compile_options(-g -O2)
add_definitions(-DSVNVERSION=\"${SVNVERSION}\")
message(STATUS "-DSVNVERSION="${SVNVERSION})

MESSAGE(STATUS "current platform: Linux ")  
set( OS_NAME "OS_LINUX")
set(AGENT_LINKER_FLAGS "-Wl,-dy -lpthread -fno-exceptions -lrt -ldl")

set(AGENTNODE layer1-agent)
set(AGENT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(AGENT_ROOT_DIR ${AGENT_SRC_DIR}/..)
set(LIBRARY_OUTPUT_PATH ${AGENT_ROOT_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${AGENT_ROOT_DIR}/bin)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

set(COMMON_COMPILE_FLAGS ENABLE_MODULE_RECOVERY ENABLE_MODULE_ECDH ENABLE_MODULE_RANGEPROOF ENABLE_MODULE_BULLETPROOF ENABLE_MODULE_GENERATOR USE_FIELD_INV_BUILTIN USE_NUM_NONE USE_SCALAR_INV_BUILTIN)
set(COMPILE_FLAGS USE_FIELD_10X26 USE_SCALAR_8X32 HAVE_BUILTIN_EXPECT)

set(AGENT_3RD_LIB /usr/local/bif_3rd)
if(EXISTS "${AGENT_3RD_LIB}")
    message("AGENT_3RD_LIB path="${AGENT_3RD_LIB})
else()
    message(FATAL_ERROR "AGENT_3RD_LIB Not Exist, Please Check ! path="${AGENT_3RD_LIB})
endif()

# 处理proto文件编译
include(${AGENT_SRC_DIR}/proto/proto.cmake)

include_directories(
    ${AGENT_SRC_DIR}
    ${AGENT_3RD_LIB}/include
    ${AGENT_3RD_LIB}/include/http
)

link_directories(
    ${AGENT_3RD_LIB}/lib
    ${AGENT_3RD_LIB}/lib64
    ${LIBRARY_MODULES_PATH}
    ${LIBRARY_OUTPUT_PATH}
)

set(AGENT_DEPENDS_LIBS rocksdb pcreposix pcrecpp pcre json restclient-cpp tbb z bz2 scrypt sodium boost_regex boost_filesystem  protobuf gpr curl ssl crypto cpp-httplib pistache)

add_subdirectory(glue)
add_subdirectory(utils)
add_subdirectory(proto)
add_subdirectory(common)
add_subdirectory(main)
add_subdirectory(database)
add_subdirectory(daemon)
add_subdirectory(websocket)
add_subdirectory(dbtool)
add_subdirectory(webserver)

