#agent common module CmakeLists.txt -- libagent_webserver.a
cmake_minimum_required(VERSION 3.22)
set(LIB_AGENT_WEBSERVER agent_webserver)
aux_source_directory(${AGENT_SRC_DIR}/webserver/ WEBSERVER_SRC)

#Generate static library files
add_library(${LIB_AGENT_WEBSERVER} ${WEBSERVER_SRC}) 
add_dependencies(${LIB_AGENT_WEBSERVER} generate_proto)

#Specify compiling options for target objects
target_compile_options(${LIB_AGENT_WEBSERVER}
    PUBLIC -std=c++1z 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
	PUBLIC -fPIC
)
