## ukui-setting-daemon

 `ukui-settings-daemon` 是`UKUI`桌面环境下的底层守护程序；
 负责设置`UKUI`会话的各种参数以及运行的应用程序。

### 项目安装依赖

1. 执行：`git clone https://github.com/ukui/ukui-settings-daemon.git` 下载代码
2. 执行：`git checkout dev_qt` 切换为`dev_qt`分支
3. 执行: `sudo mk-build-deps -ir debian/control` 会安装项目所有依赖

### 项目运行

1. 项目打包

- 根目录下执行如下命令，在上层目录生成 `deb` 文件

```shell
debuild -D
```

2. 项目安装

上层目录生成 `deb` 二进制文件后，进入上层目录，执行 `sudo dpkg -i ./ukui-settings-daemon_xxxx.deb`

3. 项目运行

安装后运行：`ukui-settings-daemon --replace`

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
