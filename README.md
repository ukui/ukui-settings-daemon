## ukui-setting-daemon 平板模式

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

### 插件功能介绍

| 插件 | 功能 |
| --- | --- |
| a11y-keyboard | 辅助功能      |
| a11y-settings | 辅助功能      |
| background    | 设置背景      |
| clipboard     | 剪贴板        |
| common        | 抓取键盘按键  |
| color         | 色温设置      |
| housekepping  | 实时监听磁盘空间|
| keybindings   | 自定义绑定快捷键|
| keyboard      | 键盘设置      |
| media-keys    | 系统快捷键    |
| mouse         | 鼠标设置      |
| mpris         | 特殊媒体快捷按键|
| sound         | 提示音设置    |
| tablet-mode   | 平板模式      |
| xrandr        | 记住注销分辨率设置 |
| xrdb          | 设置主题      |
| xsettings     | 设置底层系统主题变量 |
