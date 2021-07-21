#!/bin/bash

branch_name=( "master" "9x0" "intel" )
get_branch=0

projectDir="projectDir"

source_project_name="$projectDir/ukui-settings-daemon_[_project_name].pro"
source_control_name="$projectDir/control_[_project_name]"

dest_project_name=""
dest_control_name=""

control_name=""

for name in ${branch_name[*]}
do
	if [ $1 == $name ]
	then
		if [ $# -eq 1 ];then
			project_name=$1
			echo "准备提取：$1 版本的工程文件与依赖文件" 
			let get_branch=1
		elif [ $2 == "back" ];then
			project_name=$1
			let get_branch=2
		else

			let get_branch=0
		fi
	fi
done

if [ $get_branch -eq 0 ]; then
	echo "请输出正确的版本:${branch_name[*]}" 
	
elif [ $get_branch -eq 2 ]; then
	echo "退回完毕"
	dest_project_name=${source_project_name//"_[_project_name]"/}
	dest_control_name=${source_control_name//"_[_project_name]"/}
	
	dest_project_name=${dest_project_name//"$projectDir/"/}
	dest_control_name=${dest_control_name//"$projectDir/"/"debian/"}
	
	source_project_name=${source_project_name//"[_project_name]"/$project_name}
	source_control_name=${source_control_name//"[_project_name]"/$project_name}
	
	mv -v $dest_project_name $source_project_name
	mv -v $dest_control_name $source_control_name
	
else 
	dest_project_name=${source_project_name//"_[_project_name]"/}
	dest_control_name=${source_control_name//"_[_project_name]"/}
	
	dest_project_name=${dest_project_name//"$projectDir/"/}
	dest_control_name=${dest_control_name//"$projectDir/"/"debian/"}
	
	source_project_name=${source_project_name//"[_project_name]"/$project_name}
	source_control_name=${source_control_name//"[_project_name]"/$project_name}
	
	cp -v $source_project_name $dest_project_name
	cp -v $source_control_name $dest_control_name
	
	echo "生成 $project_name 版本完成 :$dest_project_name " 
fi
































