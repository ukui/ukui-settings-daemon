## ukui-setting-daemon 重构

### 需要完成的任务

- [ ] 框架重构
    - 目前先"打桩"，把类的形式定下来，可以同步开发插件
- [ ] 各个插件重构
    - 只是替换gobject的功能部分，插件都是打包成 ".so" 的动态库
- [ ] 汉化相关
    - QT 有体统汉化相关，但是考虑到其它语言的支持，尽可能从源码部分取字符串，不过考虑到这是个守护进程，这点应该可以不太严格

### 项目安装依赖

1. git clone ...
2. 修改 /etc/apt/sources.list, 把 deb-src 之前的注释去掉，并执行 apt-get update
2. 执行 apt build-dep ukui-settings-daemon 会安装项目所有依赖

### TODO
