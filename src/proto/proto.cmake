if(NOT PROTOC_BIN)
    find_program(PROTOC_BIN protoc-3.9.2.0 ${AGENT_3RD_LIB}/bin)
endif()

if(NOT PROTOC_BIN)
    message(FATAL_ERROR "protoc is not installed. Aborting...")
else()
    message(STATUS "protoc has been found: ${PROTOC_BIN}")
endif()



file(GLOB_RECURSE PROTOS ${AGENT_SRC_DIR}/proto/*.proto)

set(PROTO_SRC "")
set(PROTO_HDRS "")

foreach(proto ${PROTOS})
        get_filename_component(PROTO_FILE ${proto} NAME_WE)

        list(APPEND PROTO_SRC "${AGENT_SRC_DIR}/proto/cpp/${PROTO_FILE}.pb.cc")

        list(APPEND PROTO_HDRS "${AGENT_SRC_DIR}/proto/cpp/${PROTO_FILE}.pb.h")
        add_custom_command(
          OUTPUT "${AGENT_SRC_DIR}/proto/cpp/${PROTO_FILE}.pb.cc"
                 "${AGENT_SRC_DIR}/proto/cpp/${PROTO_FILE}.pb.h"
          COMMAND  ${PROTOC_BIN}  --cpp_out  ${AGENT_SRC_DIR}/proto/cpp/ -I${AGENT_SRC_DIR}/proto  ${proto}
          DEPENDS ${proto}
          COMMENT "Running C++ protocol buffer compiler on ${proto}"
          VERBATIM
        )
endforeach()

set_source_files_properties(${PROTO_SRC} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)

add_custom_target(generate_proto ALL
                DEPENDS ${PROTO_SRC} ${PROTO_HDRS}
                COMMENT "generate proto target"
                VERBATIM
)

