#agent utils module CmakeLists.txt -- libagent_utils.a

set(LIB_AGENT_UTILS agent_utils)
aux_source_directory(${AGENT_SRC_DIR}/utils/ UTILS_SRC)
#Generate static library files
add_library(${LIB_AGENT_UTILS} ${UTILS_SRC}) 

#Specify compiling options for target objects
target_compile_options(${LIB_AGENT_UTILS}
    PUBLIC -std=c++1z
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
	PUBLIC -fPIC
)
