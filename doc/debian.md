### 生成debian过程

1. 安装必需的包

```shell
sudo apt install build-essential
sudo apt install debmake
```

2. 生成debian目录

```shell
debmake -e "team+kylin@tracker.debian.org" -p "ukui-settings-daemon" -f "Kylin Team" -u 0.0.1 -m -n -x1
```
