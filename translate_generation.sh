#!/bin/bash
  
ts_list=(`ls daemon/res/i18n/*.ts`)
source /etc/os-release
version=(`echo $ID`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    if [ "$version" == "fedora" ] || [ "$version" == "opensuse-tumbleweed" ] || [ "$version" == "opensuse-leap" ];then
        lrelease-qt5 "${ts}"
    else
    lrelease "${ts}"
    fi
done
