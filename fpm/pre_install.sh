#!/bin/bash

RPM_INSTALL_LINK={}
RPM_INSTALL_CONFIG_DIR=${RPM_INSTALL_LINK}/config
RPM_CONFIG_TMP_DIR=/var/tmp

pre_check() {
    echo "starting pre install pre check exec"

    #检查环境变量是否设置
    if [ ${RPM_INSTALL_LINK} == "{}" ];
    then
        echo "pre install RPM_INSTALL_LINK not set, please check"
        return 3
    fi
    echo "RPM_INSTALL_LINK" ${RPM_INSTALL_LINK} "..."

    #检查链接配置是否存在 且 为链接
    if [ -e ${RPM_INSTALL_LINK} ];
    then
        echo "RPM_INSTALL_LINK Exist "
        if [ -L ${RPM_INSTALL_LINK} ];
        then
            echo "RPM_INSTALL_LINK Exist Is Link, Continue..."
            return 1
        else
            echo "RPM_INSTALL_LINK Exist Not Link, Please Check..."
            return 2
        fi
    else
        echo "RPM_INSTALL_LINK Not Exist"
        return 0
    fi
}

remove_link() {
    echo "starting pre install remove_link exec"

    echo "remove data..."
    if [ -L ${RPM_INSTALL_LINK}/data ];
    then
        echo "RPM_INSTALL_LINK/data Is Link, remove..."
        rm -rf ${RPM_INSTALL_LINK}/data
    fi

    echo "remove log..."
    if [ -L ${RPM_INSTALL_LINK}/log ];
    then
        echo "RPM_INSTALL_LINK/log Is Link, remove..."
        rm -rf ${RPM_INSTALL_LINK}/log
    fi

    echo "copy config"
    cp -rf ${RPM_INSTALL_CONFIG_DIR} ${RPM_CONFIG_TMP_DIR}

    echo "remove link..."
    rm -rf ${RPM_INSTALL_LINK}
}

########################main

pre_check
case "$?" in
  0)
	echo "pre_check 0, exit 0"
    exit 0
	;;
  1)
    echo "pre_check 1, exit 0"
	remove_link
    exit 0
	;;
  2)
	echo "pre_check 2, exit 1"
    exit 1
	;;
  3)
    echo "RPM_INSTALL_LINK Not Set,Please Check..."
    exit 1
	;;
  *)
	echo "Unknow pre_check return value $?"
	exit 1
esac
