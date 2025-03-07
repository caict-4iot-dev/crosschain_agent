#agent database module CmakeLists.txt -- libagent_database.a
cmake_minimum_required(VERSION 3.22)
set(LIB_AGENT_DATABASE agent_database)

#Automatically get src files from the specified directory
aux_source_directory(${AGENT_SRC_DIR}/database/ DATABASE_SRC)

#Generate static library files
add_library(${LIB_AGENT_DATABASE} ${DATABASE_SRC}) 

#Specify compiling options for target objects
target_compile_options(${LIB_AGENT_DATABASE}
    PUBLIC -std=c++1z
	PRIVATE -frtti	
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
	PUBLIC -fPIC
)
