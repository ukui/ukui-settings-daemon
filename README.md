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
- [x] background
- [x] clipboard
- [x] common
- [x] housekepping
- [x] keybindings
- [x] keyboard
- [x] media-keys
- [x] mouse
- [x] mpris
- [x] sound
- [x] xrandr
- [x] xrdb
- [x] xsettings

### 有问题的插件

| 插件 | 问题 | 负责人 |
| --- | --- | --- |
| a11y-keyboard | 搁置 |  |
| a11y-settings | 搁置 |  |
| smartcard | 搁置 |  |
| media-key | 功能不完善 | 闫焕章 |
| xrandr | 功能不完善 | 商晓阳 |
