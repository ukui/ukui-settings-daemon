#!/bin/bash
cur_dir=$(pwd)
name=".so"
plugins="/plugins"
path=$cur_dir$plugins
usdName="ukui-settings-daemon"
#usdLibPath="/usr/lib/x86_64-linux-gnu/ukui-settings-daemon/"
usdLibPath="/usr/lib/aarch64-linux-gnu/ukui-settings-daemon/"
libPath=""

sudo cp -v ukui-settings-daemon/ukui-settings-daemon /usr/bin/

mvsotolib(){
	local workdir
	workdir=$1
	cd $1
	for dir in `ls -a $workdir`
	do
		if test -d "$dir";
		then
			if [ $dir != "." ] &&  [ $dir != ".." ]
			then
				#echo enter to $dir !!!!!!!!!!!!!!!!!!!!!!!!!
				cd $dir
				mvsotolib $workdir/$dir $2
				cd ..
			fi
		else
			local filename=$dir
			if  [[ ${filename:(-${#2})} = $2 ]]
			then
#				echo $filename had $2
				sudo cp $filename -v $usdLibPath
#			else
#				echo $filename "had't" $2
			fi
		fi
	done
}

findLibPath(){
	local workdir localDir
	echo new loop
	cd $1
	localDir=`pwd`
	workdir=$1
	echo local:$localDir
	for dir in `ls $workdir`
	do
		if test -d "$dir";
		then
			echo find a dir:$dir
			if [ $dir = $usdName ];
			then
				cd $dir
				libPath=`pwd`
				echo libpath $libPath
				return 0;
			else
				cd $dir
				echo enter to $dir
				findLibPath $1"/"$dir
				cd ..
			fi
		fi
	done
}

#findLibPath $usdLibPath

mvsotolib $path $name





