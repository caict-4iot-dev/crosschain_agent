#agent daemon module CmakeLists.txt

set(DEAMON_DEPENDS_LIB ssl crypto)

# -- agent_agentd -- #
set(LAYER1_AGENT_ID crossd)

add_executable(${LAYER1_AGENT_ID} ./src/main.cpp)

MESSAGE(STATUS "current platform: Linux ")  
target_link_libraries(${LAYER1_AGENT_ID} -Wl,-dn ${DEAMON_DEPENDS_LIB} ${AGENT_LINKER_FLAGS})

target_compile_options(${LAYER1_AGENT_ID}
    PUBLIC -std=c++1z
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${LAYER1_AGENT_ID}
    RUNTIME DESTINATION ${AGENTNODE}/bin
)

