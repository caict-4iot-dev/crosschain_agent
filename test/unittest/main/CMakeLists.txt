# unittest main module CmakeLists.txt -- unittest

set(APP_UNITTEST unittest)

set(APP_UNITTEST_SRC
    main.cpp
    ${AGENT_SRC_DIR}/main/configure.cpp
)

# Set unittest module dir
aux_source_directory(${UNITTEST_SRC_DIR}/common UNITTEST_COMMON_SRC)


set(INNER_LIBS agent_webserver agent_websocket agent_handlemsg agent_common agent_utils bifchain_ed25519 agent_database agent_proto)


# Generate executable files
add_executable(${APP_UNITTEST}
    ${UNITTEST_COMMON_SRC}
)


target_link_libraries(${APP_UNITTEST}
    -Wl,-dn ${INNER_LIBS} ${AGENT_DEPENDS_LIBS} ${AGENT_LINKER_FLAGS})

# Specify compiling options for target objets
target_compile_options(${APP_UNITTEST}
    PUBLIC -std=c++1z
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${APP_UNITTEST}
    RUNTIME DESTINATION ${AGENTNODE}/bin
)