## ukui-setting-daemon 重构

### 项目安装依赖

1. git clone ...
2. 修改 /etc/apt/sources.list, 把 deb-src 之前的注释去掉，并执行 apt-get update
2. 执行 apt build-dep ukui-settings-daemon 会安装项目所有依赖

### 项目运行

1. 项目打包

- 根目录下执行如下命令，在上层目录生成 `deb` 文件

```shell
debuild -D
```

2. 项目安装

在生成 `deb` 前提下，先卸载系统上的项目再执行 `sudo apt install ./ukui-settings-daemon_xxxx.deb`

3. 项目运行

`ukui-settings-daemon --replace`

### 插件进度

> 选中表示确定可正常运行

- [ ] a11y-settings
- [ ] background
- [ ] clipboard
- [ ] datetime
- [ ] media-keys
- [ ] mouse
- [ ] mpris
- [ ] sound
- [ ] typing-break
- [ ] xrandr
- [ ] xrdb
- [ ] xsettings

### 有问题的插件

| 插件 | 问题 | 负责人 |
| --- | --- | --- |
| xsettings | 编译报错| |
| sound | 运行报错:空链表... | |
| mouse | 运行报错:有个接口未实现 | |
| datetime | 应该是个服务，目前请看不确定是否可用，未报错，这个可能需要特殊处理 | |
| xrdb | 运行报错:未定义的接口，且那里边的父类看起来没用啊 ...| |
| clipboard | 运行报错:此插件已运行...| 丁敬 |
| media-key | 未完成...| 丁敬 |
| xrandr | 运行报错:未定义的 `mate_rr_config_equal` | |
