#agent main module CmakeLists.txt -- agent

set(APP_AGENT cross)

set(APP_AGENT_SRC
    main.cpp
    configure.cpp
)

set(INNER_LIBS agent_webserver agent_websocket agent_handlemsg agent_common agent_utils bifchain_ed25519 agent_database agent_proto)

#Generate executable files
add_executable(${APP_AGENT} ${APP_AGENT_SRC})

#Specify dependent libraries for target objects
	
MESSAGE(STATUS "current platform: Linux ")  
target_link_libraries(${APP_AGENT} -Wl,-dn ${INNER_LIBS} ${AGENT_DEPENDS_LIBS} ${AGENT_LINKER_FLAGS}) #

#Specify compiling options for target objets
target_compile_options(${APP_AGENT}
    PUBLIC -std=c++1z 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${APP_AGENT}
    RUNTIME DESTINATION ${AGENTNODE}/bin
)
