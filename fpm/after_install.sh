#!/bin/bash

RPM_INSTALL_LINK={}
RPM_INSTALL_VERSION={}
RPM_INSTALL_ACTUAL_DIR=${RPM_INSTALL_LINK}-${RPM_INSTALL_VERSION}
RPM_INSTALL_DATA_DIR={}
RPM_INSTALL_LOG_DIR={}
RPM_CONFIG_TMP_DIR=/var/tmp

pre_check() {
    echo "starting after install pre check exec"

    #检查环境变量是否设置
    if [ ${RPM_INSTALL_LINK} == "{}" ];
    then
        echo "after install RPM_INSTALL_LINK not set, please check"
        return 2
    fi
    echo "RPM_INSTALL_LINK" ${RPM_INSTALL_LINK} "..."

    if [ ${RPM_INSTALL_VERSION} == "{}" ];
    then
        echo "after install RPM_INSTALL_VERSION not set, please check"
        return 2
    fi
    echo "RPM_INSTALL_VERSION" ${RPM_INSTALL_VERSION} "..."

    if [ ${RPM_INSTALL_DATA_DIR} == "{}" ];
    then
        echo "after install RPM_INSTALL_DATA_DIR not set, please check"
        return 2
    fi
    echo "RPM_INSTALL_DATA_DIR" ${RPM_INSTALL_DATA_DIR} "..."

    if [ ${RPM_INSTALL_LOG_DIR} == "{}" ];
    then
        echo "after install RPM_INSTALL_LOG_DIR not set, please check"
        return 2
    fi
    echo "RPM_INSTALL_LOG_DIR" ${RPM_INSTALL_LOG_DIR} "..."

    #检查安装目录是否存在
    if [ -e ${RPM_INSTALL_ACTUAL_DIR} ];
    then
        echo "after install RPM_INSTALL_ACTUAL_DIR dir Exist, Continue..."
    else
        echo "after install RPM_INSTALL_ACTUAL_DIR dir not Exist, please check"
        return 1
    fi
    #检查data目录是否存在
    if [ -e ${RPM_INSTALL_DATA_DIR} ];
    then
        echo "after install RPM_INSTALL_DATA_DIR dir Exist, Continue..."
    else
        echo "after install RPM_INSTALL_DATA_DIR dir not Exist, please check"
        return 1
    fi
    #检查log目录是否存在
    if [ -e ${RPM_INSTALL_DATA_DIR} ];
    then
        echo "after install RPM_INSTALL_DATA_DIR dir Exist, Continue..."
    else
        echo "after install RPM_INSTALL_DATA_DIR dir not Exist, please check"
        return 1
    fi

    return 0
}

build_link() {
    echo "starting after install build_link exec"

    if [ -e ${RPM_CONFIG_TMP_DIR}/config ];
    then
        cp -rf ${RPM_CONFIG_TMP_DIR}/config/*.json ${RPM_INSTALL_ACTUAL_DIR}/config
    fi

    echo "build link..."
    ln -s ${RPM_INSTALL_ACTUAL_DIR} ${RPM_INSTALL_LINK}

    echo "build agent & agentd link to /etc/init.d"
    ln -s -f ${RPM_INSTALL_LINK}/scripts/agent /etc/init.d/agent
    ln -s -f ${RPM_INSTALL_LINK}/scripts/agentd /etc/init.d/agentd
}

########################main

pre_check
case "$?" in
  0)
	echo "pre_check 0, exit 0"
    build_link
    exit 0
	;;
  1)
    echo "pre_check 1, exit 1"
    exit 1
	;;
  2)
	echo "pre_check 2, exit 1"
    exit 1
	;;
  *)
	echo "Unknow pre_check return value $?"
	exit 1
esac
