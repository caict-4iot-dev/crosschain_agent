#agent proto module CmakeLists.txt -- libagent_proto.a
cmake_minimum_required(VERSION 3.22)

set(LIB_AGENT_PROTO agent_proto)

#Generate static library files
add_library(${LIB_AGENT_PROTO} ${PROTO_SRC})
add_dependencies(${LIB_AGENT_PROTO} generate_proto)

#Specify compiling options for target objects
target_compile_options(${LIB_AGENT_PROTO}
    PUBLIC -std=c++1z 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
)
