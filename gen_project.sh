#!/bin/bash

#sample:
#生成：
#./gen_project.sh master
#./gen_project.sh 9x0
#./gen_project.sh tablet
#改动放回源文件夹
#./gen_project.sh back master
#./gen_project.sh back 9x0
#./gen_project.sh back tabletster

branch_name=( "master" "9x0" "tablet" )
get_branch=0

projectDir="projectDir"

source_project_name="$projectDir/ukui-settings-daemon_[_project_name].pro"
source_control_name="$projectDir/control_[_project_name]"
source_common_name="$projectDir/common_[_project_name].pri"

dest_project_name=""
dest_control_name=""
dest_common_name=""


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
	dest_common_name=${source_common_name//"_[_project_name]"/}
	
	dest_project_name=${dest_project_name//"$projectDir/"/}
	dest_control_name=${dest_control_name//"$projectDir/"/"debian/"}
	dest_common_name=${dest_common_name//"$projectDir/"/"common/"}
	
	source_project_name=${source_project_name//"[_project_name]"/$project_name}
	source_control_name=${source_control_name//"[_project_name]"/$project_name}
	source_common_name=${source_common_name//"[_project_name]"/$project_name}
	
	echo $dest_common_name
	echo $source_common_name
	cp -v $dest_project_name $source_project_name
	cp -v $dest_control_name $source_control_name
	cp -v $dest_common_name $source_common_name
	
	echo "退回完毕"
else 
	dest_project_name=${source_project_name//"_[_project_name]"/}
	dest_control_name=${source_control_name//"_[_project_name]"/}
	dest_common_name=${source_common_name//"_[_project_name]"/}
	
	
	dest_project_name=${dest_project_name//"$projectDir/"/}
	dest_control_name=${dest_control_name//"$projectDir/"/"debian/"}
	dest_common_name=${dest_common_name//"$projectDir/"/"common/"}
	
	
	source_project_name=${source_project_name//"[_project_name]"/$project_name}
	source_control_name=${source_control_name//"[_project_name]"/$project_name}
	source_common_name=${source_common_name//"[_project_name]"/$project_name}
	
	
	cp -v $source_project_name $dest_project_name
	cp -v $source_control_name $dest_control_name
	cp -v $source_common_name $dest_common_name
	
	echo "生成 $project_name 版本完成 :$dest_project_name " 
fi

