#agent daemon module CmakeLists.txt

set(DBTOOL_DEPENDS_LIB rocksdb pcreposix pcrecpp pcre json restclient-cpp tbb z bz2 scrypt sodium boost_regex boost_filesystem  protobuf gpr curl ssl crypto cpp-httplib)

set(BIN_AGENT_ID dbtool)
set(INNER_LIBS agent_database agent_common agent_utils bifchain_ed25519  agent_proto)

add_executable(${BIN_AGENT_ID} ./main.cpp)

MESSAGE(STATUS "current platform: Linux ")  
target_link_libraries(${BIN_AGENT_ID} -Wl,-dn ${INNER_LIBS} ${DBTOOL_DEPENDS_LIB} ${AGENT_LINKER_FLAGS})

target_compile_options(${BIN_AGENT_ID}
    PUBLIC -std=c++1z
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${BIN_AGENT_ID}
    RUNTIME DESTINATION ${AGENTNODE}/bin
)

