#!/usr/bin/sh

if [ $UID -ne 0 ];then
#    print_warning "非root用户没有权限安装"
    exit 1
fi

rm -f /usr/bin/ukui-settings-daemon
rm -fr /usr/local/lib/ukui-settings-daemon/*
make clean
qmake
make -j8
sudo make install
