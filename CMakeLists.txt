cmake_minimum_required(VERSION 3.22)

project(agnet-core)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    set(CUR_OS "linux")
else()
	message(FATAL_ERROR "current platform: unkonw ")
endif()

set(CUR_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(AGENT_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/${CUR_OS})
set(AGENT_UNITTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/unittest)
file(MAKE_DIRECTORY ${AGENT_BUILD_DIR})
file(MAKE_DIRECTORY ${AGENT_UNITTEST_DIR})

execute_process(COMMAND git log -1 --pretty=format:%H
                OUTPUT_VARIABLE GIT_VERSION
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(BUILD_ALL
    ALL
	COMMAND cmake -DSVNVERSION=${GIT_VERSION} -DCMAKE_VERBOSE_MAKEFILE=ON ../../src && make -j 16
    WORKING_DIRECTORY ${AGENT_BUILD_DIR}
    COMMENT "begin build all..."
    )

add_custom_target(test
	COMMAND cmake -DUNITTEST=ON -DSVNVERSION=${GIT_VERSION} -DCMAKE_VERBOSE_MAKEFILE=ON ../../test/unittest && make -j 16
    WORKING_DIRECTORY ${AGENT_UNITTEST_DIR}
    COMMENT "begin build test..."
    )
 
add_custom_target(clean_build
	COMMAND ls | grep -v one_step_pack.sh | grep -v README.md | xargs rm -rf
    WORKING_DIRECTORY ${AGENT_BUILD_DIR}
    COMMENT "begin build clean_cur_build..."
    )

add_custom_target(clean_test
	COMMAND ls | grep -v README.md | xargs rm -rf
    WORKING_DIRECTORY ${AGENT_UNITTEST_DIR}
    COMMENT "begin build clean_cur_build..."
    )

add_custom_target(clean_all
    COMMAND rm -rf bin/agent && cd lib && ls | grep -v '*.so' | xargs rm -rf && cd ${CMAKE_CURRENT_BINARY_DIR} && echo "" > CMakeCache.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS clean_build clean_test
    COMMENT "begin build clean_all..."
    )

add_custom_target(docker
	COMMAND cp -r bin docker && cd docker && docker build -t agent .
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "begin build docker..."
    )

add_custom_target(fpm
	COMMAND sh `pwd`/../fpm/fpm_build.sh
    COMMENT "begin build fpm..."
    )
