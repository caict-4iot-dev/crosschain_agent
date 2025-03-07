cmake_minimum_required(VERSION 3.22)

project(layer1-agent)
add_compile_options(-g -O2)
add_definitions(-DSVNVERSION=\"${SVNVERSION}\")

set( V8_LIB_PATH "linux")
set( OS_NAME "OS_LINUX")
set(AGENT_LINKER_FLAGS "-Wl,-dy -lpthread -lrt -ldl") 

set(AGENTNODE layer1-agent)
set(AGENT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
set(AGENT_ROOT_DIR ${AGENT_SRC_DIR}/..)
set(AGENT_3RD_LIB /usr/local/bif_3rd)
set(LIBRARY_OUTPUT_PATH ${AGENT_ROOT_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${AGENT_ROOT_DIR}/bin)

set(UNITTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../test/unittest)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

set(COMMON_COMPILE_FLAGS ENABLE_MODULE_RECOVERY ENABLE_MODULE_ECDH ENABLE_MODULE_RANGEPROOF ENABLE_MODULE_BULLETPROOF ENABLE_MODULE_GENERATOR USE_FIELD_INV_BUILTIN USE_NUM_NONE USE_SCALAR_INV_BUILTIN)
set(COMPILE_FLAGS USE_FIELD_10X26 USE_SCALAR_8X32 HAVE_BUILTIN_EXPECT)

include_directories(
    ${AGENT_SRC_DIR}
    ${AGENT_3RD_LIB}/include
    ${AGENT_3RD_LIB}/include/http
)

link_directories(
    ${AGENT_3RD_LIB}/lib
    ${AGENT_3RD_LIB}/lib64
    ${LIBRARY_OUTPUT_PATH}
)

set(AGENT_DEPENDS_LIBS rocksdb pcreposix pcrecpp pcre json restclient-cpp tbb z bz2 scrypt sodium boost_regex boost_filesystem nuraft protobuf gpr cares curl ssl crypto cpp-httplib gtest)



#add unittest dir
add_subdirectory(main)