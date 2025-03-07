cmake_minimum_required(VERSION 3.22)
set(LIB_AGENT_WEB agent_websocket)

#Automatically get src files from the specified directory
aux_source_directory(${AGENT_SRC_DIR}/websocket/ WEB_SRC)

#Generate static library files
add_library(${LIB_AGENT_WEB} ${WEB_SRC})
add_dependencies(${LIB_AGENT_WEB} generate_proto)

#Specify compiling options for target objects
target_compile_options(${LIB_AGENT_WEB}
    PUBLIC -std=c++1z 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
