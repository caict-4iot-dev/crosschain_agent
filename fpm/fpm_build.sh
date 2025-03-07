#!/bin/bash

### BEGIN INIT INFO
# Provides:          bif
# Short-Description: fpm rpm build
# Description:       
#
#
### END INIT INFO

PACK_NAME=cross-agent
PACK_VERSION=1.0.0
PACK_RELEASE=0
PACK_TYPE=rpm
RPM_INSTALL_LINK=/usr/local/cross-agent
RPM_INSTALL_DIR=${RPM_INSTALL_LINK}-${PACK_VERSION}
AGENT_CURRENT_DIR=../fpm
RPM_MODULE_DIR=${AGENT_CURRENT_DIR}/module

#准备构建打包数据
prepare() {
    echo "fpm build prepare start..."
    #创建打包目录
    AGENT_FPM_DIR=${AGENT_CURRENT_DIR}/rpm
    if [ -d ${AGENT_FPM_DIR} ];
    then
        rm -rf ${AGENT_FPM_DIR}
    fi

    mkdir -p ${AGENT_FPM_DIR}
    mkdir -p ${AGENT_FPM_DIR}/SOURCE
    mkdir -p ${AGENT_FPM_DIR}/SHELL
    mkdir -p ${AGENT_FPM_DIR}/RPMS

    #创建打包数据
    AGENT_FPM_SOURCE_DIR=${AGENT_FPM_DIR}/SOURCE
    mkdir ${AGENT_FPM_SOURCE_DIR}/bin
    mkdir ${AGENT_FPM_SOURCE_DIR}/config
    mkdir ${AGENT_FPM_SOURCE_DIR}/data
    mkdir ${AGENT_FPM_SOURCE_DIR}/scripts
    mkdir ${AGENT_FPM_SOURCE_DIR}/coredump

    AGENT_ROOT_DIR=${AGENT_CURRENT_DIR}/..
    #bin
    cp -rf  ${RPM_MODULE_DIR}/bin/*.bin ${AGENT_FPM_SOURCE_DIR}/bin
    cp -rf  ${AGENT_ROOT_DIR}/bin/cross ${AGENT_FPM_SOURCE_DIR}/bin
    cp -rf  ${AGENT_ROOT_DIR}/bin/crossd ${AGENT_FPM_SOURCE_DIR}/bin
    #config
    cp -rf  ${RPM_MODULE_DIR}/config/*.json ${AGENT_FPM_SOURCE_DIR}/config
    #scripts
    cp -rf  ${RPM_MODULE_DIR}/scripts/* ${AGENT_FPM_SOURCE_DIR}/scripts
    sed -i "s|install_dir={}|install_dir=${RPM_INSTALL_LINK}|g" ${AGENT_FPM_SOURCE_DIR}/scripts/cross
    sed -i "s|install_dir={}|install_dir=${RPM_INSTALL_LINK}|g" ${AGENT_FPM_SOURCE_DIR}/scripts/crossd

    cp -f ${AGENT_CURRENT_DIR}/pre_install.sh ${AGENT_FPM_DIR}/SHELL
    sed -i "s|RPM_INSTALL_LINK={}|RPM_INSTALL_LINK=${RPM_INSTALL_LINK}|g" ${AGENT_FPM_DIR}/SHELL/pre_install.sh 

    cp -f ${AGENT_CURRENT_DIR}/after_install.sh ${AGENT_FPM_DIR}/SHELL
    sed -i "s|RPM_INSTALL_LINK={}|RPM_INSTALL_LINK=${RPM_INSTALL_LINK}|g" ${AGENT_FPM_DIR}/SHELL/after_install.sh
    sed -i "s|RPM_INSTALL_VERSION={}|RPM_INSTALL_VERSION=${PACK_VERSION}|g" ${AGENT_FPM_DIR}/SHELL/after_install.sh
    sed -i "s|RPM_INSTALL_DATA_DIR={}|RPM_INSTALL_DATA_DIR=${RPM_INSTALL_DATA_DIR}|g" ${AGENT_FPM_DIR}/SHELL/after_install.sh
    sed -i "s|RPM_INSTALL_LOG_DIR={}|RPM_INSTALL_LOG_DIR=${RPM_INSTALL_LOG_DIR}|g" ${AGENT_FPM_DIR}/SHELL/after_install.sh

    echo "fpm build prepare end..."
}

build_fpm() {
    echo "fpm build build_fpm start..."
    echo ${PACK_TYPE}
    echo ${PACK_VERSION}
    echo ${PACK_RELEASE}
    fpm -s dir -t ${PACK_TYPE} -n ${PACK_NAME} -v ${PACK_VERSION} --iteration ${PACK_RELEASE} -C ${AGENT_FPM_DIR}/SOURCE -f -p ${AGENT_FPM_DIR}/RPMS --prefix ${RPM_INSTALL_DIR} --pre-install ${AGENT_FPM_DIR}/SHELL/pre_install.sh --post-install ${AGENT_FPM_DIR}/SHELL/after_install.sh
    echo "fpm build build_fpm end..."
}


#####################main
#for make fpm
if [ $# -eq 0 ]; then
    prepare
    build_fpm
    exit 0
#for cicd pack
elif [ $# -eq 4 ]; then
    echo "cicd pack for fpm build"
    PACK_VERSION=$1
    PACK_RELEASE=$2
    PACK_TYPE=$3
    EXEC_PATH=$4
    RPM_INSTALL_DIR=${RPM_INSTALL_LINK}-${PACK_VERSION}

    cd ${EXEC_PATH}
    echo ${EXEC_PATH}
    prepare
    build_fpm
    exit 0
else
    exit 1
fi

