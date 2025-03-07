#agent common module CmakeLists.txt -- libagent_common.a
cmake_minimum_required(VERSION 3.22)
set(LIB_AGENT_COMMON agent_common)
aux_source_directory(${AGENT_SRC_DIR}/common/ COMMON_SRC)

#Generate static library files
add_library(${LIB_AGENT_COMMON} ${COMMON_SRC}) 
add_dependencies(${LIB_AGENT_COMMON} generate_proto)

#Specify compiling options for target objects
target_compile_options(${LIB_AGENT_COMMON}
    PUBLIC -std=c++1z 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
	PUBLIC -fPIC
)
